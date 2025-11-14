# Awalé Multiplayer Game

## Compilation

### Server

Compile the server using:

```
gcc server_awale.c awale_logic.c server2.c -o server
```

### Client

Compile the client using:

```
gcc cliente_awale.c client2.c -lncurses -o client

```
- lncurses: This library is used in our code to create the UI in the terminal.

- cliente_awale.c: Contains all the core game logic, including the rules of the Awale game.

- client2.c: This file has functions to handle communication with the server, managing sending and receiving data.


## Running the Game

### Start the Server

Run the server:

```
./server
```

### Start the Client

Run the client and connect to the server:

```
./client [server_address] [username]
```

Example:

```
./client localhost Alice
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

* `/users`
  List all currently connected users.

### If the user is in the game room

* `#`
  If there are more than two players the user can input a number between 1-6 to play
  If there is only one player, he should wait for the second player to play

* `/leave`
  Leave the current room (as player or spectator).

* `/q`
  Leave the match and return to the menu.

* `/chat`
  Enter chat mode in your current room. You will see the chat history and can send messages.


### Help

* `/help`
  Show all available commands with descriptions.

## Game Rules and Interaction

* Each player makes moves by typing numbers 1-6 when it is their turn.
* Spectators can only watch the game.

## Notes

* If a player disconnects, the room will be closed and all players and spectators notified.
* Chat messages are local to each room.
* The maximum number of chat messages stored per room is defined by `MAX_CHAT_MESSAGES`.
* You must be in a room to send messages or make moves.
* If a user tries to join with a username that already exists, the system automatically appends a random number to the end of their name. Example: client -> client_2382 (if client already exists)


## How we used AI-based TOOLS


- Debugging and Error Understanding:
We used AI tools to identify and understand errors in our code, especially those we couldn’t resolve on our own due to lack of experience in development with C.

- Understanding Template Code:
Also AI helped us understand how the template code works so we could adapt it for multiplayer functionality.

- Improving Code Structure and Readability:
In the same way we used these tools so we can create more functions and write cleaner, more maintainable code, so the proffessor and the other person of the group could read easily
