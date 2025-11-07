#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "server2.h"

/* Inicializa el socket de escucha del servidor */
int init_connection(void)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin = {0};

    if (sock == INVALID_SOCKET) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(PORT);
    sin.sin_family = AF_INET;

    if (bind(sock, (SOCKADDR *)&sin, sizeof(sin)) == SOCKET_ERROR) {
        perror("bind()");
        closesocket(sock);
        exit(EXIT_FAILURE);
    }

    if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR) {
        perror("listen()");
        closesocket(sock);
        exit(EXIT_FAILURE);
    }

    printf("Serveur en écoute sur le port %d...\n", PORT);
    return sock;
}

/* Cierra un socket */
void end_connection(int sock)
{
    closesocket(sock);
}

/* Lee un mensaje del cliente */
int read_client(SOCKET sock, char *buffer)
{
    memset(buffer, 0, BUF_SIZE);
    int n = recv(sock, buffer, BUF_SIZE - 1, 0);
    if (n < 0) {
        perror("recv()");
        n = 0;
    }
    buffer[n] = '\0';
    return n;
}

void write_client(SOCKET sock, const char *buffer)
{
    if (send(sock, buffer, strlen(buffer), 0) < 0) {
        perror("send()");
    }
}
