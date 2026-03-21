#include <openssl/core_dispatch.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <string.h>
#include <stdio.h>

#include "tofu.h"
#include "utils.h"

/*
    get hosts certificate and hashes into sha256 then prints hash into fingerprint_dest 
    NOTE: fingerprint_dest must be equal or larger than (EVP_MAX_MD_SIZE * 2 + 1) 
*/

void read_fingerprint(char *fingerprint_dest, SSL *ssl) {
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int n;

    X509 *certificate = SSL_get_peer_certificate(ssl);

    X509_digest(certificate, EVP_sha256(), md, &n);

    // convert to string
    for(unsigned int i = 0; i < n; i++) {
        sprintf(&fingerprint_dest[i * 2], "%02x", md[i]);
    }

    X509_free(certificate);
}

TrustStatus is_host_trusted(const char *hostname, const char *fingerprint) {
    char known_host_line[512];
    FILE *known_hosts_file = fopen(DATADIR "/known_hosts", "rb");
    TrustStatus status = NOT_FOUND;

    while(fgets(known_host_line, 256, known_hosts_file)) {
        char *ptr = known_host_line;

        char *known_hostname = next_word(&ptr);
        char *known_fingerprint = next_word(&ptr);

        if(strcmp(hostname, known_hostname)) 
            continue;

        if(strcmp(fingerprint, known_fingerprint)) {
            status = NOT_TRUSTED;
            break;
        }

        status = TRUSTED;
    }

    fclose(known_hosts_file);
    return status;
}

void add_known_host_entry(const char *hostname, const char *fingerprint) {
    FILE *known_hosts_file = fopen(DATADIR "/known_hosts", "ab");
    fprintf(known_hosts_file, "%s %s\n", hostname, fingerprint);
    fclose(known_hosts_file);
}