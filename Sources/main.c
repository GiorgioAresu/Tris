#include "main.h"


int main() {
	/* Dichiarazione variabili. */
	WINDOW *mainMenu, *gameTable, *status;
	struct cell cells[9];
	struct player players[2]={	{"Giocatore1", 1, 0},
								{"Giocatore2", 0, 1}};
	int player_turn=0;			// 0: P1, 1:P2, 2: usato in checkVictory() per il bianco
	bool multiPlayer, overNet;	// Con queste 2 variabili identifico tutti i tipi di partita
	bool wantToExit=0;			// Posto a uno quando l'utente seleziona Esci o preme F2 per uscire dal gioco
	int i, ch;
	// Per TCP/IP
	int sockfd, len, pos;
	char buf[LINESIZE];			// Buffer per i messaggi da-per server
	char address[16];
	struct sockaddr_in server;
	bool connectedAsFirst=FALSE;// Usato per initializeGame, per sapere se in una multi via rete sono il primo giocatore connesso
	
	struct timeval timeOut; //per il timeout della connessione
	struct timeval noTimeOut;
	timeOut.tv_sec = 1;
	timeOut.tv_usec = noTimeOut.tv_sec = noTimeOut.tv_usec = 0;
	
	initscr();									// Inizializzo lo schermo
	mainMenu=newwin(17,40,3,38);
	gameTable=newwin(17,33, 3, 3);
	status=newwin(1,77,22,2);
	start_color();								// ⎫
	init_pair(1, FG_COLOR1, BG_COLOR);			// ⎬ Imposto i colori
	init_pair(2, FG_COLOR2, BG_COLOR);			// |
    assume_default_colors(TXT_COLOR, BG_COLOR);	// ⎭
	curs_set(0);								// Nascondo il cursore
	raw();										// No buffer (input senza ↵)
	keypad(stdscr, TRUE);						// Supporto frecce, tastierino, KEY_F(n)
	keypad(mainMenu, TRUE);
	noecho();
	initializeCells(cells);
	
	bzero(address, 16);
	
	/* Disegno il bordo della finestra principale */
	box(stdscr, 0,0);
	mvaddch(21, 0, ACS_LTEE);
	mvaddch(21, 79, ACS_RTEE);
	mvhline(21, 1, ACS_HLINE, 78);
	refresh();
	
	for (i=COLS; i>(COLS-strlen(TITLE))/2; i--) {
		mvprintw(0, i, "%.*s", COLS-i-1, &TITLE[0]);
		hline(ACS_HLINE, COLS-i-27);
		usleep(TITLE_SCROLLING_SPEED);
		refresh();
	}
	
	/* Disegno la finestra del tavolo da gioco */
	box(gameTable, 0, 0);
	wrefresh(gameTable);
	drawGrid(gameTable);
	usleep(SIGNS_SPEED_IN_LOADING);

	// Riempio la griglia iniziale
	drawSign(gameTable, &cells[6], &player_turn, players[0]);
	usleep(SIGNS_SPEED_IN_LOADING);
	drawSign(gameTable, &cells[7], &player_turn, players[1]);
	usleep(SIGNS_SPEED_IN_LOADING);
	drawSign(gameTable, &cells[8], &player_turn, players[0]);
	usleep(SIGNS_SPEED_IN_LOADING);
	drawSign(gameTable, &cells[5], &player_turn, players[1]);
	usleep(SIGNS_SPEED_IN_LOADING);
	drawSign(gameTable, &cells[2], &player_turn, players[0]);
	usleep(SIGNS_SPEED_IN_LOADING);
	drawSign(gameTable, &cells[1], &player_turn, players[1]);
	usleep(SIGNS_SPEED_IN_LOADING);
	drawSign(gameTable, &cells[0], &player_turn, players[0]);
	usleep(SIGNS_SPEED_IN_LOADING);
	drawSign(gameTable, &cells[3], &player_turn, players[1]);
	usleep(SIGNS_SPEED_IN_LOADING);
	drawSign(gameTable, &cells[4], &player_turn, players[0]);
	usleep(SIGNS_SPEED_IN_LOADING);
	
	/* Elimino l'input che l'utente può aver premuto durante il disegno
	 * dell'interfaccia e visualizzo il menù principale              */
	trashInput(200);
	
	while (!wantToExit) {
		bzero(buf, LINESIZE);
		connectedAsFirst = FALSE;
		
		// Agisco in base alla scelta dell'utente nel menu principale
		switch (drawMainMenu(mainMenu))
		{
			case 0:					// Giocatore singolo
				multiPlayer=0;
				overNet=0;
				
				// Ciclo nuove partite finché l'utente decide di non volerne più fare o uscire
				if(initializeGame(mainMenu, status, players, multiPlayer, overNet, sockfd, connectedAsFirst)) {
					do {
						wclear(mainMenu);
						box(mainMenu, 0, 0);
						writeSlowly(mainMenu, 1, 2, "Buona partita ;-)", BETWEEN_CHARS_ME, BETWEEN_LINES_LO, AFTER_LAST_LINE_LO);
						if(!drawInGame(mainMenu, gameTable, status, cells, players, multiPlayer, &player_turn, overNet, sockfd))
							break;
					} while (drawNewGame(mainMenu)==1 && !wantToExit);
				}
				break;
			case 1:					// Multiplayer locale
				multiPlayer=1;
				overNet=0;
				
				if(initializeGame(mainMenu, status, players, multiPlayer, overNet, sockfd, connectedAsFirst)) {
					do {
						wclear(mainMenu);
						box(mainMenu, 0, 0);
						writeSlowly(mainMenu, 1, 2, "Buona partita ;-)", BETWEEN_CHARS_ME, BETWEEN_LINES_LO, AFTER_LAST_LINE_LO);
						if(!drawInGame(mainMenu, gameTable, status, cells, players, multiPlayer, &player_turn, overNet, sockfd))
							break;
					} while (drawNewGame(mainMenu)==1 && !wantToExit);
				}
				break;
			case 2:					// Giocatore singolo TCP/IP
				multiPlayer=0;
				overNet=1;
				
				// Chiedo l'indirizzo del server
				if (!drawServerAddress(mainMenu, status, address))
					break;
				
				i=0;
				pos=1;
				wclear(mainMenu);
				box(mainMenu, 0, 0);
				writeSlowly(mainMenu, pos++, 2, "Connessione al server", 0, 0, 0);
				
				server.sin_family = AF_INET;
				server.sin_addr.s_addr = inet_addr(address);
				server.sin_port = htons(SERVER_PORT);
				
				if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
					writeSlowly(mainMenu, pos++, 2, "Creazione socket fallita", 0, 0, 0);
					sleep(5);
					break;
				}
				
				// Connessione al server
				if (connect(sockfd,(struct sockaddr *)&server,sizeof server)==-1) {
					writeSlowly(mainMenu, pos++, 2, "Connessione fallita", 0, 0, 0);
					sleep(5);
					break;
				}
				
				// Ricezione messaggio di benvenuto
				if ((len = recv(sockfd,buf,LINESIZE,0))>0) {
					buf[len]='\0';
					normalizeString(buf, 1);
					if (strcmp(buf,"HI") != 0)
					{
						writeSlowly(mainMenu, pos++, 2, "Risposta del server non riconosciuta", 0, 0, 0);
						shutdown(sockfd, 2);
						close(sockfd);
						writeSlowly(mainMenu, pos++, 2, "Connessione interrotta", 0, 0, 0);
						sleep(5);
						break;
					}
					send(sockfd, "HI\n", 3, 0);
				} else {
					shutdown(sockfd, 2);
					close(sockfd);
					writeSlowly(mainMenu, pos++, 2, "Connessione al server interrotta", 0, 0, 0);
					sleep(5);
					break;
				}
				recv(sockfd, buf, LINESIZE, 0);
				bzero(buf, LINESIZE);
				
				// Comunicazione tipo partita
				strcpy(buf,"single\0");
				len=strlen(buf);
				if (send(sockfd,buf,len,0)==-1) {
					writeSlowly(mainMenu, pos++, 2, "Comunicazione col server fallita", 0, 0, 0);
					shutdown(sockfd, 2);
					close(sockfd);
					writeSlowly(mainMenu, pos++, 2, "Connessione interrotta", 0, 0, 0);
					sleep(5);
					break;
				}
				bzero(buf, LINESIZE);
				
				// Ricezione conferma
				if ((len = recv(sockfd,buf,LINESIZE,0))>0) {
					buf[len]='\0';
					normalizeString(buf, 1);
					if (strcmp(buf,"CONNECTED") == 0) {
						writeSlowly(mainMenu, pos++, 2, "Connessione avvenuta", 0, 0, 0);
					} else {
						writeSlowly(mainMenu, pos++, 2, "Risposta del server non riconosciuta", 0, 0, 0);
						shutdown(sockfd, 2);
						close(sockfd);
						writeSlowly(mainMenu, pos++, 2, "Connessione interrotta", 0, 0, 0);
						sleep(5);
						break;
					}
				} else {
					shutdown(sockfd, 2);
					close(sockfd);
					writeSlowly(mainMenu, pos++, 2, "Connessione al server interrotta", 0, 0, 0);
					sleep(5);
					break;
				}
				bzero(buf, LINESIZE);
				
				writeSlowly(mainMenu, pos++, 2, "OK", 0, 0, 0);
				writeSlowly(mainMenu, 15, 2, "AVVIO PARTITA", 0, 0, 0);
				sleep(1);
				
				// Partita vera e propria
				if(initializeGame(mainMenu, status, players, multiPlayer, overNet, sockfd, connectedAsFirst)) {
					wclear(mainMenu);
					box(mainMenu, 0, 0);
					writeSlowly(mainMenu, 1, 2, "Buona partita ;-)", BETWEEN_CHARS_ME, BETWEEN_LINES_LO, AFTER_LAST_LINE_LO);
					if(!drawInGame(mainMenu, gameTable, status, cells, players, multiPlayer, &player_turn, overNet, sockfd))
						break;
				}
				
				break;
			case 3:					// Multiplayer TCP/IP
				multiPlayer=1;
				overNet = 1;
				
				// Chiedo l'indirizzo del server
				if (!drawServerAddress(mainMenu, status, address))
					break;
				
				i=0;
				pos=1;
				wclear(mainMenu);
				box(mainMenu, 0, 0);
				writeSlowly(mainMenu, pos++, 2, "Connessione al server", 0, 0, 0);
				
				server.sin_family = AF_INET;
				server.sin_addr.s_addr = inet_addr(address);
				server.sin_port = htons(SERVER_PORT);
				
				if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
					writeSlowly(mainMenu, pos++, 2, "Creazione socket fallita", 0, 0, 0);
					sleep(5);
					break;
				}
				
				// Connessione al server
				if (connect(sockfd,(struct sockaddr *)&server,sizeof server)==-1) {
					writeSlowly(mainMenu, pos++, 2, "Connessione fallita", 0, 0, 0);
					sleep(5);
					break;
				}
				
				// Ricezione messaggio di benvenuto
				if ((len = recv(sockfd,buf,LINESIZE,0))>0) {
					buf[len]='\0';
					normalizeString(buf, 1);
					if (strcmp(buf,"HI") != 0)
					{
						writeSlowly(mainMenu, pos++, 2, "Risposta del server non riconosciuta", 0, 0, 0);
						shutdown(sockfd, 2);
						close(sockfd);
						writeSlowly(mainMenu, pos++, 2, "Connessione interrotta", 0, 0, 0);
						sleep(5);
						break;
					}
					send(sockfd, "HI\n", 3, 0);
				} else {
					shutdown(sockfd, 2);
					close(sockfd);
					writeSlowly(mainMenu, pos++, 2, "Connessione al server interrotta", 0, 0, 0);
					sleep(5);
					break;
				}
				recv(sockfd, buf, LINESIZE, 0);
				bzero(buf, LINESIZE);
				
				// Comunicazione tipo partita
				strcpy(buf,"multi\0");
				len=strlen(buf);
				if (send(sockfd,buf,len,0)==-1) {
					writeSlowly(mainMenu, pos++, 2, "Comunicazione col server fallita", 0, 0, 0);
					shutdown(sockfd, 2);
					close(sockfd);
					writeSlowly(mainMenu, pos++, 2, "Connessione interrotta", 0, 0, 0);
					sleep(5);
					break;
				}
				bzero(buf, LINESIZE);
				
				// Ricezione conferma
				if ((len = recv(sockfd,buf,LINESIZE,0))>0) {
					buf[len]='\0';
					normalizeString(buf, 1);
					if (strcmp(buf,"CONNECTED") == 0) {
						writeSlowly(mainMenu, pos++, 2, "Connessione avvenuta", 0, 0, 0);
					} else {
						writeSlowly(mainMenu, pos++, 2, "Risposta del server non riconosciuta", 0, 0, 0);
						shutdown(sockfd, 2);
						close(sockfd);
						writeSlowly(mainMenu, pos++, 2, "Connessione interrotta", 0, 0, 0);
						sleep(5);
						break;
					}
				} else {
					shutdown(sockfd, 2);
					close(sockfd);
					writeSlowly(mainMenu, pos++, 2, "Connessione al server interrotta", 0, 0, 0);
					sleep(5);
					break;
				}
				bzero(buf, LINESIZE);
				
				// Creazione di una nuova partita o connessione a quella in corso
				if ((len = recv(sockfd,buf,LINESIZE,0))>0) {
					buf[len]='\0';
					normalizeString(buf, 1);
					if (strcmp(buf,"WAITING FOR PLAYER 2") == 0) {
						connectedAsFirst = TRUE;
						writeSlowly(mainMenu, pos++, 2, "Partita creata", 0, 0, 0);
						writeSlowly(mainMenu, pos++, 2, "In attesa di giocatori...", 0, 0, 0);
						
						// Imposto il timeout al socket, se recv == -1 => timeout
						// Così si può interrompere l'attesa e tornare al menu principale
						if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeOut, sizeof timeOut)) {
							writeSlowly(mainMenu, pos++, 2, "Configurazione socket fallita", 0, 0, 0);
							sleep(5);
							break;
						}
				
						// Attesa connessione del giocatore 2
						bzero(buf, LINESIZE);
						timeout(0);
						while ((len = recv(sockfd, buf, LINESIZE, 0)) == -1) {
							if (getch()==KEY_F(2)) {
								setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&noTimeOut, sizeof noTimeOut);
								shutdown(sockfd, 2);
								close(sockfd);
								writeSlowly(mainMenu, pos++, 2, "Disconnessione...", 0, 0, 0);
								sockfd=0;
								sleep(5);
								break;
							}
						}
						timeout(-1);
						
						if (sockfd == 0) {
							break;
						} else {
							// Disattivo il timeout al socket
							if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&noTimeOut, sizeof noTimeOut)) {
								writeSlowly(mainMenu, pos++, 2, "Configurazione socket fallita", 0, 0, 0);
								sleep(5);
								break;
							}
						}
						
						send(sockfd, "OK", 2, 0);
						
						// Giocatore 2 connesso
						if (len>0) {
							buf[len]='\0';
							normalizeString(buf, 1);
							if (strcmp(buf,"PLAYER 2 CONNECTED") == 0) {
								writeSlowly(mainMenu, pos++, 2, "Giocatore 2 connesso", 0, 0, 0);
							} else {
								writeSlowly(mainMenu, pos++, 2, "Risposta del server non riconosciuta", 0, 0, 0);
								shutdown(sockfd, 2);
								close(sockfd);
								writeSlowly(mainMenu, pos++, 2, "Connessione interrotta", 0, 0, 0);
								sleep(5);
								break;
							}
						} else {
							shutdown(sockfd, 2);
							close(sockfd);
							writeSlowly(mainMenu, pos++, 2, "Connessione al server interrotta", 0, 0, 0);
							sleep(5);
							break;
						}
					} else if (strcmp(buf,"PLEASE RECONNECT") == 0) {
						// Sono il giocatore 2, mi devo riconnettere per collegarmi alla partita
						writeSlowly(mainMenu, pos++, 2, "Connessione alla partita in corso", 0, 0, 0);
						shutdown(sockfd, 2);
						close(sockfd);
						if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
							writeSlowly(mainMenu, pos++, 2, "Creazione socket fallita", 0, 0, 0);
							sleep(5);
							break;
						}
						if (connect(sockfd,(struct sockaddr *)&server,sizeof server)==-1) {
							writeSlowly(mainMenu, pos++, 2, "Connessione fallita", 0, 0, 0);
							sleep(5);
							break;
						} else {
							// Connesso alla partita
							writeSlowly(mainMenu, pos++, 2, "Connesso alla partita", 0, 0, 0);
							if ((len = recv(sockfd,buf,LINESIZE,0))>0) {
								buf[len]='\0';
								normalizeString(buf, 1);
								if (strcmp(buf,"HI") != 0) {
									writeSlowly(mainMenu, pos++, 2, buf, 0, 0, 0);
									writeSlowly(mainMenu, pos++, 2, "Risposta del server non riconosciuta", 0, 0, 0);
									shutdown(sockfd, 2);
									close(sockfd);
									writeSlowly(mainMenu, pos++, 2, "Connessione interrotta", 0, 0, 0);
									sleep(5);
									break;
								}
								
								send(sockfd, "HI\n", 3, 0);
								if ((len = recv(sockfd,buf,LINESIZE,0))>0) {
									buf[len]='\0';
									normalizeString(buf, 1);
									if (strcmp(buf,"CONNECTED AS PLAYER 2") != 0) {
										writeSlowly(mainMenu, pos++, 2, buf, 0, 0, 0);
										writeSlowly(mainMenu, pos++, 2, "Risposta del server non riconosciuta", 0, 0, 0);
										shutdown(sockfd, 2);
										close(sockfd);
										writeSlowly(mainMenu, pos++, 2, "Connessione interrotta", 0, 0, 0);
										sleep(5);
										break;
									}
								}
							} else {
								shutdown(sockfd, 2);
								close(sockfd);
								writeSlowly(mainMenu, pos++, 2, "Connessione al server interrotta", 0, 0, 0);
								sleep(5);
								break;
							}
						}
					} else {
						writeSlowly(mainMenu, pos++, 2, "Risposta del server non riconosciuta", 0, 0, 0);
						shutdown(sockfd, 2);
						close(sockfd);
						writeSlowly(mainMenu, pos++, 2, "Connessione interrotta", 0, 0, 0);
						sleep(5);
						break;
					}
				} else {
					shutdown(sockfd, 2);
					close(sockfd);
					writeSlowly(mainMenu, pos++, 2, "Connessione al server interrotta", 0, 0, 0);
					sleep(5);
					break;
				}
				bzero(buf, LINESIZE);
				
				writeSlowly(mainMenu, pos++, 2, "OK", 0, 0, 0);
				writeSlowly(mainMenu, 15, 2, "AVVIO PARTITA", 0, 0, 0);
				sleep(1);
				if (!connectedAsFirst)
					send(sockfd, "READY\n", 6, 0);
					
				// Partita vera e propria
				if(initializeGame(mainMenu, status, players, multiPlayer, overNet, sockfd, connectedAsFirst)) {
					wclear(mainMenu);
					box(mainMenu, 0, 0);
					writeSlowly(mainMenu, 1, 2, "Buona partita ;-)", BETWEEN_CHARS_ME, BETWEEN_LINES_LO, AFTER_LAST_LINE_LO);
					if(!drawInGame(mainMenu, gameTable, status, cells, players, multiPlayer, &player_turn, overNet, sockfd))
						break;
					sleep(5);
				}
				
				break;
			case 4:					// Avvia server
				mvwhline(status, 0, 0, ' ', 77);
				system("xterm -e \"bash -c ./server\" &");
				mvwhline(status, 0, 0, ' ', 77);
				sleep(1);
				writeSlowly(status, 0, 0, "Server avviato", 0, 0, 0);
				break;
			case 5:					// Informazioni
				drawInformazioni(mainMenu);
				break;
			default:				// Esci
				wantToExit=1;
				break;
		}
	}
	delwin(status);
	delwin(gameTable);
	delwin(mainMenu);
	endwin();
	return 0;
}










































