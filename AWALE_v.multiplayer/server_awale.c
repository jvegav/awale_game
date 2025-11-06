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
    broadcast_to_room(r, "La sala ha terminado.");
    // mark as free
    r->active = 0;
    r->player_count = 0;
    r->spec_count = 0;
}


void handle_leave(int conn_idx) {

    if (!clients[conn_idx].connected) return;

    int room_id = clients[conn_idx].room_id;
    if (room_id == -1) {
        write_client(clients[conn_idx].sock, "No estás en ninguna sala.\n");
        return;
    }

    int ridx = find_room_by_id(room_id);
    if (ridx == -1) {
        clients[conn_idx].room_id = -1;
        write_client(clients[conn_idx].sock, "La sala ya no existe.\n");
        return;
    }

    GameRoom *r = &rooms[ridx];

    /* ---- Si era jugador ---- */
    for (int p = 0; p < r->player_count; ++p) {
        if (r->players[p].sock == clients[conn_idx].sock) {

            write_client(clients[conn_idx].sock, "Has salido de la partida.\n");

            // Notificamos al otro jugador
            if (r->player_count == 2) {
                int other = (p == 0 ? 1 : 0);
                write_client(r->players[other].sock,
                    "El otro jugador ha salido. La sala se cerrará.\n");
            }

            close_room(ridx);

            clients[conn_idx].room_id = -1;
            clients[conn_idx].in_play_mode = 0;
            return;
        }
    }

    /* ---- Si era espectador ---- */
    for (int sp = 0; sp < r->spec_count; ++sp) {
        if (r->spectators[sp].sock == clients[conn_idx].sock) {

            write_client(clients[conn_idx].sock, "Has dejado de observar la sala.\n");

            // Lo sacamos del array
            for (int k = sp; k < r->spec_count - 1; ++k)
                r->spectators[k] = r->spectators[k + 1];

            r->spec_count--;
            clients[conn_idx].room_id = -1;
            return;
        }
    }

    // Caso raro: estaba en sala pero no como player ni spectator
    clients[conn_idx].room_id = -1;
    write_client(clients[conn_idx].sock, "Has salido de la sala.\n");
}




/* Create a room and assign creator as player1 */
static int handle_create_game(int conn_idx) {
    int ridx = find_free_room();
    if (ridx == -1) return -1;


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
    snprintf(reply, sizeof(reply), "Sala creada. ID: %d \n", r->id);
    write_client(clients[conn_idx].sock, reply);
   write_client(clients[conn_idx].sock,
        "\nSala creada. Esperando a otro jugador para comenzar...\n"
        "Para salir de la sala: /leave\n");
    return r->id;
}

static void send_board_to_room(GameRoom *r) {
    char base_header[1024];
    char board[1024];

    // Header con puntajes
    snprintf(base_header, sizeof(base_header),
        "\n----------------------------------------\n"
        "SCORE\n Joueur 1: %d | Joueur 2: %d\n"
        "----------------------------------------\n",
        r->score1, r->score2
    );

    // --- Jugador 1 ---
    if (r->player_count >= 1) {
        char msg1[1200];
        msg1[0] = '\0';                  // limpiar buffer
        strcpy(msg1, base_header);
        matrix_to_string_joueur1(r->matrix, board);
        board[sizeof(board)-1] = '\0';   // asegurar null-termination
        strcat(msg1, board);

        if (r->turn == 1)
            strcat(msg1, "\nC'est TON tour! Choisis une colonne (1-6):\n");
        else
            strcat(msg1, "\nEn attente... C'est le tour du Joueur 2.\n");

        write_client(r->players[0].sock, msg1);
    }

    // --- Jugador 2 ---
    if (r->player_count == 2) {
        char msg2[1200];
        msg2[0] = '\0';
        strcpy(msg2, base_header);
        matrix_to_string_joueur2(r->matrix, board);
        board[sizeof(board)-1] = '\0';
        strcat(msg2, board);

        if (r->turn == 2)
            strcat(msg2, "\nC'est TON tour! Choisis une colonne (1-6):\n");
        else
            strcat(msg2, "\nEn attente... C'est le tour du Joueur 1.\n");

        write_client(r->players[1].sock, msg2);
    }

    // --- Espectadores ---
    if (r->spec_count > 0) {
        char specmsg[1200];
        specmsg[0] = '\0';
        strcpy(specmsg, base_header);
        matrix_to_string_joueur1(r->matrix, board); // vista neutral
        board[sizeof(board)-1] = '\0';
        strcat(specmsg, board);
        strcat(specmsg, "\n(Mode spectateur)\n");

        for (int i = 0; i < r->spec_count; i++)
            write_client(r->spectators[i].sock, specmsg);
    }
}




