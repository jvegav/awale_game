#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "server2.h"
#include "awale_logic.h"


static GameRoom rooms[MAX_GAMES];
static Client clients[MAX_CLIENTS];
static int listen_sock = -1;


/* Helpers */
static int find_free_room(void) {
    for (int i = 0; i < MAX_GAMES; ++i) if (!rooms[i].active) return i;
    return -1;
}


static int find_room_by_id(int id) {
    for (int i = 0; i < MAX_GAMES; ++i) if (rooms[i].active && rooms[i].id == id) return i;
    return -1;
}


static int find_conn_index_by_sock(int sock) {
    for (int i = 0; i < MAX_CLIENTS; ++i) if (clients[i].connected && clients[i].sock == sock) return i;
    return -1;
}


static int register_new_conn(int sock, const char *name) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients[i].connected) {
            clients[i].connected = 1;
            clients[i].sock = sock;
            strncpy(clients[i].name, name ? name : "Anon", NAME_LEN-1);
            clients[i].name[NAME_LEN-1] = '\n';
            clients[i].room_id = -1;
            clients[i].role = ROLE_NONE;
            clients[i].in_play_mode = 0;
            return i;
        }
    }
    return -1;
}



static void unregister_conn_by_index(int idx) {
    if (idx < 0 || idx >= MAX_CLIENTS) return;
    clients[idx].connected = 0;
    clients[idx].sock = -1;
    clients[idx].room_id = -1;
    clients[idx].role = ROLE_NONE;
}

static void broadcast_to_room(GameRoom *r, const char *msg) {
    for (int i = 0; i < r->player_count; ++i) {
        if (r->players[i].sock > 0) write_client(r->players[i].sock, msg);
    }
    for (int i = 0; i < r->spec_count; ++i) {
        if (r->spectators[i].sock > 0) write_client(r->spectators[i].sock, msg);
    }
}


static void close_room(int ridx) {
    GameRoom *r = &rooms[ridx];
    // notify
    broadcast_to_room(r, "The room has ended.\n");
    // mark as free
    r->active = 0;
    r->player_count = 0;
    r->spec_count = 0;
}


void handle_leave(int conn_idx) {

    if (!clients[conn_idx].connected) return;

    int room_id = clients[conn_idx].room_id;
    if (room_id == -1) {
        write_client(clients[conn_idx].sock, "You are not in any room.\n");
        return;
    }

    int ridx = find_room_by_id(room_id);
    if (ridx == -1) {
        clients[conn_idx].room_id = -1;
        write_client(clients[conn_idx].sock, "The room no longer exists.\n");
        return;
    }

    GameRoom *r = &rooms[ridx];

    /* ---- If was a player ---- */
    for (int p = 0; p < r->player_count; ++p) {
        if (r->players[p].sock == clients[conn_idx].sock) {


            // Notify the other player and spectators with winner announcement
            if (r->player_count == 2) {
                int loser = p;     // p is the one leaving
                int winner = (p == 0 ? 1 : 0);  // index of remaining player

                char msg[256];
                snprintf(msg, sizeof(msg),
                    "You have left the match.\n=== GAME OVER ===\nPlayer %d has left.\nPlayer %d is the WINNER!\n",
                    (loser + 1), (winner + 1));

                // Send message to *everyone* in the room
                broadcast_to_room(r, msg);
            } else {
                // Solo había un jugador (no rival)
                write_client(clients[conn_idx].sock,
                    "You left the match. The room is now closed.\n");
            }

            

            close_room(ridx);

            clients[conn_idx].room_id = -1;
            clients[conn_idx].in_play_mode = 0;
            return;
        }
    }

    /* ---- If was a spectator ---- */
    for (int sp = 0; sp < r->spec_count; ++sp) {
        if (r->spectators[sp].sock == clients[conn_idx].sock) {

            write_client(clients[conn_idx].sock, "You have stopped watching the room.\n");

            // Remove from array
            for (int k = sp; k < r->spec_count - 1; ++k)
                r->spectators[k] = r->spectators[k + 1];

            r->spec_count--;
            clients[conn_idx].room_id = -1;
            return;
        }
    }

    // Strange case: was in room but not as player nor spectator
    clients[conn_idx].room_id = -1;
    write_client(clients[conn_idx].sock, "You have left the room.\n");
}




