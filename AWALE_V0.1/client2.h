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

typedef struct
{
   SOCKET sock;
   char name[BUF_SIZE];
} Client;

// funciones cliente
SOCKET init_connection();
int read_server(SOCKET sock, char *buffer);
void write_server(SOCKET sock, const char *buffer);
void clean_space();

#endif /* CLIENT_H */
