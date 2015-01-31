#include "aus.h"

void initializeCells(struct cell cells[9]) {
	/* Inizializzazione cells:
	 * La numerazione segue la disposizione del tastierino numerico (-1 a causa della
	 * numerazione degli elementi degli array)
	 * Es. La cell al centro è la 4 anziché la 5 ecc... */
	int k;
	for (k = 0; k < 9; k += 1) {
		switch(k/3) {
			case (0):
				cells[k].y=VERTICAL_3;
				break;
			case (1):
				cells[k].y=VERTICAL_2;
				break;
			case (2):
				cells[k].y=VERTICAL_1;
		}
		
		switch(k%3) {
			case (0):
				cells[k].x=HORIZONTAL_1;
				break;
			case (1):
				cells[k].x=HORIZONTAL_2;
				break;
			case (2):
				cells[k].x=HORIZONTAL_3;
		}
		cells[k].status=0;
	}
}

// Disegna una griglia su una finestra di 17x33 caratteri
void drawGrid(WINDOW *window) {
	int i, j, localY=0;
	wmove(window, localY, 0);
	for(i=0; i<18; i++)
	{
		if(i==6 || i==12)
			for(j=0; j<18; j++)
				if(j==5 || j==12) {
					waddch(window, ACS_PLUS);
				} else {
					if(j!=11)
						waddch(window, ACS_HLINE);

					waddch(window, ACS_HLINE);
				}
		else
			for(j=0; j<18; j++)
				if(j==5 || j==12) {
					waddch(window, ACS_VLINE);
				} else {
					if(j!=11)
						waddch(window, ' ');
						
					waddch(window, ' ');
				}
		wmove(window, localY++, 0);
	}
	
	// Scrivo i numeri nelle caselle
	mvwaddch(window, VERTICAL_3+1, HORIZONTAL_1+2, '1');
	mvwaddch(window, VERTICAL_3+1, HORIZONTAL_2+2, '2');
	mvwaddch(window, VERTICAL_3+1, HORIZONTAL_3+2, '3');
	mvwaddch(window, VERTICAL_2+1, HORIZONTAL_1+2, '4');
	mvwaddch(window, VERTICAL_2+1, HORIZONTAL_2+2, '5');
	mvwaddch(window, VERTICAL_2+1, HORIZONTAL_3+2, '6');
	mvwaddch(window, VERTICAL_1+1, HORIZONTAL_1+2, '7');
	mvwaddch(window, VERTICAL_1+1, HORIZONTAL_2+2, '8');
	mvwaddch(window, VERTICAL_1+1, HORIZONTAL_3+2, '9');
	
	wrefresh(window);
}

// Disegna il simbolo nella casella specificata e assegna il turno al giocatore successivo
void drawSign(WINDOW *window, struct cell *CELL, int *player_turn, struct player currentPlayer) {
	int i;
	
	/* NON do errore se già occupata. Fare attenzione nelle chiamate */
	if (*player_turn!=2) {
		if (CELL->status==0)
			CELL->status=*player_turn+1;
			
		// Se non sto segnando la vittoria imposto il colore relativo al giocatore corrente
		wattron(window, COLOR_PAIR(currentPlayer.hasSCP+1));
	}
	
	if (currentPlayer.hasCross) {
		for(i=0; i<5; i++) {
			mvwaddch(window, CELL->y+i, CELL->x+2*i, ACS_DIAMOND);
			mvwaddch(window, CELL->y+i, CELL->x+2*(4-i), ACS_DIAMOND);
		}
	} else {
		for (i=0; i<5; i++){
			mvwaddch(window, CELL->y+i, CELL->x+(i+2)%4*2, ACS_DIAMOND);
			mvwaddch(window, CELL->y+i, CELL->x+(4-(i+2)%4)*2, ACS_DIAMOND);
		}
	}
	
	if (*player_turn!=2) {
		wattroff(window, A_COLOR);
		*player_turn=1-*player_turn;
	}
	wrefresh(window);
}