static int handle_join_game(int conn_idx, int id) {
    int idx = find_room_by_id(id);
    if (idx == -1) {
        write_client(clients[conn_idx].sock, "Sala no encontrada.\n");
        return -1;
    }

    GameRoom *r = &rooms[idx];
    if (r->player_count >= 2) {
        write_client(clients[conn_idx].sock, "Sala ya tiene 2 jugadores.\n");
        return -1;
    }

    // Añadir jugador 2
    r->players[1] = (Client){ .sock = clients[conn_idx].sock };
    strncpy(r->players[1].name, clients[conn_idx].name, sizeof(r->players[1].name)-1);
    r->players[1].name[sizeof(r->players[1].name)-1] = '\0';
    r->player_count = 2;
    r->active = 2; // jugando

    clients[conn_idx].room_id = r->id;
    clients[conn_idx].role = ROLE_PLAYER2;

    // Activar in_play_mode para ambos jugadores
    for (int k = 0; k < r->player_count; k++) {
        int idxc = find_conn_index_by_sock(r->players[k].sock);
        if (idxc != -1) clients[idxc].in_play_mode = 1;
    }

    // Notificar a ambos jugadores
    char msg[256];
    snprintf(msg, sizeof(msg), "Jugador %.32s se ha unido. La partida empieza.", clients[conn_idx].name);
    write_client(r->players[0].sock, msg);
    write_client(r->players[1].sock, msg);

    // Mensaje de inicio de partida
    for (int k = 0; k < 2; k++) {
        write_client(r->players[k].sock,
            "\n=== LA PARTIDA COMIENZA ===\n"
            "Para salir de la partida y volver al menú: /q\n");
    }

    // Inicializar turno del jugador 1
    r->turn = 1;

    // Enviar tablero inicial
    send_board_to_room(r);

    return r->id;
}


static int handle_watch(int conn_idx, int id) {
    int idx = find_room_by_id(id);
    if (idx == -1) {
        write_client(clients[conn_idx].sock, "Sala no encontrada.");
        return -1;
    }
    GameRoom *r = &rooms[idx];
    if (r->spec_count >= MAX_SPECTATORS) {
        write_client(clients[conn_idx].sock, "Sala llena de espectadores.");
        return -1;
    }
    r->spectators[r->spec_count] = (Client){ .sock = clients[conn_idx].sock };
    strncpy(r->spectators[r->spec_count].name, clients[conn_idx].name, sizeof(r->spectators[r->spec_count].name)-1);
    r->spectators[r->spec_count].name[sizeof(r->spectators[r->spec_count].name)-1] = '\n';
    r->spec_count++;


    clients[conn_idx].room_id = r->id;
    clients[conn_idx].role = ROLE_SPECTATOR;


    write_client(clients[conn_idx].sock, "Te has unido como espectador.");
    send_board_to_room(r);
    return r->id;
}


