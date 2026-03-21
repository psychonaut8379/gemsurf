#ifndef TOFU_H
#define TOFU_H

#include <openssl/ssl.h>

typedef enum {TRUSTED, NOT_TRUSTED, NOT_FOUND} TrustStatus;

void read_fingerprint(char *fingerprint_dest, SSL *ssl);
TrustStatus is_host_trusted(const char *hostname, const char *fingerprint);
void add_known_host_entry(const char *hostname, const char *fingerprint);

#endif