CC = gcc


LIBS = -lncurses


AWALE_DIR = AWALE_v.2
CLIENT_DIR = $(AWALE_DIR)/Client
SERVER_DIR = $(AWALE_DIR)/Server


SERVER_SRC = $(SERVER_DIR)/server_awale.c $(SERVER_DIR)/awale_logic.c $(SERVER_DIR)/server2.c
SERVER_TARGET = server


CLIENT_SRC = $(CLIENT_DIR)/cliente_awale.c $(CLIENT_DIR)/client2.c
CLIENT_TARGET = client


all: $(SERVER_TARGET) $(CLIENT_TARGET)


$(SERVER_TARGET): $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER_TARGET)


$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) $(LIBS) -o $(CLIENT_TARGET)

clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)
