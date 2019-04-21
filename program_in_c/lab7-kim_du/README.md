# CSE5462 lab7: TicTacToe - MultiPlayer - Stream
## Authors
* Chenghao Du 
* Yongjeong Kim

## Date
2019.04.04

## Description
This is a tictactoe TCP program that enables multiple users to play at the same time.
It is modified from lab6 to apply TCP connection.

## Protocol
This program uses the version 7 protocol agreed from the class.

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
./tictactoeServer <Server Port>
```

From the client side (player 2), type:
```
./tictactoeClient <Server Port> <Server IP address> 
```


## Files
* tictactoe.h: a header file for tictactoeClient.c and tictactoeServer.c.
* tictactoeClient.c: a source file for a client side tictactoe program.
* tictactoeServer.c: a source file for a server side tictactoe program.
* makefile: a makefile that contains compile commands for tictactoeServer.c and tictactoeClient.s
