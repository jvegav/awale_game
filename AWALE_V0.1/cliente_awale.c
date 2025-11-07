#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <ncurses.h>
#include "client2.h"

#define MAX_CHAT_LINES 100

WINDOW *game_win, *chat_win;
char chat_history[MAX_CHAT_LINES][BUF_SIZE];
int chat_count = 0;

/* --- UI Setup --- */
void init_ui() {
    initscr();            // start ncurses mode
    cbreak();             // disable line buffering
    noecho();             // don’t echo input automatically
    keypad(stdscr, TRUE); // enable arrow keys

    int height = LINES;
    int width = COLS;
    int game_h = height * 2 / 3;     // top 2/3 of screen
    int chat_h = height - game_h;    // bottom 1/3

    game_win = newwin(game_h, width, 0, 0);
    chat_win = newwin(chat_h, width, game_h, 0);

    scrollok(chat_win, TRUE);
    box(game_win, 0, 0);
    box(chat_win, 0, 0);

    mvwprintw(game_win, 0, (width - 13) / 2, " AWALE-GAME ");
    mvwprintw(chat_win, 0, (width - 6) / 2, " CHAT ");

    wrefresh(game_win);
    wrefresh(chat_win);
}

/* --- Chat Management --- */
void add_chat_message(const char *msg) {
    if (chat_count < MAX_CHAT_LINES) {
        strncpy(chat_history[chat_count++], msg, BUF_SIZE - 1);
    } else {
        // scroll up if full
        for (int i = 1; i < MAX_CHAT_LINES; i++)
            strcpy(chat_history[i - 1], chat_history[i]);
        strncpy(chat_history[MAX_CHAT_LINES - 1], msg, BUF_SIZE - 1);
    }
}

void refresh_chat_window() {
    werase(chat_win);
    box(chat_win, 0, 0);
    mvwprintw(chat_win, 0, (COLS - 6) / 2, " CHAT ");
    int start = chat_count > 10 ? chat_count - 10 : 0;
    int y = 1;
    for (int i = start; i < chat_count; i++)
        mvwprintw(chat_win, y++, 2, "%s", chat_history[i]);
    wrefresh(chat_win);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s [adresse] [pseudo]\n", argv[0]);
        return EXIT_FAILURE;
    }

    SOCKET sock = init_connection(argv[1]);
    write_server(sock, argv[2]); // send name to server
    char buffer[BUF_SIZE];

    init_ui();
    mvwprintw(game_win, 1, 2, "Connecté au serveur Awale comme %s", argv[2]);
    wrefresh(game_win);

    fd_set rdfs;
    char input[BUF_SIZE] = "";

    while (1) {
        FD_ZERO(&rdfs);
        FD_SET(sock, &rdfs);
        FD_SET(STDIN_FILENO, &rdfs);

        if (select(sock + 1, &rdfs, NULL, NULL, NULL) == -1) {
            perror("select()");
            break;
        }

        /* --- Messages from server --- */
        if (FD_ISSET(sock, &rdfs)) {
            int n = read_server(sock, buffer);
            if (n == 0) {
                mvwprintw(game_win, 2, 2, "Serveur déconnecté.\n");
                wrefresh(game_win);
                break;
            }

           if (strncmp(buffer, "CHAT:", 5) == 0) {
                add_chat_message(buffer + 5);  // saltar el prefijo
                refresh_chat_window();
            } 
            else {
                // cualquier otro mensaje va al tablero
                werase(game_win);
                box(game_win, 0, 0);
                mvwprintw(game_win, 0, (COLS - 13) / 2, " AWALE-GAME ");
                mvwprintw(game_win, 1, 2, "%s", buffer);
                wrefresh(game_win);
            }

        }

        /* --- Input from player --- */
        if (FD_ISSET(STDIN_FILENO, &rdfs)) {
            echo();
            mvwgetnstr(chat_win, getmaxy(chat_win) - 2, 2, input, BUF_SIZE - 1);
            noecho();

            if (strlen(input) == 0)
                continue;

            if (isdigit(input[0])) {
                // player move
                write_server(sock, input);
            } else {
                // chat message
                char msg[BUF_SIZE];
                snprintf(msg, sizeof(msg), "#%s", input);
                write_server(sock, msg);

                char local[BUF_SIZE];
                snprintf(local, sizeof(local), "[%s]: %s", argv[2], input);
                add_chat_message(local);
                refresh_chat_window();
            }

            memset(input, 0, sizeof(input));
        }
    }

    endwin(); // close ncurses
    closesocket(sock);
    return EXIT_SUCCESS;
}
