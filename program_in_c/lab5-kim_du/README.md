# CSE5462 lab5: TicTacToe - MultiPlayer
## Authors
* Chenghao Du 
* Yongjeong Kim

## Date
2019.03.05

## Description
This is a tictactoe UDP program enables multiple users to play at the same time.

## Protocol
This program uses the version 5 protocol agreed from the class.

## Compile
To compile the source files (tictactoeServer.c and tictactoeClient.c), type 
```
make
```

## Cleaning the compiled program
To clean the program, type 
```
make clean
```

## Running the program
To get the server IP address, in in the server-side Terminal, type: 
```
ifconfig
```

From the server side (player), type: 
```
./tictactoeServer <port number> <player number>
```

From the client side (player 2), type:
```
./tictactoeClient <port number> <player number> <remote-IP> 
```

## Files
* tictactoe.h: a header file for tictactoeClient.c and tictactoeServer.c.
* tictactoeClient.c: a source file for a client side tictactoe program.
* tictactoeServer.c: a source file for a server side tictactoe program.
* makefile: a makefile that contains compile commands for tictactoeServer.c and tictactoeClient.s
