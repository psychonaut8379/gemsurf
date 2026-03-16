
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

char *next_word(char *s) {
    while(*s != ' ' && *s != '\t')
        s++;

    while(*s == ' ' || *s == '\t')
        s++;

    char *start = s;

    while(*s != ' ' && *s != '\t') 
        s++;

    return strndup(start, s - start);
}