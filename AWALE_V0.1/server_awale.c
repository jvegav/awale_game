#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "server2.h"
#include "client2.h"
#include "awale_logic.h"

static void send_board_to_clients(Client clients[2], int matrix[2][6], int turn, int score1, int score2) {
    char msg[1024], board[512];

    sprintf(msg, "\n=== SCORE ===\nJoueur 1: %d | Joueur 2: %d\n", score1, score2);

    if (turn == 1) {
        // Jugador 1: su turno
        strcpy(msg, "");
        sprintf(msg, "\n=== SCORE ===\nJoueur 1: %d | Joueur 2: %d\n", score1, score2);
        strcat(msg, "\nC'est le tour du Joueur 1\n");
        matrix_to_string_joueur1(matrix, board);
        strcat(msg, board);
        strcat(msg, "\nChoisis une colonne (1-6): \n");
        write_client(clients[0].sock, msg);

        // Jugador 2: espera
        strcpy(msg, "");
        sprintf(msg, "\n=== SCORE ===\nJoueur 1: %d | Joueur 2: %d\n", score1, score2);
        strcat(msg, "\nTour du Joueur 1  Attend \n");
        matrix_to_string_joueur2(matrix, board);
        strcat(msg, board);
        write_client(clients[1].sock, msg);

    } else {
        // Jugador 2: su turno
        strcpy(msg, "");
        sprintf(msg, "\n=== SCORE ===\nJoueur 1: %d | Joueur 2: %d\n", score1, score2);
        strcat(msg, "\nC'est le tour du Joueur 2\n");
        matrix_to_string_joueur2(matrix, board);
        strcat(msg, board);
        strcat(msg, "\nChoisis une colonne (1-6): \n");
        write_client(clients[1].sock, msg);

        // Jugador 1: espera
        strcpy(msg, "");
        sprintf(msg, "\n=== SCORE ===\nJoueur 1: %d | Joueur 2: %d\n", score1, score2);
        strcat(msg, "\nTour du Joueur 2 Attend\n");
        matrix_to_string_joueur1(matrix, board);
        strcat(msg, board);
        write_client(clients[0].sock, msg);
    }
}

static int is_number_string(const char *s) {
    if (!s || *s == '\0') return 0;
    // permitir espacios alrededor
    while (isspace((unsigned char)*s)) s++;
    if (*s == '\0') return 0;
    // opcionalmente permitir signo? aquí no
    while (*s) {
        if (isspace((unsigned char)*s)) break;
        if (!isdigit((unsigned char)*s)) return 0;
        s++;
    }
    return 1;
}

int main(void) {
    SOCKET sock = init_connection();
    Client clients[2];
    int actual = 0;
    char buffer[BUF_SIZE];
    int matrix[2][6];
    int score_j1 = 0, score_j2 = 0;
    int turn = 1;

    init_game(matrix);
    printf("Serveur Awalé en attente de 2 joueurs...\n");

    while (actual < 2) {
        SOCKADDR_IN csin;
        socklen_t sinsize = sizeof(csin);
        int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
        if (csock == SOCKET_ERROR) {
            perror("accept()");
            continue;
        }
        // leer nombre del cliente
        int rn = read_client(csock, buffer);
        if (rn <= 0) {
            closesocket(csock);
            continue;
        }
        Client c;
        c.sock = csock;
        memset(c.name, 0, sizeof(c.name));
        strncpy(c.name, buffer, sizeof(c.name) - 1);
        clients[actual++] = c;
        printf("Joueur %d connecté: %s\n", actual, c.name);
    }

    // Enviar estado inicial a ambos clientes
    send_board_to_clients(clients, matrix, turn, score_j1, score_j2);

    while (can_play(matrix)) {
        fd_set rdfs;
        FD_ZERO(&rdfs);
        FD_SET(clients[0].sock, &rdfs);
        FD_SET(clients[1].sock, &rdfs);

        int maxfd = (clients[0].sock > clients[1].sock) ? clients[0].sock : clients[1].sock;
        if (select(maxfd + 1, &rdfs, NULL, NULL, NULL) < 0) {
            perror("select()");
            break;
        }

        for (int i = 0; i < 2; i++) {
            if (!FD_ISSET(clients[i].sock, &rdfs))
                continue;

            Client *current = &clients[i];
            Client *opponent = &clients[1 - i];

            memset(buffer, 0, sizeof(buffer));
            int n = read_client(current->sock, buffer);
            if (n <= 0) {
                // cliente desconectado: cerrar y terminar
                printf("Cliente %s desconectado.\n", current->name);
                write_client(opponent->sock, "L'autre joueur s'est déconnecté. Fin.\n");
                closesocket(current->sock);
                closesocket(opponent->sock);
                end_connection(sock);
                return 0;
            }

            // recortar spaces al inicio
            char *p = buffer;
            while (isspace((unsigned char)*p)) p++;

            // --- CHAT: prefijo '#' (puedes cambiarlo) ---
            if (*p == '#') {
                // mensaje sin el '#'
                char *msg_text = p + 1;
                while (isspace((unsigned char)*msg_text)) msg_text++;
                char chat_msg[BUF_SIZE + 64];
                snprintf(chat_msg, sizeof(chat_msg), "CHAT:[%s]: %s\n", current->name, msg_text);
                // enviar a ambos (emisor y receptor) para que el emisor vea su mensaje
                write_client(opponent->sock, chat_msg);
                //write_client(current->sock, chat_msg);
                continue;
            }

            // --- JUGADA: solo si es su turno ---
            if ((turn == 1 && i == 0) || (turn == 2 && i == 1)) {
                // validar que sea número
                if (!is_number_string(p)) {
                    write_client(current->sock, "Entrada invalida. Envía un número (1-6) o un chat con '#mensaje'.\n");
                    continue;
                }

                int move = atoi(p);
                if (!process_move(matrix, turn, move, &score_j1, &score_j2)) {
                    write_client(current->sock, "Coup invalide, réessaie.\n");
                    continue;
                }

                // movimiento aceptado: cambiar turno y enviar tablero actualizado
                turn = (turn == 1) ? 2 : 1;
                send_board_to_clients(clients, matrix, turn, score_j1, score_j2);
            } else {
                write_client(current->sock, "Ce n'est pas ton tour.\n");
            }
        } // for clients
    } // while can_play

    char endmsg[256];
    sprintf(endmsg, "\n=== FIN DU JEU ===\nScore J1: %d | Score J2: %d\n", score_j1, score_j2);
    write_client(clients[0].sock, endmsg);
    write_client(clients[1].sock, endmsg);

    closesocket(clients[0].sock);
    closesocket(clients[1].sock);
    end_connection(sock);
    return 0;
}
