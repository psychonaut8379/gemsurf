#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

#include "ui.h"
#include "parser.h"

page_t *page_new() {
    int y, x;
    getmaxyx(stdscr, y, x);

    page_t *new_page = calloc(1, sizeof(page_t));

    new_page->win = newpad(1024, x);

    new_page->contents = calloc(1, sizeof(gemtext_t));
    new_page->uri = calloc(1, sizeof(uri_t));

    return new_page;
}

void page_free(page_t *page) {
    delwin(page->win);

    uri_free(page->uri);

    gemtext_free(page->contents);

    if(page->data) 
        free(page->data);

    free(page);
}

static void page_load_contents_view(page_t *page) {
    int y, x;

    for(int i = 0; i < page->contents->line_count; i++) {
        getyx(page->win, y, x);
        page->contents->lines[i].line_num = y;
        wprintw(page->win, "%s\n", page->contents->lines[i].line);
    }

    getyx(page->win, y, x);
    page->last_line = y;
}

static void page_load_contents_selection(page_t *page) {
    int y, x;
    int ymax, xmax;

    getmaxyx(stdscr, ymax, xmax);

    int selection_index = 0;
    for(int i = 0; i < page->contents->line_count; i++) {
        getyx(page->win, y, x);
        page->contents->lines[i].line_num = y;

        if(page->contents->lines[i].line_type == LINK && selection_index == page->selection_index) {
            wattron(page->win, A_REVERSE);
            wprintw(page->win, "%s\n", page->contents->lines[i].line);
            wattroff(page->win, A_REVERSE);

            getmaxyx(stdscr, ymax, xmax);
            if(page->line_index + ymax - 2 < page->contents->lines[i].line_num)
                page->line_index = page->contents->lines[i].line_num;

            page->selected_line = page->contents->lines[i].line;
        } else 
            wprintw(page->win, "%s\n", page->contents->lines[i].line);
            
        if(page->contents->lines[i].line_type == LINK) 
            selection_index++;           
    }

    if(!selection_index)
        page->ui_state = VIEW;

    getyx(page->win, y, x);
    page->last_line = y;
}

void page_reload_contents(page_t *page) {
    werase(page->win);

    if(page->ui_state == VIEW)
        page_load_contents_view(page);
    else if(page->ui_state == SELECTION)
        page_load_contents_selection(page);
}

void page_refresh(page_t *page) {
    int y, x;
    getmaxyx(stdscr, y, x);

    prefresh(page->win, page->line_index, 0, 0, 0, y - 2, x);
}

void set_nearest_selection_index(page_t *page) {
    int link_index = 0;

    for(int i = 0; i < page->contents->line_count; i++) {
        if(page->contents->lines[i].line_type == LINK && page->contents->lines[i].line_num >= page->line_index) {
            page->selection_index = link_index;
            break;
        }

        if(page->contents->lines[i].line_type == LINK)
            link_index++;
    }
}

static void put_char(char *s, int ch, int index) {
    char *start = s;

    while(*s)
        s++;

    while(s != start && s != start + index) {
        *s = *(s - 1);
        s--;
    }

    *s = ch;
}

static void remove_char(char *s, int index) {
    s += index;
    while(*s) {
        *s = *(s + 1);
        s++;
    }
}

int remove_word(char *s, int index) {
    while(s[index - 1] == ' ') {
        remove_char(s, index);
        index--;
    }

    while(s[index - 1] != ' ' && s[index - 1]) {
        remove_char(s, index - 1);
        index--;
    }

    return index;
}

void read_user_input(io_win_t *io_win) {
    wmove(io_win->win, 0, 0);
    memset(io_win->data, 0, 128);
    werase(io_win->win);

    curs_set(1);
    int index = 0;
    while(1) {
        int ch = wgetch(io_win->win);
        if(ch == 10) 
            break;

        if(ch == KEY_RIGHT) 
            index++;
        else if(ch == KEY_LEFT)
            index--;
        else if(ch >= 32 && ch <= 127) {
            put_char(io_win->data, ch, index);
            index++;
        } else if(ch == KEY_BACKSPACE && index > 0) {
            remove_char(io_win->data, index - 1);
            index--;
        } else if(ch == KEY_DC) 
            remove_char(io_win->data, index);
        else if(ch == 23) {
            index = remove_word(io_win->data, index);
        } else if(ch == 27) {
            index = 0;

            memset(io_win->data, 0, 128);

            werase(io_win->win);
            waddstr(io_win->win, io_win->data);
            wmove(io_win->win, 0, index);

            break;
        }

        werase(io_win->win);
        waddstr(io_win->win, io_win->data);
        wmove(io_win->win, 0, index);
    }

    curs_set(0);
}

void push_msg(io_win_t *io_win, const char *msg) {
    werase(io_win->win);
    waddstr(io_win->win, msg);
    wrefresh(io_win->win);
}

int form_window_2_opt(const char *msg, const char **opts, int height, int width) {
    int ymax, xmax;

    getmaxyx(stdscr, ymax, xmax);

    WINDOW *form_win = newwin(height, width, (ymax - height) / 2, (xmax - width) / 2);


    int selection_index = 0;
    int ch;
    const char *msg_temp;
    while(1) {
        msg_temp = msg;
        wclear(form_win);
        box(form_win, 0, 0);

        int msg_len = strlen(msg_temp);

        int i = 0;
        do {
            mvwaddnstr(form_win, i + 1, 1, msg_temp, width - 2);
            msg_len = msg_len - width - 2;
            msg_temp += width - 2;
            i++;
        } while(msg_len > 0);

        for(int j = 0; j < 2; j++) {
            if(selection_index == j) {
                wattron(form_win, A_REVERSE);
                mvwprintw(form_win, j + i + 2, 3, "%s", opts[j]);
                wattroff(form_win, A_REVERSE);
            } else {
                mvwprintw(form_win, j + i + 2, 3, "%s", opts[j]);
            }
        }

        wrefresh(form_win);

        ch = wgetch(form_win);

        if(ch == 'j' && selection_index < 1) {
            selection_index++;
        } else if(ch == 'k' && selection_index > 0) {
            selection_index--;
        } else if(ch == 10) {
            break;
        } else if(ch == 27) {
            selection_index = 1;
            break;
        }
    }

    wclear(form_win);
    wrefresh(form_win);
    delwin(form_win);
    return selection_index;
}