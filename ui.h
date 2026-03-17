#ifndef UI_H
#define UI_H

#include <ncurses.h>
// #include <stddef.h>

#include "parser.h"

typedef enum {VIEW, SELECTION} UIState;

typedef struct page {
    WINDOW *win;

    gemtext_t *contents;
    uri_t *uri;

    UIState ui_state;

    int line_index;
    int selection_index;
    int last_line;

    // char *selected_uri;
    char *selected_line;

    char *data;

    struct page *prev_page;
    struct page *next_page;
} page_t;

typedef struct {
    WINDOW *win;
    char data[129];
} io_win_t;

page_t *page_new();
void page_free(page_t *page);
void page_reload_contents(page_t *page);
void page_refresh(page_t *page);

void set_nearest_selection_index(page_t *page);

void read_user_input(io_win_t *io_win);
void push_msg(io_win_t *io_win, const char *msg);

int form_window_2_opt(const char *msg, const char **opts, int height, int width);

#endif