/* Create a room and assign creator as player1 */
static int handle_create_game(int conn_idx) {
    int ridx = find_free_room();
    if (ridx == -1) return -1;

    clients[conn_idx].in_play_mode = 1;
    GameRoom *r = &rooms[ridx];
    r->active = 1;
    r->id = ridx + 1; // human-friendly id
    r->player_count = 1;
    r->players[0] = (Client){ .sock = clients[conn_idx].sock };
    strncpy(r->players[0].name, clients[conn_idx].name, sizeof(r->players[0].name)-1);
    r->players[0].name[sizeof(r->players[0].name)-1] = '\n';
    r->spec_count = 0;
    r->score1 = r->score2 = 0;
    r->turn = 1;
    init_game(r->matrix);


    clients[conn_idx].room_id = r->id;
    clients[conn_idx].role = ROLE_PLAYER1;


    char reply[128];
    snprintf(reply, sizeof(reply), "Room created. ID: %d\n", r->id);
    write_client(clients[conn_idx].sock, reply);
   write_client(clients[conn_idx].sock,
        "\nRoom created. Waiting for another player to start...\n"
        "To leave the room: /leave\n");
    return r->id;
}

static void send_board_to_room(GameRoom *r) {
    char base_header[256];
    snprintf(base_header, sizeof(base_header),
        "----------------------------------------\n"
        "SCORE\n Player 1: %d | Player 2: %d\n"
        "----------------------------------------\n",
        r->score1, r->score2
    );

    char board[1024];   
    char msg[2048];     

   
    if (r->player_count >= 1) {
        memset(board, 0, sizeof(board));
        matrix_to_string_joueur1(r->matrix, board, sizeof(board));

        memset(msg, 0, sizeof(msg));
        snprintf(msg, sizeof(msg), "%s%s%s",
                 base_header,
                 board,
                 (r->turn == 1) ? "It's YOUR turn! Choose a column (1-6):\n"
                                : "Waiting... It's Player 2's turn.\n");

        write_client(r->players[0].sock, msg);
    }

    // --- Player 2 ---
    if (r->player_count == 2) {
        memset(board, 0, sizeof(board));
        matrix_to_string_joueur2(r->matrix, board, sizeof(board));

        memset(msg, 0, sizeof(msg));
        snprintf(msg, sizeof(msg), "%s%s%s",
                 base_header,
                 board,
                 (r->turn == 2) ? "It's YOUR turn! Choose a column (1-6):\n"
                                : "Waiting... It's Player 1's turn.\n");

        write_client(r->players[1].sock, msg);
    }

    // --- Spectators ---
    if (r->spec_count > 0) {
        memset(board, 0, sizeof(board));
        matrix_to_string_joueur1(r->matrix, board, sizeof(board)); // neutral view

        memset(msg, 0, sizeof(msg));
        snprintf(msg, sizeof(msg), "%s%s\n(Spectator mode)\n",
                 base_header, board);

        for (int i = 0; i < r->spec_count; i++) {
            write_client(r->spectators[i].sock, msg);
        }
    }
}



static int handle_join_game(int conn_idx, int id) {
    int idx = find_room_by_id(id);
    if (idx == -1) {
        write_client(clients[conn_idx].sock, "Room not found.\n");
        return -1;
    }

    GameRoom *r = &rooms[idx];
    if (r->player_count >= 2) {
        write_client(clients[conn_idx].sock, "Room already has 2 players.\n");
        return -1;
    }
    clients[conn_idx].in_play_mode = 1;

    // Add player 2
    r->players[1] = (Client){ .sock = clients[conn_idx].sock };
    strncpy(r->players[1].name, clients[conn_idx].name, sizeof(r->players[1].name)-1);
    r->players[1].name[sizeof(r->players[1].name)-1] = '\0';
    r->player_count = 2;
    r->active = 2; // playing

    clients[conn_idx].room_id = r->id;
    clients[conn_idx].role = ROLE_PLAYER2;

    // Enable in_play_mode for both players
    for (int k = 0; k < r->player_count; k++) {
        int idxc = find_conn_index_by_sock(r->players[k].sock);
        if (idxc != -1) clients[idxc].in_play_mode = 1;
    }

    // Notify both players
    char msg[256];
    snprintf(msg, sizeof(msg), "Player %.32s has joined. The match is starting.\n", clients[conn_idx].name);
    write_client(r->players[0].sock, msg);
    write_client(r->players[1].sock, msg);

    // Start message
    for (int k = 0; k < 2; k++) {
        write_client(r->players[k].sock,
            "=== THE MATCH BEGINS ===\n"
            "To leave the match and return to the menu: /q\n");
    }

    // Initialize turn to player 1
    r->turn = 1;

    // Send initial board
    send_board_to_room(r);

    return r->id;
}


