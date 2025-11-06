#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>

#include "client2.h" 

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
    // send nickname to server
    write_server(sock, nick);

    fd_set rdfs;

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
                printf("Servidor desconectado.");
                break;
            }
            buffer[n] = '\0';
            printf("%s", buffer);
        }

        // entrada del usuario
        if (FD_ISSET(STDIN_FILENO, &rdfs)) {
            if (!fgets(buffer, sizeof(buffer), stdin)) break;
            // strip newline
            size_t L = strlen(buffer);
            if (L > 0 && buffer[L-1] == '\n') buffer[L-1] = '\0';

            // local handling for /quit
            if (strcmp(buffer, "/quit") == 0) {
                write_server(sock, "/quit");
                break;
            }

            // ignore empty lines
            if (buffer[0] == '\n') continue;

            // send whatever user typed to server (commands or moves)
            write_server(sock, buffer);
        }
    }

    closesocket(sock);
    return EXIT_SUCCESS;
}

