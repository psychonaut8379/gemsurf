#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

#define FREE_SAFE(p) do {free(p); p = NULL;} while(0);

void normalize_newlines(char *s);
char *next_word(char **s);

#endif