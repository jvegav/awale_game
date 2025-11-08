#ifndef SERVER2_H
#define SERVER2_H

#ifdef WIN32
#include <winsock2.h>
#elif defined (linux)
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

#define PORT 1977
#define MAX_GAMES 64
#define MAX_CLIENTS 256
#define MAX_SPECTATORS 32
#define NAME_LEN 64
#define BUF_SIZE 8001
#define MAX_CHAT_MESSAGES 10




typedef enum {ROLE_NONE=0, ROLE_PLAYER1, ROLE_PLAYER2, ROLE_SPECTATOR} Role;

typedef struct
{
   SOCKET sock;
   char name[BUF_SIZE];
   int room_id; // -1 if not in any room
    Role role;
    int connected;
    int in_play_mode;
    int in_chat_mode;
} Client;


typedef struct {
    int id; // room id
    int active; // 0 = free, 1 = waiting for player, 2 = playing
    Client players[2];
    int player_count;
    Client spectators[MAX_SPECTATORS];
    int spec_count;
    int matrix[2][6];
    int score1, score2;
    int turn; // 1 or 2
    char chat_history[MAX_CHAT_MESSAGES][BUF_SIZE];
    int chat_count;
} GameRoom;


/* === Funciones públicas utilizadas por server_awale.c === */
int init_connection(void);
void end_connection(int sock);
int read_client(SOCKET sock, char *buffer);
void write_client(SOCKET sock, const char *buffer);

#endif /* SERVER2_H */
