    #ifndef NETWORK_H
    #define NETWORK_H
    
    #include <openssl/ssl.h>

    #define GEMINI_PORT "1965"

    typedef struct {
        int socket_fd;

        SSL_CTX *ssl_ctx;
        SSL *ssl;
        BIO *bio;
        struct s2n_config *s2n_config;
    } connection_handle_t;

    int connection_initialize(connection_handle_t *connection_handle);
    void connection_wipe(connection_handle_t *connection_handle);
    void connection_cleanup(connection_handle_t *connection_handle);
    int connection_establish(connection_handle_t *connection_handle, const char *hostname);
    int connection_send(connection_handle_t *connection_handle, const char *request, ssize_t request_len);
    char *connection_receive(connection_handle_t *connection_handle);

    #endif