#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "client2.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s [adresse] [pseudo]\n", argv[0]);
        return EXIT_FAILURE;
    }

    SOCKET sock = init_connection(argv[1]);
    char buffer[BUF_SIZE];

    write_server(sock, argv[2]); // Enviar nombre

    printf("Connecté au serveur Awalé comme %s\n", argv[2]);

    fd_set rdfs;

    while (1) {
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        if (select(sock + 1, &rdfs, NULL, NULL, NULL) == -1) {
            perror("select()");
            exit(errno);
        }

        // Si hay mensaje del servidor
        if (FD_ISSET(sock, &rdfs)) {
            int n = read_server(sock, buffer);
            if (n == 0) {
                printf("Serveur déconnecté.\n");
                break;
            }
            printf("%s", buffer);
        }

        // Si el jugador escribe algo
        if (FD_ISSET(STDIN_FILENO, &rdfs)) {
            fgets(buffer, BUF_SIZE - 1, stdin);
            buffer[strcspn(buffer, "\n")] = 0;
            write_server(sock, buffer);
        }
    }

    closesocket(sock);
    return EXIT_SUCCESS;
}
