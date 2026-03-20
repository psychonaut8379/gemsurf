#include <netdb.h>
#include <openssl/bio.h>
#include <openssl/prov_ssl.h>
#include <openssl/tls1.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <openssl/ssl.h>

#include "network.h"

// static int socket_connect(const char *hostname) {
//     struct addrinfo hints, *result, *result_ptr;
//     int status, socket_fd;


//     memset(&hints, 0, sizeof(hints));
//     hints.ai_family = AF_INET;
//     hints.ai_socktype = SOCK_STREAM;
//     hints.ai_protocol = 0;
//     hints.ai_flags = 0;

//     status = getaddrinfo(hostname, GEMINI_PORT, &hints, &result);
//     if (status != 0) {
//         //fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
//         //exit(EXIT_FAILURE);

//         return -1;
//     }

//     for (result_ptr = result; result_ptr != NULL; result_ptr = result_ptr->ai_next) {
//         socket_fd = socket(result_ptr->ai_family, result_ptr->ai_socktype, result_ptr->ai_protocol);

//         if (socket_fd == -1)
//             continue;

//         int flags = fcntl(socket_fd, F_GETFL, 0);
//         fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);


//         int connect_res = connect(socket_fd, result_ptr->ai_addr, result_ptr->ai_addrlen);

//         if(connect_res == -1 && errno == EINPROGRESS) {
//             struct timeval tv;
//             tv.tv_sec = 5;
//             tv.tv_usec = 0;

//             fd_set writefds;
//             FD_ZERO(&writefds);
//             FD_SET(socket_fd, &writefds);

//             int select_res = select(socket_fd + 1, NULL, &writefds, NULL, &tv);

//             if(select_res > 0) {
//                 int so_error;
//                 socklen_t len = sizeof(so_error);
//                 getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &so_error, &len);

//                 if(so_error == 0) 
//                     goto success;
//             }
//         } else if(!connect_res)
//             goto success;

//         close(socket_fd);
//         socket_fd = -1;
//     }

//     freeaddrinfo(result);
//     return -1;

// success:
//     freeaddrinfo(result);

//     int final_flags = fcntl(socket_fd, F_GETFL, 0);
//     fcntl(socket_fd, F_SETFL, final_flags & ~O_NONBLOCK);

//     return socket_fd;
// }

// uint8_t verify_host_callback(const char *hostname, size_t hostname_len, void *data) {
//     return 1;
// }

static BIO *create_socket_bio(const char *hostname, const char *port, int family) {
    int socket_fd = -1;
    BIO_ADDRINFO *addrinfo, *result;
    BIO *bio;

    if(!BIO_lookup_ex(hostname, port, BIO_LOOKUP_CLIENT, family, SOCK_STREAM, 0, &result)) 
        return NULL;

    for(addrinfo = result; addrinfo; BIO_ADDRINFO_next(addrinfo)) {
        socket_fd = BIO_socket(BIO_ADDRINFO_family(addrinfo), SOCK_STREAM, 0, 0);
        if(socket_fd == -1)
            continue;

        if(!BIO_connect(socket_fd, BIO_ADDRINFO_address(addrinfo), BIO_SOCK_NODELAY)) {
            BIO_closesocket(socket_fd);
            socket_fd = -1;
            continue;
        }

        break;
    }

    BIO_ADDRINFO_free(addrinfo);

    if(socket_fd == -1)
        return NULL;

    bio = BIO_new(BIO_s_socket());
    if(!bio) {
        BIO_closesocket(socket_fd);
        return NULL;
    }

    BIO_set_fd(bio, socket_fd, BIO_CLOSE);

    return bio;
}

// TODO: better error handling
int connection_initialize(connection_handle_t *connection_handle) {
    SSL_CTX *ssl_ctx = SSL_CTX_new(TLS_client_method());
    if(!ssl_ctx)
        return -1;

    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, NULL);
    SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_2_VERSION);

    connection_handle->ssl_ctx = ssl_ctx;
    return 0; // ?
}

void connection_wipe(connection_handle_t *connection_handle) {
    SSL_free(connection_handle->ssl);
    connection_handle->ssl = NULL;
}

