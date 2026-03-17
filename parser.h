#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

typedef enum {TEXT, LINK, HEADING, LIST, QUOTE, PREFORMATTED} LineType;
typedef enum {PATH_RELATIVE, ROOT_RELATIVE, HIERARCHICAL, OPAQUE, ABSOLUTE} URIType;

typedef struct {
    char *line;
    LineType line_type;
    int line_num;
} gemtext_line_t;

typedef struct {
    char *protocol;
    char *hostname;
    char *path;
    URIType uri_type;
} uri_t;

typedef struct {
    gemtext_line_t *lines;
    size_t line_count;
    size_t link_count;
} gemtext_t;

URIType uri_type(char *uri_str);
char *parse_link_from_line(char *line);
void uri_normalize_end(char *uri_str);
void uri_parse(uri_t *uri, char *uri_str);
void uri_parse_relative(uri_t *uri, char *uri_str, uri_t *current_uri);
void uri_free(uri_t *uri);
void uri_to_str(char *dest, uri_t *uri);
void uri_to_file_path(uri_t *uri, char *dest);

void gemtext_parse(gemtext_t *gemtext, char *contents_str);
void gemtext_free(gemtext_t *gemtext);

#endif