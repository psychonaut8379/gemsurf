
#include <utils.h>

void normalize_newlines(char *s) {
    char *src = s;
    char *dest = s;

    while(1) {
        if(!*src) {
            *dest = '\0';
            break;
        }
        if(*src != '\r') {
            *dest = *src;
            dest++;
        }

        src++;
    }
}

static void skip_whitespaces(char **s) {
    while(**s == ' ' || **s == '\t')
        (*s)++;
}