static void process_text_message(int conn_idx, const char *txt) {

    char line[BUF_SIZE];
    strncpy(line, txt, sizeof(line)-1);
    line[sizeof(line)-1] = '\0';       // asegurar null-termination

    // trim inicio
    while (*line == ' ') memmove(line, line+1, strlen(line));

    if (*line == '\0' || *line == '\n') return;

    // ---- COMANDOS ----
    if (line[0] == '/') {

        if (strncmp(line, "/create_game", 12) == 0) {
            handle_create_game(conn_idx);
            clients[conn_idx].in_play_mode = 0;
            return;
        }
        else if (strncmp(line, "/join_game", 10) == 0) {
            int id = atoi(line + 11);
            handle_join_game(conn_idx, id);
            clients[conn_idx].in_play_mode = 0;
            return;
        }
        else if (strncmp(line, "/watch", 6) == 0) {
            int id = atoi(line + 7);
            handle_watch(conn_idx, id);
            clients[conn_idx].in_play_mode = 0;
            return;
        }
        else if (strncmp(line, "/q", 2) == 0) {
            handle_leave(conn_idx);
            clients[conn_idx].in_play_mode = 0;
            write_client(clients[conn_idx].sock, "Has vuelto al menú.\n");
            return;
        }
        else if (strncmp(line, "/list", 5) == 0) {
            char buf[1024];
            int off = 0;
            off += snprintf(buf + off, sizeof(buf) - off, "Salles actives:\n");
            for (int i = 0; i < MAX_GAMES; ++i)
                if (rooms[i].active)
                    off += snprintf(buf + off, sizeof(buf) - off,
                                    "ID %d - Joueurs: %d - Spectateurs: %d\n",
                                    rooms[i].id, rooms[i].player_count, rooms[i].spec_count);
            write_client(clients[conn_idx].sock, buf);
            return;
        }
        else if (strncmp(line, "/leave", 6) == 0) {
            handle_leave(conn_idx);
            return;
        }
        else {
            write_client(clients[conn_idx].sock,
                "Commande inconnue. Comandos: /create_game, /join_game <id>, /watch <id>, /list, /q\n");
            return;
        }
    }

    // ---- MOVIMIENTOS ----
    int rid = clients[conn_idx].room_id;
    if (rid == -1) {
        write_client(clients[conn_idx].sock, "No estás en ninguna sala.\n");
        return;
    }

    int rindex = find_room_by_id(rid);
    if (rindex == -1) { write_client(clients[conn_idx].sock, "Sala no encontrada.\n"); return; }

    GameRoom *r = &rooms[rindex];

    if (clients[conn_idx].role != ROLE_PLAYER1 && clients[conn_idx].role != ROLE_PLAYER2) {
        write_client(clients[conn_idx].sock, "Eres espectador. No puedes jugar.\n");
        return;
    }

    int player_num = (clients[conn_idx].role == ROLE_PLAYER1) ? 1 : 2;
    if (r->turn != player_num) {
        write_client(clients[conn_idx].sock, "No es tu turno. Espera.\n");
        return;
    }

    int move = atoi(line);
    if (move < 1 || move > 6) {
        write_client(clients[conn_idx].sock, "Movimiento inválido (1-6).\n");
        return;
    }

    if (!process_move(r->matrix, player_num, move, &r->score1, &r->score2)) {
        write_client(clients[conn_idx].sock, "Movimiento ilegal según reglas.\n");
        return;
    }

    // cambiar turno
    r->turn = (r->turn == 1) ? 2 : 1;
    send_board_to_room(r);

    if (!can_play(r->matrix)) {
        char endmsg[256];
        snprintf(endmsg, sizeof(endmsg),
                 "=== FIN DE LA PARTIDA === Score J1: %d | Score J2: %d\n",
                 r->score1, r->score2);
        broadcast_to_room(r, endmsg);
        close_room(rindex);
    }
}





int main(void) {

    // inicializas arrays de maximo de juegos y de clientes
    for (int i = 0; i < MAX_GAMES; ++i) rooms[i].active = 0;
    for (int i = 0; i < MAX_CLIENTS; ++i) clients[i].connected = 0;

    // INICIAMOS LA CONEXION 
    listen_sock = init_connection();
    if (listen_sock < 0) { 
        perror("init_connection"); return EXIT_FAILURE; 
    }


    printf("Server Awalé  waiting...");

    fd_set readfds;
    int maxfd = listen_sock;

    while (1) {

        FD_ZERO(&readfds);
        FD_SET(listen_sock, &readfds);

        // Añadir clientes al conjunto
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

        /* ---- NUEVA CONEXIÓN ---- */
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

                if (n <= 0) {
                    closesocket(csock);
                }
                else {
                    int idx = register_new_conn(csock, namebuf);

                    if (idx == -1) {
                        write_client(csock, "Servidor lleno.\n");
                        closesocket(csock);
                    }
                    else {
                        char welcome[256];
                        snprintf(welcome, sizeof(welcome),
                            "Bienvenido %s!\nComandos:\n"
                            "/create_game\n/join_game <id>\n/watch <id>\n/list\n",
                            namebuf);

                        write_client(csock, welcome);
                        printf("Nuevo cliente conectado: %s (sock=%d)\n", namebuf, csock);
                    }
                }
            }
        }

        /* ---- MENSAJES DE CLIENTES EXISTENTES ---- */
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (!clients[i].connected) continue;

            int s = clients[i].sock;

            if (FD_ISSET(s, &readfds)) {

                char buf[BUF_SIZE];
                int n = read_client(s, buf);

                // Cliente desconectado
                if (n <= 0) {
                    printf("Cliente desconectado: %s\n", clients[i].name);

                    // Si pertenece a una sala -> manejar abandono
                    if (clients[i].room_id != -1) {
                        int ridx = find_room_by_id(clients[i].room_id);

                        if (ridx != -1) {
                            GameRoom *r = &rooms[ridx];

                            // Si era jugador
                            for (int p = 0; p < r->player_count; ++p) {
                                if (r->players[p].sock == s) {
                                    if (r->player_count == 2) {
                                        int other_sock = r->players[1 - p].sock;
                                        write_client(other_sock,
                                            "Tu oponente se ha desconectado. La sala se cerrará.\n");
                                    }
                                    close_room(ridx);
                                    break;
                                }
                            }

                            // Si era espectador
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
                else { // Cliente envió texto
                    buf[n] = '\0';  // ← muy importante
                    process_text_message(i, buf);
                }
            }
        }
    }



    end_connection(listen_sock);
    return EXIT_SUCCESS;
}
