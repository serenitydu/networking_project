# CSE5462 lab3: TicTacToe
## Author
* Chenghao Du 
* Yongjeong Kim
## Date
2019.2.05

## Description
This is a tictactoe program enables users to play together, allowing a connection between them.

## Protocol
This program uses the version 2 protocol agreed from the class.

## Compile
To compile the source file (tictactoeOriginal.c), type 
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
./tictactoeOriginal <port number> <player number>
```

From the client side (player 2), type:
```
./tictactoeOriginal <port number> <player name> <remote-IP> 
```