// Intelligenza Artificiale
void IAturn(WINDOW *gameTable, struct cell cells[9], int *player_turn, struct player *myPlayer, int *clientFd) {
	char buf[LINESIZE];
	int i, temp[4], random, index;
	
	bzero(buf,LINESIZE);
	srand(time(NULL));
	random=rand()%100;
	
	for (i=0; i<9; i++) {
		if (cells[i].status==0) {
			// Se posso vinco
			cells[i].status=*player_turn+1;
			if (checkVictory(NULL, cells, myPlayer)==1) {
				cells[i].status=0;
				if (gameTable!=NULL) {
					drawSign(gameTable, &cells[i], player_turn, *myPlayer);
				} else {
					cells[i].status=*player_turn+1;
					buf[0]=i+49;
					send(*clientFd, buf, LINESIZE, 0);
				}
				return;
			} else {
				cells[i].status=0;
			}
		}
	}
	
	for (i=0; i<9; i++) {
		if (cells[i].status==0) {
		// Se l'avversario può vincere lo blocco
			cells[i].status=*player_turn;
			if (checkVictory(NULL, cells, myPlayer)==1) {
				cells[i].status=0;
				if (gameTable!=NULL) {
					drawSign(gameTable, &cells[i], player_turn, *myPlayer);
				} else {
					cells[i].status=*player_turn+1;
					buf[0]=i+49;
					send(*clientFd, buf, LINESIZE, 0);
				}
				return;
			} else {
				cells[i].status=0;
			}
		}
	}
	
	// Con varie probabilità (definite in precompDefs.h) scelgo dove mettere il segno
	if (random<PUT_IN_CENTER_PERCENT) {
		if (cells[4].status == 0) {
			if (gameTable!=NULL) {
				drawSign(gameTable, &cells[4], player_turn, *myPlayer);
			} else {
				cells[4].status=*player_turn+1;
				buf[0]='5';
				send(*clientFd, buf, LINESIZE, 0);
			}
			return;
		}
	}
	
	if (random<PUT_IN_CORNER_PERCENT) {
		temp[0]=0;
		temp[1]=2;
		temp[2]=6;
		temp[3]=8;
		for (i=0; i<4; i++) {
			index = temp[(random+i)%4];
			if (cells[index].status == 0) {
				if (gameTable!=NULL) {
					drawSign(gameTable, &cells[index], player_turn, *myPlayer);
				} else {
					cells[index].status=*player_turn+1;
					buf[0]=index+49;
					send(*clientFd, buf, LINESIZE, 0);
				}
				return;
			}
		}
	}
	
	temp[0]=1;
	temp[1]=3;
	temp[2]=5;
	temp[3]=7;
	for (i=0; i<4; i++) {
		index = temp[(random+i)%4];
		if (cells[index].status == 0) {
			if (gameTable!=NULL) {
				drawSign(gameTable, &cells[index], player_turn, *myPlayer);
			} else {
				cells[index].status=*player_turn+1;
				buf[0]=index+49;
				send(*clientFd, buf, LINESIZE, 0);
			}
			return;
		}
	}
}

// Controllo se qualcuno ha vinto
bool checkVictory(WINDOW *window, struct cell cells[9], struct player *currPlayer) {
	if (cells[4].status!=0) {
		if (cells[0].status==cells[4].status && cells[4].status==cells[8].status) { 		// DIAGONALE .'
			if (window != NULL) {
				drawWinningSigns(window, &cells[0], &cells[4], &cells[8], *currPlayer);
			}
			return 1;
		} else if (cells[2].status==cells[4].status && cells[4].status==cells[6].status) {	// DIAGONALE '.
			if (window != NULL) {
				drawWinningSigns(window, &cells[2], &cells[4], &cells[6], *currPlayer);
			}
			return 1;
		} else if (cells[3].status==cells[4].status && cells[4].status==cells[5].status) {	// RIGA CENTRALE
			if (window != NULL) {
				drawWinningSigns(window, &cells[3], &cells[4], &cells[5], *currPlayer);
			}
			return 1;
		} else if (cells[1].status==cells[4].status && cells[4].status==cells[7].status) {	// COLONNA CENTRALE
			if (window != NULL) {
				drawWinningSigns(window, &cells[1], &cells[4], &cells[7], *currPlayer);
			}
			return 1;
		}
	}
	
	if (cells[0].status!=0) {
		if (cells[1].status==cells[0].status && cells[0].status==cells[2].status) {			// RIGA INFERIORE
			if (window != NULL) {
				drawWinningSigns(window, &cells[0], &cells[1], &cells[2], *currPlayer);
			}
			return 1;
		} else if (cells[3].status==cells[0].status && cells[0].status==cells[6].status) {	// COLONNA SX
			if (window != NULL) {
				drawWinningSigns(window, &cells[0], &cells[3], &cells[6], *currPlayer);
			}
			return 1;
		}
	}
	
	if (cells[8].status!=0) {
		if (cells[6].status==cells[8].status && cells[8].status==cells[7].status) {			// RIGA SUPERIORE
			if (window != NULL) {
				drawWinningSigns(window, &cells[6], &cells[7], &cells[8], *currPlayer);
			}
			return 1;
		} else if (cells[2].status==cells[8].status && cells[8].status==cells[5].status) {	// COLONNA DX
			if (window != NULL) {
				drawWinningSigns(window, &cells[2], &cells[5], &cells[8], *currPlayer);
			}
			return 1;
		}
	}
	return 0;
}

// Disegno la mossa della vittoria con lo stesso simbolo ma bianco
void drawWinningSigns(WINDOW *window, struct cell *cell1, struct cell *cell2, struct cell *cell3, struct player currPlayer) {
	int i=2;
	drawSign(window, cell1, &i, currPlayer);
	drawSign(window, cell2, &i, currPlayer);
	drawSign(window, cell3, &i, currPlayer);
}