void connection_cleanup(connection_handle_t *connection_handle) {
    if(connection_handle->ssl) 
        SSL_free(connection_handle->ssl);

    SSL_CTX_free(connection_handle->ssl_ctx);
}

int connection_establish(connection_handle_t *connection_handle, const char *hostname) {
    connection_handle->ssl = SSL_new(connection_handle->ssl_ctx);
    if(!connection_handle->ssl)
        goto failure;    

    connection_handle->bio = create_socket_bio(hostname, GEMINI_PORT, AF_INET);
    if(!connection_handle->bio)
        goto failure;

    SSL_set_bio(connection_handle->ssl, connection_handle->bio, connection_handle->bio);

    if(!SSL_set_tlsext_host_name(connection_handle->ssl, hostname)) 
        goto failure;

    if(SSL_connect(connection_handle->ssl) < 1)
        goto failure;

    return 0;

    failure:
    SSL_free(connection_handle->ssl);
    return -1;

    // s2n_blocked_status blocked;

    // int socket_fd = socket_connect(hostname);

    // if(socket_fd < 0)
    //     return -1; 

    // connection_handle->socket_fd = socket_fd;

    // s2n_connection_set_fd(connection_handle->s2n_connection, connection_handle->socket_fd);
    // s2n_set_server_name(connection_handle->s2n_connection, hostname);
    // s2n_connection_set_config(connection_handle->s2n_connection, connection_handle->s2n_config);

    // s2n_negotiate(connection_handle->s2n_connection, &blocked);
    // const char *err = s2n_strerror(s2n_errno, "EN"); 

    // const int MAX_ATTEMPTS = 5000;
    // int attempts = 0;
    // int res;
    // do {
    //     res = s2n_negotiate(connection_handle->s2n_connection, &blocked);

    //     if(res != S2N_SUCCESS) {
    //         if(s2n_error_get_type(s2n_errno) != S2N_ERR_T_BLOCKED) {
    //             close(socket_fd);
    //             return S2N_FAILURE;
    //         }
    //     }

    //     attempts++;

    //     if(attempts >= MAX_ATTEMPTS) {
    //         close(socket_fd);
    //         return 1;
    //     }
    // } while(res != S2N_SUCCESS);

    // int final_flags = fcntl(socket_fd, F_GETFL, 0);
    // fcntl(socket_fd, F_SETFL, final_flags & ~O_NONBLOCK);

    // return res;
}

int connection_send(connection_handle_t *connection_handle, const char *request, ssize_t request_len) {
    size_t written;
    if(!SSL_write_ex(connection_handle->ssl, request, request_len, &written))
        return -1;

    return 0;
}

char *connection_receive(connection_handle_t *connection_handle) {
    char buf[1024];
    char *response = NULL;
    size_t bytes_readed;
    size_t total_bytes_readed = 0;
    while(1) {
        if(!SSL_read_ex(connection_handle->ssl, buf, sizeof(buf), &bytes_readed))
            break;

        response = realloc(response, total_bytes_readed + bytes_readed);
        memcpy(response + total_bytes_readed, buf, bytes_readed);
        total_bytes_readed += bytes_readed;
    }

    if(!total_bytes_readed)
        return NULL;

    response = realloc(response, total_bytes_readed + 1);
    response[total_bytes_readed] = '\0';
    return response;



    // s2n_blocked_status blocked;
    // char *response = NULL;
    // char buf[1024];
    // ssize_t bytes_readed = 0;
    // ssize_t total_bytes_readed = 0;

    // while(1) {
    //     bytes_readed = s2n_recv(connection_handle->s2n_connection, buf, 1024, &blocked);

    //     if(bytes_readed <= 0)
    //         break;

    //     response = realloc(response, total_bytes_readed + bytes_readed);
    //     memcpy(response + total_bytes_readed, buf, bytes_readed);
    //     total_bytes_readed += bytes_readed;
    // }

    // if(!total_bytes_readed) {
    //     return NULL;
    // }

    // response = realloc(response, total_bytes_readed + 1);
    // response[total_bytes_readed] = '\0';
    // return response;
}