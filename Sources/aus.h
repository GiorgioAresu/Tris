#ifndef aus_H
#define aus_H

	#include <ncurses.h>
	#include <string.h>
	#include <sys/socket.h>
	#include "precompDefs.h"

	// Struttura per le celle
	struct cell {
		int y;
		int x;
		int status;     // 0: vuota, 1: giocatore1, 2: giocatore2 o IA
	};
	
	// Struttura dei giocatori
	struct player {
		char name[11];
		bool hasCross;	// Simbolo scelto
		bool hasSCP;	// SCP = Second Color Pair
	};
	
	// Struttura per le voci di menu
	struct menuItem {
		int y;			// Coordinate
		int x;
		char label[37];	// Testo della voce
	};
	
	/* * * * * * * * * * * * * * * * * * * * *
	 *        Prototipi delle funzioni       *
	 * * * * * * * * * * * * * * * * * * * * */	
	void initializeCells(struct cell cells[9]);
	void drawGrid(WINDOW *window);
	void drawSign(WINDOW *window, struct cell *CELL, int *player_turn, struct player currentPlayer);
	void IAturn(WINDOW *gameTable, struct cell cells[9], int *player_turn, struct player *myPlayer, int *clientFd);
	bool checkVictory(WINDOW *window, struct cell cells[9], struct player *currPlayer);
	void drawWinningSigns(WINDOW *window, struct cell *cell1, struct cell *cell2, struct cell *cell3, struct player currPlayer);
	bool checkEnd(struct cell cells[9]);	
	void printStatus(WINDOW *window, struct cell cells[9]);
	void trashInput(int quantity);
	void writeSlowly(WINDOW *window, int y, int x, char *text, int slowlyChar, int slowlyRow, int slowlyFinally);
	void normalizeString(char str[], bool capitalize);
	void drawMenuItem(WINDOW *window, struct menuItem menuItems, bool printSlowly, bool selected);
	
	// Restituisce l'indice della scelta
	int drawMainMenu(WINDOW *window);
	
	// Restituiscono 0 se premuto F2, altrimenti >0
	bool initializeGame(WINDOW *window, WINDOW *status, struct player players[2], bool multiPlayer, bool overNet, int sockfd, bool connectedAsFirst);
	bool drawInformazioni(WINDOW *window);
	bool drawPlayerName(WINDOW *window, struct player *setPlayer, bool askingForPlayer2);
	bool drawServerAddress(WINDOW *window, WINDOW *status, char address[]);
	bool drawPlayerAttrs(WINDOW *window, struct player setPlayer[2]);
	bool drawInGame(WINDOW *menu, WINDOW *gameTable, WINDOW *status, struct cell cells[9], struct player players[2], bool multiPlayer, int *p_turn, bool overNet, int sockfd);
	int drawNewGame(WINDOW *window);
	
	
#endif