static int handle_watch(int conn_idx, int id) {
    int idx = find_room_by_id(id);
    if (idx == -1) {
        write_client(clients[conn_idx].sock, "Room not found.\n");
        return -1;
    }
    GameRoom *r = &rooms[idx];
    if (r->spec_count >= MAX_SPECTATORS) {
        write_client(clients[conn_idx].sock, "Room is full of spectators.\n");
        return -1;
    }
    clients[conn_idx].in_play_mode = 1;
    r->spectators[r->spec_count] = (Client){ .sock = clients[conn_idx].sock };
    strncpy(r->spectators[r->spec_count].name, clients[conn_idx].name, sizeof(r->spectators[r->spec_count].name)-1);
    r->spectators[r->spec_count].name[sizeof(r->spectators[r->spec_count].name)-1] = '\n';
    r->spec_count++;


    clients[conn_idx].room_id = r->id;
    clients[conn_idx].role = ROLE_SPECTATOR;


    write_client(clients[conn_idx].sock, "You have joined as a spectator.\n");
    send_board_to_room(r);
    return r->id;
}

static int player_name_invalid(const char *name) {
    if (name == NULL) return 1;
    size_t L = strlen(name);
    if (L == 0 || L >= NAME_LEN) return 1;
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].connected && strcmp(clients[i].name, name) == 0) return 1;
    }
    return 0;
}

