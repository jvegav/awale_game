
CC = gcc
LIBS = -lncurses


SERVER_SRC = server_awale.c awale_logic.c server2.c
SERVER_TARGET = server


CLIENT_SRC = cliente_awale.c client2.c
CLIENT_TARGET = client

all: $(SERVER_TARGET) $(CLIENT_TARGET)


$(SERVER_TARGET): $(SERVER_SRC)
	$(CC) $(SERVER_SRC) -o $(SERVER_TARGET)

$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CC) $(CLIENT_SRC) $(LIBS) -o $(CLIENT_TARGET)

clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)