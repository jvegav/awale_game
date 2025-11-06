#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "server2.h"
#include "client2.h"
#include "awale_logic.h"

static void send_board_to_clients(Client clients[2], int matrix[2][6], int turn, int score1, int score2) {
    char msg[1024], board[512];

    sprintf(msg, "\n=== SCORE ===\nJoueur 1: %d | Joueur 2: %d\n", score1, score2);

    if (turn == 1) {
        strcat(msg, "\nC'est le tour du Joueur 1\n");
        matrix_to_string_joueur1(matrix, board);
        strcat(msg, board);
        strcat(msg, "\nChoisis une colonne (1-6): \n");
        write_client(clients[0].sock, msg);

        strcpy(msg, "\nTour du Joueur 1  Attend \n");
        matrix_to_string_joueur2(matrix, board);
        strcat(msg, board);
        write_client(clients[1].sock, msg);
    } else {
        strcat(msg, "\nC'est le tour du Joueur 2\n");
        matrix_to_string_joueur2(matrix, board);
        strcat(msg, board);
        strcat(msg, "\nChoisis une colonne (1-6): \n");
        write_client(clients[1].sock, msg);

        strcpy(msg, "\nTour du Joueur 2 Attend\n");
        matrix_to_string_joueur1(matrix, board);
        strcat(msg, board);
        write_client(clients[0].sock, msg);
    }
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
        read_client(csock, buffer);
        Client c = { csock };
        strncpy(c.name, buffer, BUF_SIZE - 1);
        clients[actual++] = c;
        printf("Joueur %d connecté: %s\n", actual, c.name);
    }

    // Juego principa

    

    while (can_play(matrix)) {
        send_board_to_clients(clients, matrix, turn, score_j1, score_j2);
        Client current = (turn == 1) ? clients[0] : clients[1];
        Client opponent = (turn == 1) ? clients[1] : clients[0];

        write_client(current.sock, "Choisis une colonne (1-6): ");
        int n = read_client(current.sock, buffer);
        if (n <= 0) break;

        int move = atoi(buffer);
        if (!process_move(matrix, turn, move, &score_j1, &score_j2)) {
            write_client(current.sock, "Coup invalide, réessaie.\n");
            continue;
        }
        
        turn = (turn == 1) ? 2 : 1;
    }

    char endmsg[256];
    sprintf(endmsg, "\n=== FIN DU JEU ===\nScore J1: %d | Score J2: %d\n",
            score_j1, score_j2);
    write_client(clients[0].sock, endmsg);
    write_client(clients[1].sock, endmsg);

    closesocket(clients[0].sock);
    closesocket(clients[1].sock);
    end_connection(sock);
    return 0;
}
