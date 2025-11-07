#ifndef CLIENT_H
#define CLIENT_H

#ifdef WIN32
#include <winsock2.h>
#elif defined(linux)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h>  /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;
#else
#error not defined for this platform
#endif

#define BUF_SIZE 1024
#define PORT 1977
#define MAX_GAMES 64
#define MAX_CLIENTS 256
#define MAX_SPECTATORS 32
#define NAME_LEN 64

// funciones cliente
SOCKET init_connection(const char *address);
int read_server(SOCKET sock, char *buffer);
void write_server(SOCKET sock, const char *buffer);
void clean_space();

#endif /* CLIENT_H */