static void process_text_message(int conn_idx, const char *txt) {

    char line[BUF_SIZE];
    memset(line, 0, sizeof(line));
    strncpy(line, txt, sizeof(line)-1);
    line[sizeof(line)-1] = '\0';       

    while (*line == ' ') memmove(line, line+1, strlen(line));

    if (*line == '\0' || *line == '\n') return;

    if(clients[conn_idx].in_play_mode == 1){

        int rid = clients[conn_idx].room_id;
        int rindex = find_room_by_id(rid);
        if (rindex == -1) { write_client(clients[conn_idx].sock, "Room not found.\n"); return; }

        GameRoom *r = &rooms[rindex];

        
        if (strncmp(line, "/leave", 6) == 0 || strncmp(line, "/q", 2) == 0) {
            handle_leave(conn_idx);
            int other_idx_sock = r->players[0].sock == clients[conn_idx].sock ? r->players[1].sock : r->players[0].sock;
            int other_idx = find_conn_index_by_sock(other_idx_sock);
            
            if (other_idx != -1) clients[other_idx].in_play_mode = 0;
            
            write_client(clients[conn_idx].sock, "You have returned to the menu.\n");
            write_client(clients[other_idx].sock, "You have returned to the menu.\n");
            return;
        }
        
        if (strncmp(line, "/chat", 5) == 0 ){
            //handle_leave(conn_idx);
            int other_idx_sock = r->players[0].sock == clients[conn_idx].sock ? r->players[1].sock : r->players[0].sock;
            int other_idx = find_conn_index_by_sock(other_idx_sock);
            
            //if (other_idx != -1) clients[other_idx].in_play_mode = 0;
            
            char chat_msg[BUF_SIZE + 64];
            char *msg_text = line + 5;
            
            if (clients[conn_idx].role != ROLE_PLAYER1 && clients[conn_idx].role != ROLE_PLAYER2) {
            write_client(clients[conn_idx].sock, "You are a spectator. You cannot write in the chat but you can read.\n");
            return;
            }

            snprintf(chat_msg, sizeof(chat_msg), "CHAT:[%s]: %s\n", clients[conn_idx].name, msg_text);
            write_client(clients[conn_idx].sock, chat_msg);
            write_client(clients[other_idx].sock, chat_msg);

            for (int i = 0; i < r->spec_count; i++) {
            write_client(r->spectators[i].sock, chat_msg);
        }
            return;
        }

        if (r->player_count < 2) {
            write_client(clients[conn_idx].sock,
                "You are the only player in the room.\n"
                "Please wait for another player to join, or use /leave or /q to exit the match.\n");
            return;
        }
        
        if (line[0] < '1' || line[0] > '6' || line[1] != '\0') {
            write_client(clients[conn_idx].sock,
                 "You can only play numbers 1 to 6. To leave, use /leave or /q.\n");
            return;
        }

        // ---- MOVES ----
        
        if (rid == -1) {
            write_client(clients[conn_idx].sock, "You are not in any room.\n");
            return;
        }

        

        if (clients[conn_idx].role != ROLE_PLAYER1 && clients[conn_idx].role != ROLE_PLAYER2) {
            write_client(clients[conn_idx].sock, "You are a spectator. You cannot play.\n");
            return;
        }
        

        int player_num = (clients[conn_idx].role == ROLE_PLAYER1) ? 1 : 2;
        if (r->turn != player_num) {
            write_client(clients[conn_idx].sock, "It's not your turn. Please wait.\n");
            return;
        }
        int move = atoi(line);
        if (!process_move(r->matrix, player_num, move, &r->score1, &r->score2)) {
            write_client(clients[conn_idx].sock, "Illegal move according to rules.\n");
            return;
        }

        // change turn
        r->turn = (r->turn == 1) ? 2 : 1;
        send_board_to_room(r);

        if (!can_play(r->matrix)) {
            char endmsg[256];
            snprintf(endmsg, sizeof(endmsg),
                    "=== GAME OVER === Score P1: %d | Score P2: %d\n",
                    r->score1, r->score2);
            broadcast_to_room(r, endmsg);
            close_room(rindex);
        }

    }

    else if (line[0] == '/') {

        if (strncmp(line, "/create_game", 12) == 0) {
            handle_create_game(conn_idx);
            clients[conn_idx].in_play_mode = 1;
            return;
        }
        else if (strncmp(line, "/join_game", 10) == 0) {
            int id = atoi(line + 11);
            handle_join_game(conn_idx, id);
            return;
        }
        else if (strncmp(line, "/watch", 6) == 0) {
            int id = atoi(line + 7);
            handle_watch(conn_idx, id);
            clients[conn_idx].in_play_mode = 1;
            clients[conn_idx].role != ROLE_SPECTATOR;
            return;
        }
        
        else if (strncmp(line, "/list", 5) == 0) {
            char buf[1024];
            int off = 0;
            off += snprintf(buf + off, sizeof(buf) - off, "Active rooms:\n");
            for (int i = 0; i < MAX_GAMES; ++i)
                if (rooms[i].active)
                    off += snprintf(buf + off, sizeof(buf) - off,
                                    "ID %d - Players: %d - Spectators: %d\n",
                                    rooms[i].id, rooms[i].player_count, rooms[i].spec_count);
            write_client(clients[conn_idx].sock, buf);
            return;
        }
        else if (strncmp(line, "/users", 6) == 0) {
            char list[1024];
            int off = 0;
            off += snprintf(list + off, sizeof(list) - off, "Online users:\n");
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (clients[i].connected) {
                    off += snprintf(list + off, sizeof(list) - off, "- %s\n", clients[i].name);
                }
            }
            write_client(clients[conn_idx].sock, list);
            return;
        }
        else if (strncmp(line, "/help", 6) == 0) {
            char list[1024];
            int off = 0;
            off += snprintf(list + off, sizeof(list) - off, "Available commands:\n");
            off += snprintf(list + off, sizeof(list) - off, "/create_game    - Create a new room and become Player 1.\n");
            off += snprintf(list + off, sizeof(list) - off, "/join_game <id> - Join the room with ID <id> as Player 2.\n");
            off += snprintf(list + off, sizeof(list) - off, "/watch <id>     - Join as a spectator (you cannot play).\n");
            off += snprintf(list + off, sizeof(list) - off, "/list           - Show active rooms (ID, players, spectators).\n");
            off += snprintf(list + off, sizeof(list) - off, "/users          - List connected users.\n");
            off += snprintf(list + off, sizeof(list) - off, "/chat           - View the room chat history and enter chat mode.\n");
            off += snprintf(list + off, sizeof(list) - off, "/help           - Show this help with command descriptions.\n");
            write_client(clients[conn_idx].sock, list);
            return;
        }
        else if (strncmp(line, "/chat", 5) == 0) {
            int rid = clients[conn_idx].room_id;
            if (rid == -1) {
                write_client(clients[conn_idx].sock, "You are not in a room.\n");
                return;
            }

            int rindex = find_room_by_id(rid);
            if (rindex == -1) {
                write_client(clients[conn_idx].sock, "Room not found.\n");
                return;
            }

            
    }
    }

    else {
            write_client(clients[conn_idx].sock,
                "Unknown command. \n If you want to know the commands type : /help\n");
            return;
    }
}





