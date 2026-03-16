#include <asm-generic/errno.h>
#include <asm-generic/socket.h>
#include <netdb.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <s2n.h>
#include <fcntl.h>
#include <errno.h>

#include "network.h"
#include "utils.h"

static int socket_connect(const char *hostname) {
    struct addrinfo hints, *result, *result_ptr;
    int status, socket_fd;


    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = 0;

    status = getaddrinfo(hostname, GEMINI_PORT, &hints, &result);
    if (status != 0) {
        //fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        //exit(EXIT_FAILURE);

        return -1;
    }

    for (result_ptr = result; result_ptr != NULL; result_ptr = result_ptr->ai_next) {
        socket_fd = socket(result_ptr->ai_family, result_ptr->ai_socktype, result_ptr->ai_protocol);

        if (socket_fd == -1)
            continue;

        int flags = fcntl(socket_fd, F_GETFL, 0);
        fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);


        int connect_res = connect(socket_fd, result_ptr->ai_addr, result_ptr->ai_addrlen);

        if(connect_res == -1 && errno == EINPROGRESS) {
            struct timeval tv;
            tv.tv_sec = 5;
            tv.tv_usec = 0;

            fd_set writefds;
            FD_ZERO(&writefds);
            FD_SET(socket_fd, &writefds);

            int select_res = select(socket_fd + 1, NULL, &writefds, NULL, &tv);

            if(select_res > 0) {
                int so_error;
                socklen_t len = sizeof(so_error);
                getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &so_error, &len);

                if(so_error == 0) 
                    goto success;
            }
        } else if(!connect_res)
            goto success;

        close(socket_fd);
        socket_fd = -1;
    }

    freeaddrinfo(result);
    return -1;

success:
    freeaddrinfo(result);

    int final_flags = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, final_flags & ~O_NONBLOCK);

    return socket_fd;
}

// TODO: better error handling
int connection_initialize(connection_handle_t *connection_handle) {
    s2n_init();

    connection_handle->s2n_connection = s2n_connection_new(S2N_CLIENT);
    connection_handle->s2n_config = s2n_config_new_minimal();
    int res = s2n_config_disable_x509_verification(connection_handle->s2n_config);

    return (connection_handle->s2n_connection == NULL && connection_handle->s2n_config == NULL) ? -1 : 0;
}

void connection_wipe(connection_handle_t *connection_handle) {
    if(socket >= 0) {
        close(connection_handle->socket_fd);
    }

    s2n_connection_wipe(connection_handle->s2n_connection);
}

void connection_cleanup(connection_handle_t *connection_handle) {
    if(socket >= 0) {
        close(connection_handle->socket_fd);
    }

    s2n_connection_free(connection_handle->s2n_connection);
    s2n_config_free(connection_handle->s2n_config);

    s2n_cleanup_final();
}

int connection_establish(connection_handle_t *connection_handle, const char *hostname) {
    s2n_blocked_status blocked;

    int socket_fd = socket_connect(hostname);

    if(socket_fd < 0)
        return -1; 

    connection_handle->socket_fd = socket_fd;

    s2n_connection_set_fd(connection_handle->s2n_connection, connection_handle->socket_fd);
    s2n_set_server_name(connection_handle->s2n_connection, hostname);
    s2n_connection_set_config(connection_handle->s2n_connection, connection_handle->s2n_config);

    return s2n_negotiate(connection_handle->s2n_connection, &blocked);
}

int connection_send(connection_handle_t *connection_handle, const char *request, ssize_t request_len) {
    s2n_blocked_status blocked;
    return s2n_send(connection_handle->s2n_connection, request, request_len, &blocked);
}

char *connection_receive(connection_handle_t *connection_handle) {
    s2n_blocked_status blocked;
    char *response = NULL;
    char buf[1024];
    ssize_t bytes_readed = 0;
    ssize_t total_bytes_readed = 0;

    while(1) {
        bytes_readed = s2n_recv(connection_handle->s2n_connection, buf, 1024, &blocked);

        if(bytes_readed <= 0)
            break;

        response = realloc(response, total_bytes_readed + bytes_readed);
        memcpy(response + total_bytes_readed, buf, bytes_readed);
        total_bytes_readed += bytes_readed;
    }

    if(!total_bytes_readed) {
        return NULL;
    }

    response = realloc(response, total_bytes_readed + 1);
    response[total_bytes_readed] = '\0';
    return response;
}