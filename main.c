#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <ncurses.h>
#include <s2n.h>
#include <unistd.h>

#include "network.h"
#include "parser.h"
#include "utils.h"
#include "ui.h"

char *open_file(const char *file_path) {
    FILE *file = fopen(file_path, "r");
    if(!file)
        return NULL;

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *str = malloc(length + 1);
    fread(str, 1, length, file);
    str[length] = '\0';

    fclose(file);

    return str;
}

char *fetch_response(connection_handle_t *connection_handle, uri_t *uri) {
    char request[129] = {0};
    int request_len;

    request_len = strlen(uri->protocol) + strlen(uri->hostname) + strlen(uri->path) + 6;

    snprintf(request, request_len, "%s://%s%s\r\n", uri->protocol, uri->hostname, uri->path);

    int status = connection_establish(connection_handle, uri->hostname);

    if(status == S2N_FAILURE)
        return NULL;

    connection_send(connection_handle, request, request_len);

    char *response = connection_receive(connection_handle);

    connection_wipe(connection_handle);

    return response;
}

void gemini_parse(page_t *page, char *gemtext_str) {
    page->data = gemtext_str;

    normalize_newlines(page->data);
    gemtext_parse(page->contents, page->data);
}

void clear_forward_pages(page_t *page) {
    page_t *next_page;

    if(!page)
        return;

    while(1) {
        next_page = page->next_page;

        if(!next_page) {
            page_free(page);
            return;
        }

        page_free(page);

        page = next_page;
    }
}

int main() {
    atexit((void *)endwin);
    setlocale(LC_ALL, "");

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    set_escdelay(25);
    curs_set(0);

    int term_height;
    int term_width;

    getmaxyx(stdscr, term_height, term_width);

    io_win_t io_win = {0};
    io_win.win = newwin(1, term_width, term_height - 1, 0);
    keypad(io_win.win, TRUE);

const char *welcome_page_str_backup =   "```_____                                __\n"
                                        "|  __ \\                              / _|\n"
                                        "| |  \\/ ___ _ __ ___  ___ _   _ _ __| |_\n"
                                        "| | __ / _ \\ '_ ` _ \\/ __| | | | '__|  _|\n"
                                        "| |_\\ \\  __/ | | | | \\__ \\ |_| | |  | |\n"
                                        " \\____/\\___|_| |_| |_|___/\\__,_|_|  |_|\n"
                                        "```\n"
                                        "\n"
                                        "> Welcome to the Gemsurf Client.\n"
                                        "\n"
                                        "### ⌨️ Navigation\n"
                                        "`TAB` Select | `j/k` Scroll | `b/n` Prev/Next | `/` Search | `q` Quit\n"
                                        "\n"
                                        "---\n"
                                        "\n"
                                        "### 🌟 Featured\n"
                                        "=> gemini://cdg.thegonz.net/ Collaborative Directory of Geminispace\n"
                                        "\n"
                                        "";

    page_t *current_page = page_new();
    char *welcome_page_str = open_file(NULL); // proper program files folder not implemented yet

    if(!welcome_page_str)
        welcome_page_str = strdup(welcome_page_str_backup);

    gemini_parse(current_page, welcome_page_str);
    page_t *first_page = current_page;

    connection_handle_t connection_handle;
    connection_initialize(&connection_handle);

    char uri_str[128] = {0};
    
    char request[129] = {0};
    int request_len;

    char *response;

    while(1) {
        if(current_page != first_page) {
            sprintf(uri_str, "%s://%s%s", current_page->uri->protocol, current_page->uri->hostname, current_page->uri->path);
            push_msg(&io_win, uri_str);
        }

        page_reload_contents(current_page);
        page_refresh(current_page);
        
        int ch = wgetch(io_win.win);
        switch(ch) {
            case 'j':
            if(current_page->ui_state == VIEW && current_page->line_index + term_height - 1 < current_page->last_line)
                current_page->line_index++;
            if(current_page->ui_state == SELECTION && current_page->selection_index < current_page->contents->link_count - 1) 
                current_page->selection_index++;
            break;
            case 'k':
            if(current_page->ui_state == VIEW && current_page->line_index > 0) 
                current_page->line_index--;
            if(current_page->ui_state == SELECTION && current_page->selection_index > 0) 
                current_page->selection_index--;
            break;
            case 'q':
            goto exit;
            case 'n':
            if(current_page->next_page != NULL)
                current_page = current_page->next_page;
            break;
            case 'b':
            if(current_page->prev_page != NULL)
                current_page = current_page->prev_page;
            break;
            case '\t':
            current_page->ui_state = SELECTION;
            set_nearest_selection_index(current_page);
            break;
            case 27:
            current_page->ui_state = VIEW;
            break;
            case 10:
            if(current_page->ui_state != SELECTION)
                break;

            page_t *buf = page_new();

            char *selected_uri;

            if(current_page->selected_line)
                selected_uri = parse_link_from_line(current_page->selected_line);               

            if(uri_type(selected_uri) == ABSOLUTE)
                uri_parse(buf->uri, selected_uri);
            else
                uri_parse_relative(buf->uri, selected_uri, current_page->uri);

            free(selected_uri);

            if(!strcmp(buf->uri->protocol, "file")) {
               char file_path[129];
               uri_to_file_path(buf->uri, file_path); 

               char *file_contents = open_file(file_path);

               gemini_parse(buf, file_contents);

               buf->next_page = NULL;
               buf->prev_page = current_page;

               current_page->next_page = buf;

               current_page = current_page->next_page;

               break;
            }

            if(strcmp(buf->uri->protocol, "gemini")) {
                page_free(buf);
                push_msg(&io_win, "Protocol isn't supported");
                sleep(1);

                break;
            }

            char *response = fetch_response(&connection_handle, buf->uri);
            if(!response) {
                page_free(buf);
                push_msg(&io_win, "Can't connect to the host");

                break;
            }

            gemini_parse(buf, response);

            clear_forward_pages(current_page->next_page);

            buf->next_page = NULL;
            buf->prev_page = current_page;

            current_page->next_page = buf;

            current_page = current_page->next_page;

            break;
            case '/':
            read_user_input(&io_win);
            if(!*io_win.data)
                break;

            buf = page_new();

            uri_normalize_end(io_win.data);
            uri_parse(buf->uri, io_win.data);

            if(!strcmp(buf->uri->protocol, "file")) {
               char file_path[129];
               uri_to_file_path(buf->uri, file_path); 

               char *file_contents = open_file(file_path);

               gemini_parse(buf, file_contents);

               buf->next_page = NULL;
               buf->prev_page = current_page;

               current_page->next_page = buf;

               current_page = current_page->next_page;

               break;
            }

            if(strcmp(buf->uri->protocol, "gemini")) {
                page_free(buf);
                push_msg(&io_win, "Protocol isn't supported");

                break;
            }

            response = fetch_response(&connection_handle, buf->uri);
            if(!response) {
                page_free(buf);
                push_msg(&io_win, "Can't connect to the host");

                break;
            }

            gemini_parse(buf, response);

            clear_forward_pages(current_page->next_page);

            buf->next_page = NULL;
            buf->prev_page = current_page;

            current_page->next_page = buf;

            current_page = current_page->next_page;

            break;
        }
    }
exit:
    connection_cleanup(&connection_handle);

    clear_forward_pages(first_page->next_page);

    page_free(first_page); // free the first page


    delwin(io_win.win);

    return 0;
}