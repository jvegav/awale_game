# === Project Name ===
PROJECT = awale

# === Directories ===
CLIENT_DIR = Client
SERVER_DIR = Server

# === Source Files ===
CLIENT_SRC = $(CLIENT_DIR)/cliente_awale.c \
             $(CLIENT_DIR)/client2.c

SERVER_SRC = $(SERVER_DIR)/server_awale.c \
             $(SERVER_DIR)/server2.c \
             $(SERVER_DIR)/awale_logic.c

# === Output Binaries ===
CLIENT_BIN = cli
SERVER_BIN = serv

# === Compiler and Flags ===
CC = gcc
CFLAGS = -Wall -Wextra -g
CLIENT_LIBS = -lncurses

# === Default Target ===
all: server client

# === Build Server ===
server: $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER_BIN)

# === Build Client ===
client: $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) $(CLIENT_LIBS) -o $(CLIENT_BIN)

# === Clean Compiled Files ===
clean:
	rm -f $(CLIENT_BIN) $(SERVER_BIN)

# === Run Commands ===
run_server: server
	./$(SERVER_BIN)

run_client: client
	./$(CLIENT_BIN) localhost Player

.PHONY: all server client clean run_server run_client
