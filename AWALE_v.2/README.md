# Awalé Multiplayer Game

notes: 
the UI crashes if the size of the window (terminal) is changed mid game DO NOT change it

## Compilation

For the UA to properly work -> 
library instalation with the followig command: 

```
sudo apt install libncurses-dev
```


### Server

Compile the server using:

```
gcc server_awale.c awale_logic.c server2.c -o serv
```

### Client

Compile the client using:

```
gcc cliente_awale.c client2.c -lncurses -o cli
```

## Running the Game

### Start the Server

Run the server:

```
./serv
```

### Start the Client

Run the client and connect to the server:

```
./cli [server_address] [username]
```

Example:

```
./cli localhost Alice
```

## Available Commands

Once connected, clients can use the following commands:

### Room and Game Management

* `/create_game`
  Create a new room and become Player 1.

* `/join_game <id>`
  Join an existing room with the given ID as Player 2.

* `/watch <id>`
  Join a room as a spectator (cannot play).

* `/list`
  List all active rooms along with the number of players and spectators.

* `/leave`
  Leave the current room (as player or spectator).

* `/q`
  Leave the match and return to the menu.

### Chat and Users

* `/chat`
  Enter chat mode in your current room. You will see the chat history and can send messages.
  note: Expectators cannot write, but can read the chat of a room.

* `/users`
  List all currently connected users.

### Help

* `/help`
  Show all available commands with descriptions.

## Game Rules and Interaction

* Each player makes moves by typing numbers 1-6 when it is their turn.
* Spectators can only watch the game and read chat messages.
* Chat messages are stored in the room's chat history and can be accessed with `/chat`.

## Notes

* If a player disconnects, the room will be closed and all players and spectators notified.
* Chat messages are local to each room.
* The maximum number of chat messages stored per room is defined by `MAX_CHAT_MESSAGES`.
* You must be in a room to send messages or make moves.

Enjoy playing Awalé!