// Controlla se la partita è terminata (non la vittoria ma solo che non ci sono più celle libere)
bool checkEnd(struct cell cells[9]) {
	int i;
	for(i=0; i<9; i++) {
		if(cells[i].status==0)
			return 0;
	}
	return 1;
}

/* Non serve, era usata per debug */
void printStatus(WINDOW *window, struct cell cells[9]) {
	int i, j;
	
	wmove(window, 0, 0);
	for(i=0; i<9; i++){
		wprintw(window, "Cella: %d; Reale: %d, Stato: %d\n", i, i+1, cells[i].status);
	}
	wrefresh(window);
}

/* Scarta input (con getch altrimenti rischio di recuperare input "vecchio")
 * quantity:	Input scartati; comportano un lag, tenere relativamente basso
 * 				(massimo qualche centinaio) */
void trashInput(int quantity) {
	int j;
	
	timeout(0);
	for(j=0; j<quantity; j++)
		getch();
	timeout(-1);
}

// Scrive e aggiorna la finestra, permette di impostare delle tempistiche tra i caratteri,
// tra una riga e l'altra e alla fine
void writeSlowly(WINDOW *window, int y, int x, char *text, int slowlyChar, int slowlyRow, int slowlyFinally) {
	wmove(window, y, x);
	while(*text!='\0') {
		if(*text=='\n') {
			wmove(window, ++y, x);
			usleep(slowlyRow);
		} else {
			waddch(window, *text);
		}
		text++;
		wrefresh(window);
		usleep(slowlyChar);
	}
	usleep(slowlyFinally);
}

// Pulisce una stringa, mettendo il carattere di terminazione alla prima
// occorrenza di un carattere non alfanumerico.
// Con capitalize si può farla in maiuscolo (utile per strcmp)
void normalizeString(char str[], bool capitalize) {
	int i;
	for (i = 0; i < strlen(str); i ++)
	{
		if (capitalize) 
			str[i]=toupper(str[i]);
			
		if(!((str[i]>='A' && str[i]<='Z') || (str[i]>='a' && str[i]<='z')|| (str[i]>='0' && str[i]<='9') || str[i]==' ')) {
			// Non è un carattere alfanumerico
			str[i]='\0';
			break;
		}
	}
}

// Disegna una voce di menu (evidenziandola se selezionata)
void drawMenuItem(WINDOW *window, struct menuItem menuItems, bool printSlowly, bool selected) {
	/* Se l'elemento deve essere selezionato utilizzo A_REVERSE per scambiare i
	 * colori di testo e sfondo. Con hline coloro lo sfondo della riga */
	if(selected)
		wattron(window, A_REVERSE);
	mvwhline(window, menuItems.y, menuItems.x, ' ', 36);
	writeSlowly(window,  menuItems.y, menuItems.x, menuItems.label, BETWEEN_CHARS_LO*printSlowly, BETWEEN_LINES_HI*printSlowly, AFTER_LAST_LINE_HI*printSlowly);
	wattroff(window, A_REVERSE);
}