int main(void) {

    // initialize arrays for games and clients
    for (int i = 0; i < MAX_GAMES; ++i) rooms[i].active = 0;
    for (int i = 0; i < MAX_CLIENTS; ++i) clients[i].connected = 0;

    // START CONNECTION
    listen_sock = init_connection();
    if (listen_sock < 0) { 
        perror("init_connection"); return EXIT_FAILURE; 
    }


    printf("Awale server waiting...\n");

    fd_set readfds;
    int maxfd = listen_sock;

    while (1) {

        FD_ZERO(&readfds);
        FD_SET(listen_sock, &readfds);

        // Add clients to set
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i].connected) {
                FD_SET(clients[i].sock, &readfds);
                if (clients[i].sock > maxfd)
                    maxfd = clients[i].sock;
            }
        }

        int ret = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (ret == -1) {
            perror("select");
            break;
        }

        /* ---- NEW CONNECTION ---- */
        if (FD_ISSET(listen_sock, &readfds)) {

            struct sockaddr_in csin;
            socklen_t sinsize = sizeof(csin);
            int csock = accept(listen_sock, (struct sockaddr *)&csin, &sinsize);

            if (csock == -1) {
                perror("accept");
            }
            else {
                char namebuf[NAME_LEN];
                int n = read_client(csock, namebuf);

                if (player_name_invalid(namebuf))
                {
                    int code = rand() % 9000 + 1000;
                    char temp[1024]; 
                    snprintf(temp, sizeof(temp), "%s_%d", namebuf, code);
                    strncpy(namebuf, temp, sizeof(namebuf) - 1);
                    namebuf[sizeof(namebuf) - 1] = '\0';
                }
                


                if (n <= 0 || player_name_invalid(namebuf)) {
                    closesocket(csock);
                }
                else {
                    int idx = register_new_conn(csock, namebuf);

                    if (idx == -1) {
                        write_client(csock, "Server full.\n");
                        closesocket(csock);
                    }
                    else {
                        char welcome[1024];
                        int off = 0;

                        off += snprintf(welcome + off, sizeof(welcome) - off,
                            "Welcome %s!\nAvailable commands:\n", namebuf);

                        off += snprintf(welcome + off, sizeof(welcome) - off,
                            "/create_game    - Create a new room and become Player 1.\n");
                        off += snprintf(welcome + off, sizeof(welcome) - off,
                            "/join_game <id> - Join the room with ID <id> as Player 2.\n");
                        off += snprintf(welcome + off, sizeof(welcome) - off,
                            "/watch <id>     - Join as a spectator (you cannot play).\n");
                        off += snprintf(welcome + off, sizeof(welcome) - off,
                            "/list           - Show active rooms (ID, players, spectators).\n");
                        off += snprintf(welcome + off, sizeof(welcome) - off,
                            "/users          - List connected users.\n");
        
                        off += snprintf(welcome + off, sizeof(welcome) - off,
                            "/help           - Show this help with command descriptions.\n");

                        write_client(csock, welcome);
                        printf("New client connected: %s (sock=%d)\n", namebuf, csock);
                    }
                }
            }
        }

        /* ---- MESSAGES FROM EXISTING CLIENTS ---- */
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (!clients[i].connected) continue;

            int s = clients[i].sock;

            if (FD_ISSET(s, &readfds)) {

                char buf[BUF_SIZE];
                int n = read_client(s, buf);

                // Client disconnected
                if (n <= 0) {
                    printf("Client disconnected: %s\n", clients[i].name);

                    // If belonged to a room -> handle leaving
                    if (clients[i].room_id != -1) {
                        int ridx = find_room_by_id(clients[i].room_id);

                        if (ridx != -1) {
                            GameRoom *r = &rooms[ridx];

                            // If was a player
                            for (int p = 0; p < r->player_count; ++p) {
                                if (r->players[p].sock == s) {
                                    if (r->player_count == 2) {
                                        int other_sock = r->players[1 - p].sock;
                                        int other_idx = find_conn_index_by_sock(other_sock);
                                        clients[other_idx].in_play_mode = 0;
                            
                                        write_client(other_sock,
                                            "Your opponent has disconnected. The room will close.\n");
                                    }
                                    close_room(ridx);
                                    break;
                                }
                            }

                            // If was a spectator
                            for (int sp = 0; sp < r->spec_count; ++sp) {
                                if (r->spectators[sp].sock == s) {
                                    for (int k = sp; k < r->spec_count - 1; ++k)
                                        r->spectators[k] = r->spectators[k + 1];
                                    r->spec_count--;
                                    break;
                                }
                            }
                        }
                    }

                    closesocket(s);
                    unregister_conn_by_index(i);
                }
                else { // Client sent text
                    buf[n] = '\0'; 
                    process_text_message(i, buf);
                }
            }
        }
    }



    end_connection(listen_sock);
    return EXIT_SUCCESS;
}
