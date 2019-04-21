# CSE5462 lab6: TicTacToe - MultiPlayer - Bad Net
## Authors
* Chenghao Du 
* Yongjeong Kim

## Date
2019.03.26

## Description
This is a tictactoe UDP program enables multiple users to play at the same time.
It is modified from lab5 to handle a packet loss.

## Protocol
This program uses the version 6 protocol agreed from the class.

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
## Test based on Troll
Make sure you are in stdlinux environment and log into three different IP address.
In Troll terminal, type:
```
./troll -C <Client IP address> -a <Client Port> -S <Server IP address> -b <Server Port> -r -t -x 30 <Troll Port,same as Server>
```
In Server terminal,type:
```
./tictactoeServer <Server Port>
```
In Client terminal,type:
```
./tictactoeClient <Server Port> <Server IP address> 
```

## Files
* tictactoe.h: a header file for tictactoeClient.c and tictactoeServer.c.
* tictactoeClient.c: a source file for a client side tictactoe program.
* tictactoeServer.c: a source file for a server side tictactoe program.
* makefile: a makefile that contains compile commands for tictactoeServer.c and tictactoeClient.s