// Disegna il menu principale
int drawMainMenu(WINDOW *window) {
	static bool MenuNotYetShowed=1;	// Mi serve per visualizzare i caratteri lentamente solo all'avvio del gioco
	static int selected=0;
	const struct menuItem menuItems[7] = { 	{ 6, 2, "Giocatore singolo   (Locale)"},
											{ 7, 2, "Due giocatori       (Locale)"},
											{ 8, 2, "Giocatore singolo   (TCP/IP)"},
											{ 9, 2, "Due giocatori       (TCP/IP)"},
											{11, 2, "Avvia server su questo host"},
											{14, 2, "Informazioni"},
											{15, 2, "Esci                            (F2)" }};
	int ch;
	
	wclear(window);
	box(window, 0, 0);
	wrefresh(window);
	
	/* Per le righe "statiche" utilizzo direttamente writeSlowly, per quelle
	 * selezionabili uso la struttura menuItem e la funzione drawMenuItem
	 * appositamente create */
	writeSlowly(window, 1,  2, "Benvenuto nel gioco CurTris =)", BETWEEN_CHARS_HI*MenuNotYetShowed, BETWEEN_LINES_HI*MenuNotYetShowed, AFTER_LAST_LINE_HI*MenuNotYetShowed);
	writeSlowly(window, 3,  2, "Scegli il tipo di partita e\nbuon divertimento:", BETWEEN_CHARS_HI*MenuNotYetShowed, BETWEEN_LINES_LO*MenuNotYetShowed, AFTER_LAST_LINE_HI*MenuNotYetShowed);
	drawMenuItem(window, menuItems[0], MenuNotYetShowed, (selected==0));
	drawMenuItem(window, menuItems[1], MenuNotYetShowed, (selected==1));
	drawMenuItem(window, menuItems[2], MenuNotYetShowed, (selected==2));
	drawMenuItem(window, menuItems[3], MenuNotYetShowed, (selected==3));
	
	// Trattini
	usleep(BETWEEN_CHARS_HI*MenuNotYetShowed);
	mvwaddch(window, 10, 16, ACS_HLINE);
	wrefresh(window);
	usleep(BETWEEN_LINES_HI*MenuNotYetShowed);
	mvwaddch(window, 10, 18, ACS_HLINE);
	wrefresh(window);
	usleep(BETWEEN_LINES_HI*MenuNotYetShowed);
	mvwaddch(window, 10, 20, ACS_HLINE);
	wrefresh(window);
	usleep(BETWEEN_LINES_HI*MenuNotYetShowed);
	mvwaddch(window, 10, 22, ACS_HLINE);
	wrefresh(window);
	usleep(AFTER_LAST_LINE_HI*MenuNotYetShowed);
	// Fine trattini
	
	drawMenuItem(window, menuItems[4], MenuNotYetShowed, (selected==4));
	
	// Trattini
	usleep(BETWEEN_CHARS_HI*MenuNotYetShowed);
	mvwaddch(window, 12, 16, ACS_S9);
	wrefresh(window);
	usleep(BETWEEN_LINES_HI*MenuNotYetShowed);
	mvwaddch(window, 12, 18, ACS_S9);
	wrefresh(window);
	usleep(BETWEEN_LINES_HI*MenuNotYetShowed);
	mvwaddch(window, 12, 20, ACS_S9);
	wrefresh(window);
	usleep(BETWEEN_LINES_HI*MenuNotYetShowed);
	mvwaddch(window, 12, 22, ACS_S9);
	wrefresh(window);
	usleep(AFTER_LAST_LINE_HI*MenuNotYetShowed);
	// Fine trattini
	
	drawMenuItem(window, menuItems[5], MenuNotYetShowed, (selected==5));
	drawMenuItem(window, menuItems[6], MenuNotYetShowed, (selected==6));
	
	MenuNotYetShowed=0;
	trashInput(100);
	
	// Gestione dell'input
	while (1) {
		switch (ch=getch())	{
			case KEY_F(2):
				return -1;
				break;
			case KEY_DOWN:
				if(selected!=6) {
					drawMenuItem(window, menuItems[selected++], MenuNotYetShowed, FALSE);
					drawMenuItem(window, menuItems[selected], MenuNotYetShowed, TRUE);
				}
				break;
			case KEY_UP:
				if(selected!=0) {
					drawMenuItem(window, menuItems[selected--], MenuNotYetShowed, FALSE);
					drawMenuItem(window, menuItems[selected], MenuNotYetShowed, TRUE);
				}
				break;
			case 10:						// ↵ (KEY_ENTER non funziona)
			case 13:
				return selected;
		}
	}
}

/* Chiede il nome ai giocatori (se singolo giocatore imposta quello del
 * giocatore 2 a CPU IA) poi scrive nella finestra status */
bool initializeGame(WINDOW *window, WINDOW *status, struct player players[2], bool multiPlayer, bool overNet, int sockfd, bool connectedAsFirst) {	
	int len;
	char buf[LINESIZE];
	struct timeval timeOut; //per il timeout della connessione
	struct timeval noTimeOut;
	timeOut.tv_sec = 1;
	timeOut.tv_usec = noTimeOut.tv_sec = noTimeOut.tv_usec = 0;
	
	if(!drawPlayerName(window, &players[0], 0) || !drawPlayerAttrs(window, players)) {
		if(overNet) {
			mvwhline(status, 0, 0, ' ', 77);
			mvwprintw(status, 0, 0, "Connessione al server interrotta");
			wrefresh(status);
			shutdown(sockfd, 2);
			close(sockfd);
		}
		return 0;
	}
	
	if (multiPlayer) {
		if (!overNet && !drawPlayerName(window, &players[1], 1)) {
			return 0;
		}
	} else {
		sprintf(players[1].name, "CPU IA");
	}

	if (overNet && multiPlayer) {
		// Scambio nomi con l'altro client
		bzero(buf, LINESIZE);
		strcpy(buf,players[0].name);
		len=strlen(buf);
		if (send(sockfd,buf,len,0)==-1) {
			shutdown(sockfd, 2);
			close(sockfd);
			return 0;
		}
		bzero(buf, LINESIZE);
		
		// Imposto il timeout al socket, se recv == -1 => timeout
		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeOut, sizeof timeOut);

		// Attesa connessione del giocatore 2
		bzero(buf, LINESIZE);
		timeout(0);
		while ((len = recv(sockfd, buf, LINESIZE, 0)) == -1) {
			if (getch()==KEY_F(2)) {
				setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&noTimeOut, sizeof noTimeOut);
				shutdown(sockfd, 2);
				close(sockfd);
				break;
			}
		}
		timeout(-1);
		
		// Disattivo il timeout al socket
		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&noTimeOut, sizeof noTimeOut);
		
		if (len==0) {
			shutdown(sockfd, 2);
			close(sockfd);
			return 0;
		}

		normalizeString(buf, 0);
		sprintf(players[1].name, "%s", buf);
		
		/* Rispedisco il messaggio come Acknowledgement */
		if (connectedAsFirst && send(sockfd,buf,len,0)==-1) {
			shutdown(sockfd, 2);
			close(sockfd);
			return 0;
		}
		
		bzero(buf, LINESIZE);
	}
	
	wattron(status, A_BOLD);
	wattron(status, COLOR_PAIR(players[0].hasSCP+1));
	mvwhline(status, 0, 0, ' ', 77);
	mvwaddstr(status, 0, 0, players[0].name);
	wattroff(status, A_COLOR);
	waddstr(status, " Vs. ");
	wattron(status, COLOR_PAIR(players[1].hasSCP+1));
	waddstr(status, players[1].name);
	wattroff(status, A_COLOR);
	wattroff(status, A_BOLD);
	wrefresh(status);
	return 1;
}

