#ifndef SERVER_H
#define SERVER_H

	#include <stdio.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <ctype.h>
	#include <fcntl.h>
	#include <stdlib.h>
	#include <string.h>
	#include <signal.h>
	
	#include "precompDefs.h"
	#include "aus.h"
	
	int getMove(int fd1, int fd2, int *player_turn, struct cell cells[]);
	int getType(int in);
	bool lock(int fd, int waitingPid);
	bool exchangePlayerNames(int fd1, int fd2);
	int setPlayerTurns(int fd1, int fd2, int turn);
	int main();

#endif
