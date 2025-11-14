#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>
#include <ncurses.h>

#include "client2.h" 

#define MAX_CHAT_LINES 100

WINDOW *game_win, *chat_win, *write_win;
//char chat_history[MAX_CHAT_LINES][BUF_SIZE];
//int chat_count = 0;

void init_ui() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int height = LINES;
    int width = COLS;
    int game_h = height * 6 / 12;
    int chat_h = height * 4 / 12;
    int write_h = height - game_h - chat_h;

    game_win = newwin(game_h, width, 0, 0);
    chat_win = newwin(chat_h, width, game_h, 0);
    write_win = newwin(write_h, width, game_h + chat_h, 0);

    // Enable true scrolling
    scrollok(chat_win, TRUE);
    idlok(chat_win, TRUE);

    box(game_win, 0, 0);
    box(chat_win, 0, 0);
    box(write_win, 0, 0);

    mvwprintw(game_win, 0, (width - 13) / 2, " AWALE-GAME ");
    mvwprintw(chat_win, 0, (width - 6) / 2, " CHAT ");
    mvwprintw(write_win, 1, 1, "> ");
    wmove(write_win, 1, 3);

    wrefresh(game_win);
    wrefresh(chat_win);
    wrefresh(write_win);
}

void chat_append(const char *msg) {
    int maxy = getmaxy(chat_win);

    // Move cursor to the bottom line
    wmove(chat_win, maxy - 2, 1);

    // Print the line; ncurses scrolls automatically
    wprintw(chat_win, "%s\n", msg);

    // Redraw border + title
    box(chat_win, 0, 0);
    mvwprintw(chat_win, 0, (COLS - 6) / 2, " CHAT ");

    wrefresh(chat_win);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s [server_address] [pseudo]", argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_addr = argv[1];
    const char *nick = argv[2];

    SOCKET sock = init_connection(server_addr);
    if (sock < 0) {
        fprintf(stderr, "No se pudo conectar a %s", server_addr);
        return EXIT_FAILURE;
    }

    char buffer[BUF_SIZE];
    memset(buffer, 0, sizeof(buffer));

    init_ui();
    mvwprintw(game_win, 1, 2, "Connecté au serveur Awale comme %s", argv[2]);
    wrefresh(game_win);

    write_server(sock, nick);

    fd_set rdfs;

    char input[BUF_SIZE];
    while (1) {
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);
        int maxfd = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;

        int ret = select(maxfd + 1, &rdfs, NULL, NULL, NULL);
        if (ret == -1) {
            perror("select");
            break;
        }

        // mensaje del servidor
        if (FD_ISSET(sock, &rdfs)) {
            int n = read_server(sock, buffer);
            if (n <= 0) {
                closesocket(sock);   
                break;
            }
            if (strncmp(buffer, "CHAT:", 5) == 0) {
                chat_append(buffer + 5);
            }
 
            
            else {
                // actualizar ventana de juego
                werase(game_win);
                box(game_win, 0, 0);
                mvwprintw(game_win, 0, (COLS - 13) / 2, " AWALE-GAME ");
                mvwprintw(game_win, 1, 2, "%s", buffer);
                wrefresh(game_win);
                mvwprintw(write_win, 1, 1, "                                                  "); 
                mvwprintw(write_win, 1, 1, "> "); 
                wmove(write_win, 1, 3); 
                wrefresh(write_win);
                
            }
            buffer[n] = '\0';
        }

        // entrada del usuario
        if (FD_ISSET(STDIN_FILENO, &rdfs)) {

            
            werase(write_win);
            box(write_win, 0, 0);
            mvwprintw(write_win, 0, (COLS - 6) / 2, " >");
            wrefresh(write_win);

            echo();
            mvwgetnstr(write_win, 1, 2, input, BUF_SIZE - 1);
            noecho();

            if (strcmp(input, "/quit") == 0) {
                write_server(sock, "/quit");
                break;
            }

            if (strlen(input) > 0){
            write_server(sock, input);
                if (strncmp(input, "/chat", 5) == 0 ){
                continue;
                //char local[BUF_SIZE];
                //snprintf(local, sizeof(local), "[%s]: %s", argv[2], input);
                //add_chat_message(local);
                //refresh_chat_window();
                }
                else{
                mvwprintw(write_win, 1, 1, "                                                  "); 
                mvwprintw(write_win, 1, 1, "> "); 
                wmove(write_win, 1, 3); 
                wrefresh(write_win);}
            }
            
            
        }
    }

    closesocket(sock);
    return EXIT_SUCCESS;
}