// Disegna la finestra informazioni
bool drawInformazioni(WINDOW *window) {
	char *temp;
	int ch;
	
	wclear(window);
	box(window, 0, 0);
	wrefresh(window);
	writeSlowly(window,  1, 2, "CurTris e' una implementazione del\nclassico gioco del tris realizzata\nutilizzando le librerie \"curses\".", BETWEEN_CHARS_ME, BETWEEN_LINES_LO, AFTER_LAST_LINE_HI);
	writeSlowly(window,  5, 2, "Scegliere la modalita' di gioco e\nseguire le istruzioni a schermo.", BETWEEN_CHARS_ME, BETWEEN_LINES_LO, AFTER_LAST_LINE_HI);
	writeSlowly(window,  8, 2, "Per giocare utilizzare il tastierino\nnumerico.", BETWEEN_CHARS_ME, BETWEEN_LINES_LO, AFTER_LAST_LINE_HI);
	writeSlowly(window, 11, 2, "Progettato e realizzato da:", BETWEEN_CHARS_ME, BETWEEN_LINES_LO, AFTER_LAST_LINE_LO);
	temp="Giorgio Aresu";
	writeSlowly(window, 12, 38-strlen(temp), temp, BETWEEN_CHARS_ME, BETWEEN_LINES_LO, AFTER_LAST_LINE_LO);
	temp="Sara Bressan";
	writeSlowly(window, 13, 38-strlen(temp), temp, BETWEEN_CHARS_ME, BETWEEN_LINES_LO, AFTER_LAST_LINE_LO);
	temp="<Indietro>";
	wattron(window, A_REVERSE);
	writeSlowly(window, 15, (38-strlen(temp))/2, temp, BETWEEN_CHARS_LO, BETWEEN_LINES_LO, AFTER_LAST_LINE_LO);
	wattroff(window, A_REVERSE);
	wmove(window, 15, 38/2);
	trashInput(200);
	while (1)
		switch (ch=getch())
		{
			case 10:		// ↵ (KEY_ENTER non funziona)
			case 13:
				return 1;
				break;
			case KEY_F(2):
				return 0;
		}
}

// Permette al giocatore di inserire il proprio nome
bool drawPlayerName(WINDOW *window, struct player *setPlayer, bool askingForPlayer2) {
	char temp[15];
	int ch, pos=0;
	
	while(setPlayer->name[pos]!='\0')
		pos++;
		
	wclear(window);
	box(window, 0, 0);
	wrefresh(window);
	sprintf(temp, "Giocatore %d,", (int)askingForPlayer2+1);
	writeSlowly(window, 1,  2, temp, BETWEEN_CHARS_ME, BETWEEN_LINES_LO, AFTER_LAST_LINE_LO);
	writeSlowly(window, 2,  2, "inserisci il tuo nome*", BETWEEN_CHARS_ME, BETWEEN_LINES_LO, AFTER_LAST_LINE_LO);
	writeSlowly(window, 11, 2, "*Massimo 10 caratteri tra:\n[A-Z], [a-z], [0-9],\n[ ], [-], [_], [.], [']", BETWEEN_CHARS_LO, BETWEEN_CHARS_ME, AFTER_LAST_LINE_LO);
	sprintf(temp, "<OK>");
	wattron(window, A_REVERSE);
	writeSlowly(window, 15, (38-strlen(temp))/2, temp, BETWEEN_CHARS_LO, BETWEEN_LINES_LO, AFTER_LAST_LINE_LO);
	wattroff(window, A_REVERSE);
	wmove(window, 5, 15);
	curs_set(1);
	trashInput(100);
	wprintw(window, setPlayer->name);
	
	//Gestisco l'input
	while((ch=wgetch(window))!=10 && ch!=13) {
		switch (ch)
		{
			case KEY_F(2):
				setPlayer->name[pos]='\0';
				curs_set(0);
				return 0;
			case KEY_BACKSPACE:
				if(pos>0) {
					setPlayer->name[--pos]='\0';
					wmove(window, 5, 15+pos);
					waddch(window, ' ');
					wmove(window, 5, 15+pos);
					wrefresh(window);
				}
				break;
			case ' ':
			case '-':
			case '_':
			case '.':
			case '\'':
			case '0' ... '9':
			case 'A' ... 'Z':
			case 'a' ... 'z':
				if(pos<10) {
					waddch(window, ch);
					setPlayer->name[pos++]=ch;
				}
				break;
		}
	}
	setPlayer->name[pos]='\0';
	curs_set(0);
	return 1;
}

