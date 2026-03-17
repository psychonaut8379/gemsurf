#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "parser.h"
#include "utils.h"

URIType uri_type(char *uri_str) {
    if(uri_str[0] == '/')
        return ROOT_RELATIVE;

    char *protocol = strchr(uri_str, ':');

    if(!protocol || protocol == uri_str)
        return PATH_RELATIVE;

    if(*(protocol + 1) == '/' && *(protocol + 2) == '/') 
        return HIERARCHICAL;
    else 
        return OPAQUE;
}

char *parse_link_from_line(char *line) {
    while(*line != ' ' && *line != '\t')
        line++;

    while(*line == ' ' || *line == '\t')
        line++;

    char *start = line;

    while(*line != ' ' && *line != '\t') 
        line++;

    char *link = strndup(start, line - start);
    return link;
}

void uri_normalize_end(char *uri_str) {
    int uri_len = strlen(uri_str);

    if(uri_str[uri_len - 1] != '/') {
        uri_str[uri_len] = '/';
        uri_str[uri_len + 1] = '\0';
    }
} 

void uri_parse(uri_t *uri, char *uri_str) {
    uri->uri_type = uri_type(uri_str);

    char *protocol = strchr(uri_str, ':');
    if(!protocol) {
        protocol = uri_str;
        uri->protocol = strdup("gemini");
    } else {
        uri->protocol = strndup(uri_str, protocol - uri_str);

        do {
            protocol++;
        } while(*protocol == '/');
    }
    
    char *hostname = strstr(protocol, "/");
    if(!hostname) {
        return;
    }

    uri->hostname = strndup(protocol, hostname - protocol);
    uri->path = strdup(hostname);
}

void uri_parse_relative(uri_t *uri, char *uri_str, uri_t *current_uri) {
    if(uri_str[0] == '/') {
        uri->protocol = strdup(current_uri->protocol);
        uri->hostname = strdup(current_uri->hostname);
        uri->path = strdup(uri_str);
        uri->uri_type = current_uri->uri_type;

        return;
    }

    uri->protocol = strdup(current_uri->protocol);
    uri->hostname = strdup(current_uri->hostname);
    char *path = calloc(1, strlen(current_uri->path) + strlen(uri_str) + 1);
    strcat(path, current_uri->path);
    strcat(path, uri_str);
    uri->path = path;
    uri->uri_type = current_uri->uri_type;
}

void uri_free(uri_t *uri) {
    if(uri->protocol != NULL)
        FREE_SAFE(uri->protocol);

    if(uri->hostname != NULL)
        FREE_SAFE(uri->hostname);

    if(uri->path != NULL)
        FREE_SAFE(uri->path);

    free(uri);
}

void uri_to_file_path(uri_t *uri, char *dest) {
    int len = strlen(uri->hostname) + strlen(uri->path) + 2;

    snprintf(dest, len, "/%s%s", uri->hostname, uri->path);

    dest[len - 2] = '\0';
}

void uri_to_str(char *dest, uri_t *uri) {
    if(uri->uri_type == OPAQUE) 
        sprintf(dest, "%s:%s%s", uri->protocol, uri->hostname, uri->path);
    else if(uri->uri_type == HIERARCHICAL)
        sprintf(dest, "%s://%s%s", uri->protocol, uri->hostname, uri->path);
}

static LineType get_line_type(const char *line) {
    if(line[0] == '=' && line[1] == '>') {
        return LINK;
    } else if(line[0] == '#') {
        return HEADING;
    } else if(line[0] == '*') {
        return LIST;
    } else if(line[0] == '>') {
        return QUOTE;
    } else if(line[0] == '`' && line[1] == '`' && line[2] == '`') {
        return PREFORMATTED;
    } else {
        return TEXT;
    }
}

static char *next_line(char **s) {
    char *start = *s;

    while(**s != '\n' && **s) {
        (*s)++;
    }

    if(**s == '\n')
        **s = '\0', (*s)++;

    return start;
}

void gemtext_parse(gemtext_t *gemtext, char *contents_str) {
    gemtext_line_t *lines = NULL;    
    int line_index = 0;
    int link_index = 0;
    int preformatted_mode = 0;
    while(1) {
        char *line = next_line(&contents_str);
        if(!*contents_str) {
            lines = realloc(lines, sizeof(gemtext_t) * (line_index + 1));

            lines[line_index].line = line;

            LineType line_type = get_line_type(line);
            if(line_type == LINK)
                link_index++;

            if(preformatted_mode)
                lines[line_index].line_type = PREFORMATTED;
            else
                lines[line_index].line_type = line_type;

            if(line_type == PREFORMATTED && !preformatted_mode)
                preformatted_mode = 1;
            else if(line_type == PREFORMATTED && preformatted_mode)
                preformatted_mode = 0;

            line_index++;

            break;
        }

        lines = realloc(lines, sizeof(gemtext_t) * (line_index + 1));

        lines[line_index].line = line;

        LineType line_type = get_line_type(line);
        if(line_type == LINK)
            link_index++;

        if(preformatted_mode)
            lines[line_index].line_type = PREFORMATTED;
        else
            lines[line_index].line_type = line_type;

        if(line_type == PREFORMATTED && !preformatted_mode)
            preformatted_mode = 1;
        else if(line_type == PREFORMATTED && preformatted_mode)
            preformatted_mode = 0;

        line_index++;
    }

    gemtext->lines = lines;
    gemtext->line_count = line_index;
    gemtext->link_count = link_index;
}

void gemtext_free(gemtext_t *gemtext) {
    free(gemtext->lines);
    free(gemtext);
}