// Permette di specificare l'indirizzo del server
bool drawServerAddress(WINDOW *window, WINDOW *status, char address[]) {
	// address deve essere lungo 16 caratteri. (xxx.xxx.xxx.xxx\0)
	char temp[5], address_copy[16], *pch;
	int ch, y, x, token, toks, pos=0, dots=0;
	bool valid = FALSE;
	
	while(address[pos]!='\0')
		if(address[pos++] == '.')
			dots++;
	
	wclear(window);
	box(window, 0, 0);
	wrefresh(window);
	writeSlowly(window, 1,  2, "Inserisci l'indirizzo IP del server*", BETWEEN_CHARS_ME, BETWEEN_LINES_LO, AFTER_LAST_LINE_LO);
	writeSlowly(window, 12, 2, "*Formato:\n  [0-255].[0-255].[0-255].[0-255]", BETWEEN_CHARS_LO, BETWEEN_CHARS_ME, AFTER_LAST_LINE_LO);
	sprintf(temp, "<OK>");
	wattron(window, A_REVERSE);
	writeSlowly(window, 15, (38-strlen(temp))/2, temp, BETWEEN_CHARS_LO, BETWEEN_LINES_LO, AFTER_LAST_LINE_LO);
	wattroff(window, A_REVERSE);
	wmove(window, 5, 12);
	curs_set(1);
	trashInput(100);
	wprintw(window, address);
	
	//Gestisco l'input
	while(!valid || toks!=4) {
		switch (ch=wgetch(window))
		{
			case KEY_F(2):
				address[pos]='\0';
				curs_set(0);
				return 0;
				break;
			case KEY_BACKSPACE:
				if(pos>0) {
					if (address[--pos]=='.')
						dots--;
						
					address[pos]='\0';
					wmove(window, 5, 12+pos);
					waddch(window, ' ');
					wmove(window, 5, 12+pos);
					wrefresh(window);
				}
				break;
			case '.':
				if(pos<15 && dots<3) {
					waddch(window, ch);
					address[pos++]=ch;
					dots++;
				}
				break;
			case '0' ... '9':
				if(pos<15) {
					waddch(window, ch);
					address[pos++]=ch;
				}
				break;
			case 10:		// ↵ (KEY_ENTER non funziona)
			case 13:
				if (dots==3)
				{
					toks = 0;
					valid=TRUE;
					bzero(address_copy, 16);
					strcpy(address_copy, address);
					
					// Divido l'indirizzo nei numeri che lo compongono
					pch = strtok(address_copy,".");
					while (pch != NULL)
					{
						token = atoi(pch);
						if (strlen(pch) == 0 || token < 0 || token > 255)
							valid=FALSE;
						toks++;					// Conto gli spezzoni per non accettare cose tipo 127.0..1
						pch = strtok(NULL, ".");
					}
						
					if (valid && toks == 4)
						break;
				}
				
				getyx(window, y, x);
				curs_set(0);
				mvwhline(status, 0, 0, ' ', 77);
				mvwprintw(status, 0, 0, "Indirizzo non valido");
				wmove(window, y, x);
				wrefresh(status);
				sleep(2);
				wclear(status);
				wrefresh(status);
				curs_set(1);
					
				break;
		}
	}
	
	address[pos]='\0';
	curs_set(0);
	return 1;
}

// Permette al giocatore di selezionare il simbolo e colore da usare nel gioco
bool drawPlayerAttrs(WINDOW *window, struct player setPlayer[2]) {
	struct menuItem menuItems[4] = { 	{ 5, 2, "[ ] Croce"},
										{ 6, 2, "[ ] Cerchio"},
										{ 9, 2, "[ ] Nero"},
										{10, 2, "[ ] Rosso"}};
	char temp[15];
	int selected=4, ch;				// 4 è per il tasto OK
	
	menuItems[!setPlayer[0].hasCross].label[1]='#';
	menuItems[setPlayer[0].hasSCP+2].label[1]='#';
	wclear(window);
	box(window, 0, 0);
	wrefresh(window);
	sprintf(temp, "%s,", setPlayer[0].name);
	writeSlowly(window, 1,  2, temp, BETWEEN_CHARS_ME, BETWEEN_LINES_LO, AFTER_LAST_LINE_LO);
	writeSlowly(window, 2,  2, "scegli il tuo simbolo", BETWEEN_CHARS_ME, BETWEEN_LINES_LO, AFTER_LAST_LINE_LO);
	writeSlowly(window, 4, 2, "--- Simbolo ---", BETWEEN_CHARS_ME, BETWEEN_LINES_LO, AFTER_LAST_LINE_ME);
	drawMenuItem(window, menuItems[0], 1, 0);
	drawMenuItem(window, menuItems[1], 1, 0);
	writeSlowly(window, 8, 2, "--- Colore ---", BETWEEN_CHARS_ME, BETWEEN_LINES_LO, AFTER_LAST_LINE_ME);
	drawMenuItem(window, menuItems[2], 1, 0);
	drawMenuItem(window, menuItems[3], 1, 0);
	sprintf(temp, "<OK>");
	wattron(window, A_REVERSE);
	writeSlowly(window, 15, (38-strlen(temp))/2, temp, BETWEEN_CHARS_LO, BETWEEN_LINES_LO, AFTER_LAST_LINE_LO);
	wattroff(window, A_REVERSE);
	
	// Gestione input
	while (1) {
		switch (ch=getch())
		{
			case KEY_F(2):
				return 0;
				break;
			case KEY_DOWN:
				if(selected!=4) {
					if(selected==3) {
						wattron(window, A_REVERSE);
						writeSlowly(window, 15, (38-strlen(temp))/2, temp, BETWEEN_CHARS_LO, BETWEEN_LINES_LO, AFTER_LAST_LINE_LO);
						wattroff(window, A_REVERSE);
					} else {
						drawMenuItem(window, menuItems[selected+1], 0, 1);
					}
					drawMenuItem(window, menuItems[selected++], 0, 0);
				}
				break;
			case KEY_UP:
				if(selected!=0) {
					if(selected==4) {
						writeSlowly(window, 15, (38-strlen(temp))/2, temp, BETWEEN_CHARS_LO, BETWEEN_LINES_LO, AFTER_LAST_LINE_LO);
					} else {
						drawMenuItem(window, menuItems[selected], 0, 0);
					}
					drawMenuItem(window, menuItems[--selected], 0, 1);
				}
				break;
			case 10:							// ↵ (KEY_ENTER non funziona)
			case 13:
				if (selected==4) {
					setPlayer[1].hasCross=!setPlayer[0].hasCross;
					setPlayer[1].hasSCP=!setPlayer[0].hasSCP;
					return 1;
				} else {
					if ((selected/2)==0) {				// Ha cambiato il simbolo
						menuItems[selected%2].label[1]='#';
						menuItems[!(selected%2)].label[1]=' ';
						drawMenuItem(window, menuItems[0], 0, !(selected%2));
						drawMenuItem(window, menuItems[1], 0, selected%2);
						setPlayer[0].hasCross=!(selected%2);
					} else {							// Ha cambiato il colore
						menuItems[2+selected%2].label[1]='#';
						menuItems[2+!(selected%2)].label[1]=' ';
						drawMenuItem(window, menuItems[2], 0, !(selected%2));
						drawMenuItem(window, menuItems[3], 0, selected%2);
						setPlayer[0].hasSCP=selected%2;
					}
				}
				break;
		}
	}
	return 1;
}

// Prepara il tavolo da gioco e gestisce la partita
bool drawInGame(WINDOW *menu, WINDOW *gameTable, WINDOW *status, struct cell cells[9], struct player players[2], bool multiPlayer, int *p_turn, bool overNet, int sockfd) {
	int y, x, ch, charsToEOL, len;
	char buf[LINESIZE];
	
	struct timeval timeOut; //per il timeout della connessione
	struct timeval noTimeOut;
	timeOut.tv_sec = 1;
	timeOut.tv_usec = noTimeOut.tv_sec = noTimeOut.tv_usec = 0;
	
	bzero(buf, LINESIZE);
	charsToEOL = COLS-(strlen(players[0].name)+strlen(players[1].name)+7);
	getyx(status, y, x);
	
	/* Preparo il nuovo tavolo da gioco */
	box(gameTable, 0, 0);
	wrefresh(gameTable);
	drawGrid(gameTable);
	initializeCells(cells);
	
	/* Se locale scelgo un giocatore a caso, altrimenti ricevo il turno
	 * dal server */
	if (overNet) {
		recv(sockfd,buf,LINESIZE,0);
		*p_turn=buf[0]-48;
		send(sockfd,"OK\n",3,0);
	} else {
		srand(time(NULL));
		*p_turn=rand()%2;
	}
	
	while(checkEnd(cells)==0 && checkVictory(NULL, cells, &players[!*p_turn])==0) {
		/* Cancello la scritta precedente, scrivo di chi è il turno e ripristino
		 * la posizione del cursore per il prossimo turno */
		mvwhline(status, y, x, ' ', charsToEOL);
		if (*p_turn == 0 || (multiPlayer && !overNet))
			mvwprintw(status, y, x, " - %s e' il tuo turno...", players[*p_turn].name);
		else
			mvwprintw(status, y, x, " - Attendi la mossa di %s", players[*p_turn].name);
			
		wmove(status, y, x);
		wrefresh(status);
		
		if (*p_turn!=0 && (!multiPlayer || (multiPlayer && overNet))) {
			/* Non deve ricevere l'input da tastiera perché è il turno dell'AI o deve
			 * ricevere dal server */
			if (overNet) {
				bzero(buf, LINESIZE);
				
				// Imposto il timeout al socket, se recv == -1 => timeout
				setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeOut, sizeof timeOut);
				
				timeout(0);
				while ((len = recv(sockfd, buf, LINESIZE, 0)) == -1) {
					if (getch()==KEY_F(2)) {
						shutdown(sockfd, 2);
						close(sockfd);
						mvwhline(status, 0, 0, ' ', 77);
						mvwprintw(status, 0, 0, "Partita terminata");
						wrefresh(status);
						return 0;
					}
				}
				timeout(-1);
				
				if (len<=0) {
					shutdown(sockfd, 2);
					close(sockfd);
					mvwhline(status, 0, 0, ' ', 77);
					mvwprintw(status, 0, 0, "Connessione al server interrotta");
					wrefresh(status);
					return 0;
				}
				
				// Disattivo il timeout al socket
				setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&noTimeOut, sizeof noTimeOut);
				
				normalizeString(buf, FALSE);
				drawSign(gameTable, &cells[buf[0]-49], p_turn, players[*p_turn]);
			} else {
				IAturn(gameTable, cells, p_turn, &players[*p_turn], NULL);
			}
		} else {
			/* Deve ricevere l'input da tastiera */
			trashInput(100);
			switch (ch=getch())
			{
				case KEY_F(2):
					if (overNet) {
						shutdown(sockfd, 2);
						close(sockfd);
					}
					mvwhline(status, 0, 0, ' ', 77);
					mvwprintw(status, 0, 0, "Partita terminata");
					wrefresh(status);
					return 0;
					break;
				case '1' ... '9':
					while(cells[ch-49].status!=0) {
						mvwhline(status, y, x, ' ', charsToEOL);
						mvwprintw(status, y, x, " - %s, e' gia' occupata, scegline un'altra", &players[*p_turn].name);
						wmove(status, y, x);
						wrefresh(status);
						ch=getch();
					}
					drawSign(gameTable, &cells[ch-49], p_turn, players[*p_turn]);
					if (overNet) {
						bzero(buf, LINESIZE);
						buf[0]=ch;
						if (send(sockfd,buf,strlen(buf),0)==-1) {
							shutdown(sockfd, 2);
							close(sockfd);
							mvwhline(status, 0, 0, ' ', 77);
							mvwprintw(status, 0, 0, "Errore di comunicazione. Connessione interrotta");
							wrefresh(status);
							return 0;
						}
					}
			}
		}
	}
	mvwhline(status, y, x, ' ', charsToEOL);
	if (checkVictory(gameTable, cells, &players[!*p_turn])==1) {
		if (multiPlayer && !overNet)
			mvwprintw(status, y, x, " - %s HAI VINTO!!!", players[!*p_turn].name);
		else if (!*p_turn == 0)
			mvwprintw(status, y, x, " - HAI VINTO!!!");
		else if (!multiPlayer)
			mvwprintw(status, y, x, " - Hai perso!");
		else
			mvwprintw(status, y, x, " - Hai perso! Vince %s", players[!*p_turn].name);
	} else {
		mvwprintw(status, y, x, " - Partita patta");
	}
	wmove(status, y, x);
	wrefresh(status);
	if(overNet) {
		shutdown(sockfd, 2);
		sleep(1);
		close(sockfd);
	}

	return 1;
}

// Permette di scegliere se iniziare subito una nuova partita o tornare al menu principale
int drawNewGame(WINDOW *window) {
	struct menuItem menuItems[4] = { 	{ 3, 2, "Si"},
										{ 4, 2, "No"}};
	int ch, selected=0;
	
	wclear(window);
	box(window, 0, 0);
	wrefresh(window);
	writeSlowly(window, 1, 2, "Vuoi giocare un'altra partita?", BETWEEN_CHARS_ME, BETWEEN_LINES_LO, AFTER_LAST_LINE_LO);
	drawMenuItem(window, menuItems[0], 1, 1);
	drawMenuItem(window, menuItems[1], 1, 0);
	
	// Gestione input
	while (1) {
		switch (ch=getch())
		{
			case KEY_F(2):
				return 0;
				break;
			case KEY_DOWN:
				if(selected!=1) {
					drawMenuItem(window, menuItems[selected++], 0, 0);
					drawMenuItem(window, menuItems[selected], 0, 1);
				}
				break;
			case KEY_UP:
				if(selected!=0) {
					drawMenuItem(window, menuItems[selected--], 0, 0);
					drawMenuItem(window, menuItems[selected], 0, 1);
				}
				break;
			case 10:							// ↵ (KEY_ENTER non funziona)
			case 13:
				return selected+1;
		}
	}
}




























