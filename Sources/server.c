#include "server.h"

// Caratteri ANSI per la spaziatura nell'output del server, 
// i caratteri speciali li metto all'inizio del main
char tab[11] = {" [10D [10C"}; 

// Riceve una mossa da un client e la inoltra all'altro aggiornando nel frattempo
// lo stato delle celle mantenute dal server e scambiando il turno
int getMove(int fd1, int fd2, int *player_turn, struct cell cells[]) {
	char buf[LINESIZE];
	int len;
	
	bzero(buf,LINESIZE);
	
	if (*player_turn==0)
		len = recv(fd1, buf, LINESIZE, 0);
	else
		len = recv(fd2, buf, LINESIZE, 0);
		
	if (len==0)
		return 0;
		
	cells[buf[0]-49].status = *player_turn+1;
	
	if (fd1!=fd2) {
		if (*player_turn==0) {
			send(fd2, buf, len, 0);
		} else {
			send(fd1, buf, len, 0);
		}
	}
	
	*player_turn = 1-*player_turn;
	return 1;
}

// Riceve dal client il tipo di partita
int getType(int in) {
	char inputline[LINESIZE];
	bzero(inputline,LINESIZE);
	recv(in, inputline, LINESIZE, 0);
	
	normalizeString(inputline, 1);
	
	if (strcmp(inputline,"SINGLE") == 0)
		return 1;
	else if (strcmp(inputline,"MULTI") == 0)
		return 2;
	else
		return 0;
}

/* Attende che il processo figlio che sta gestendo la connessione del
 * secondo giocatore confermi al server la possibilità di accettare
 * nuovamente connessioni in ingresso. restituisce TRUE se il processo
 * figlio non ha incontrato errori, altrimenti FALSE */
bool lock(int fd, int waitingPid) {
	char inputline[LINESIZE];
	int len;
	bzero(inputline,LINESIZE);
	
	/* SUPPONGO CHE NON ARRIVINO ALTRE CONNESSIONI DURANTE LA RICONNESSIONE DEL GIOCATORE 2 */
	fprintf(stderr, "%c%sSERVER:%c%s%sSospensione accettazione nuove connessioni\n",ESC,SERVER_COLOR,ESC,DEF_COLOR,tab);
	fprintf(stderr, "%c%sSERVER:%c%s%sLa nuova connessione sara' gestita da %c%sPID%d%c%s\n",ESC,SERVER_COLOR,ESC,DEF_COLOR,tab,ESC,CLIENT_COLOR,waitingPid,ESC,DEF_COLOR);
	while ((len = read(fd, inputline, LINESIZE)) > 0)
	{
		normalizeString(inputline, 1);
		if (strcmp(inputline,"UNLOCK") == 0) {
			fprintf(stderr, "%c%sSERVER:%c%s%sAccettazione nuove connessioni ripristinata\n",ESC,SERVER_COLOR,ESC,DEF_COLOR,tab);
			return TRUE;
		} else if (strcmp(inputline,"UNLOCK_FOR_ERROR") == 0) {
			fprintf(stderr, "%c%sSERVER:%c%s%sPID%d%c%s ha terminato l'esecuzione\n",ESC,SERVER_COLOR,ESC,CLIENT_COLOR,tab,waitingPid,ESC,DEF_COLOR);
			fprintf(stderr, "%c%sSERVER:%c%s%sAccettazione nuove connessioni ripristinata\n",ESC,SERVER_COLOR,ESC,DEF_COLOR,tab);
			return FALSE;
		}
		bzero(inputline,LINESIZE);
	}
}

/* Scambia i nomi dei giocatori tra i due client (senza tenerne traccia,
 * infatti al server non interessa saperli) */
bool exchangePlayerNames(int fd1, int fd2) {
	char inputline[LINESIZE];
	
	bzero(inputline,LINESIZE);
	if (recv(fd1, inputline, LINESIZE, 0) == 0) 
		return 1;
	normalizeString(inputline, 0);
	if (send(fd2, inputline, LINESIZE, 0) == 0)
		return 1;
	
	bzero(inputline,LINESIZE);
	if (recv(fd2, inputline, LINESIZE, 0) == 0) 
		return 1;
	normalizeString(inputline, 0);
	if (send(fd1, inputline, LINESIZE, 0) == 0)
		return 1;
	
	/* Attendo il messaggio di risposta dal primo client per mantenere la
	 * sincronizzazione */
	if (recv(fd1, inputline, LINESIZE, 0) == 0) 
		return 1;	

	return 0;
}

/* Informa i client di chi deve iniziare la partita. In caso di single
 * player passare fd1 e fd2 allo stesso valore */
int setPlayerTurns(int fd1, int fd2, int turn) {
	char buf[LINESIZE];
	
	bzero(buf,LINESIZE);
	buf[0]=turn+48;
	send(fd1, buf, LINESIZE, 0);
	recv(fd1, buf, LINESIZE, 0);
	
	if (fd1 != fd2) {
		buf[0]=49-turn;
		send(fd2, buf, LINESIZE, 0);
		recv(fd2, buf, LINESIZE, 0);
	}
}

int main() {
	int type, sock, client_len, fd, waitingFd; // Tipo partita e variabili varie per il socket (file descriptor ecc)
	int player_turn;	// Numero casuale che stabilisce quale client inizia a giocare
	int p[2];			// pipe
    int yes = 1;		// per setsockopt()
	struct sockaddr_in server, client;
	pid_t pid, waitingPid = 0;
    char buf[LINESIZE];
    struct cell cells[9];
	bool multiPlayer;
	
	struct timeval timeOut; //per il timeout della connessione
	struct timeval noTimeOut;
	timeOut.tv_sec = noTimeOut.tv_sec = noTimeOut.tv_usec = 0;
	timeOut.tv_usec = 100;
	
	// inizializzo il seed del random
	srand(time(NULL));
	
	// Correggo il vettore
	tab[0] = tab[5] = 27;
	tab[10] = '\0';
	
	// Intestazione
	printf("--- CurTris server ---\n");
    printf("Press %c%sCtrl+C%c%s",ESC,RED,ESC,DEF_COLOR);
    printf(" to exit\n\n");


	/* impostazione del Transport End Point */
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		fprintf(stderr, "%c%sSERVER:%c%s%sImpostazione socket fallita%c%s\n",ESC,SERVER_COLOR,ESC,RED,tab,ESC,DEF_COLOR);
		exit(1);
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(SERVER_PORT);

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	
	
	/* binding dell'indirizzo al TEP */
	if (bind(sock, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		fprintf(stderr, "%c%sSERVER:%c%s%sBinding fallito%c%s\n",ESC,SERVER_COLOR,ESC,RED,tab,ESC,DEF_COLOR);
		exit(2);
	}

	/* impostazione del numero massimo di richieste gestibili */
    listen(sock, 5);
	
	/* gestione delle connessioni dei client */
	while(1)
	{
		initializeCells(cells);
		player_turn = rand() % 2;
		client_len = sizeof(client);
		if((fd = accept(sock, (struct sockaddr *)&client, &client_len)) < 0)
		{
			fprintf(stderr, "%c%sSERVER:%c%s%sAccettazione connessione fallita%c%s\n",ESC,SERVER_COLOR,ESC,RED,tab,ESC,DEF_COLOR);
			continue;
		}
		
		/* Controllo che il Giocatore 1 del multiplayer sia ancora in attesa
		 * e abbia terminato la connessione, in questo caso termino il processo
		 * e reimposto tutto il necessario */
		if (waitingPid!=0) {
			// Imposto il timeout al socket, se recv == -1 => timeout
			if (!setsockopt(waitingFd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeOut, sizeof timeOut)) {
				if (recv(waitingFd, buf, LINESIZE, 0) == 0) {
					fprintf(stderr, "%c%sSERVER:%c%s%sGiocatore disconnesso da %c%sPID%d%c%s\n",ESC,SERVER_COLOR,ESC,RED,tab,ESC,CLIENT_COLOR,waitingPid,ESC,DEF_COLOR);
					shutdown(waitingFd, 2);
					close(waitingFd);
					kill(waitingPid, SIGKILL);
					fprintf(stderr, "%c%sSERVER:%c%s%sPID%d%c%s terminato\n",ESC,SERVER_COLOR,ESC,CLIENT_COLOR,tab,waitingPid,ESC,DEF_COLOR);
					waitingPid=0;
				}
				setsockopt(waitingFd, SOL_SOCKET, SO_RCVTIMEO, (char *)&noTimeOut, sizeof noTimeOut);
			}
		}
		
		fprintf(stderr, "%c%sSERVER:%c%s%sInizializzazione nuova connessione\n",ESC,SERVER_COLOR,ESC,DEF_COLOR,tab);
		send(fd, "HI\n", 3, 0);
		recv(fd, buf, LINESIZE, 0);
		/* Riconoscimento tipo partita */
		send(fd, "TYPE\n", 5, 0);
		type = getType(fd);
		
		switch(type) {
			case 1:
				fprintf(stderr,"%c%sCLIENT:%c%s%sTipo partita: Giocatore singolo\n",ESC,CLIENT_COLOR,ESC,DEF_COLOR,tab);
				// SINGLE PLAYER
				
				// Crea un processo figlio per ogni connessione
				switch(fork()) {
					case -1:
						fprintf(stderr, "%c%sSERVER:%c%s%sFork fallita%c%s\n",ESC,SERVER_COLOR,ESC,RED,tab,ESC,DEF_COLOR);
						continue;
					case 0:							
						fprintf(stderr, "%c%sPID%d:%c%s%sConnessione creata\n",ESC,CLIENT_COLOR,getpid(),ESC,DEF_COLOR,tab);
						send(fd, "CONNECTED\n", 10, 0);
						setPlayerTurns(fd, fd, player_turn);
						
						// Gestisce la partita
						while(checkEnd(cells)==0 && checkVictory(NULL, cells, NULL)==0) {				
							if (player_turn!=0) {
								IAturn(NULL, cells, &player_turn, NULL, &fd);
								player_turn=1-player_turn;
							} else {
								if (!getMove(fd, fd, &player_turn, cells)) {
									fprintf(stderr, "%c%sPID%d:%c%s%sClient disconnesso%c%s\n",ESC,CLIENT_COLOR,getpid(),ESC,RED,tab,ESC,DEF_COLOR);
									shutdown(fd,2);
									close(fd);
									fprintf(stderr, "%c%sPID%d:%c%s%sIl processo verra' ora terminato\n",ESC,CLIENT_COLOR,getpid(),ESC,DEF_COLOR,tab);
									exit(1);
								}
							}
						}
						
						// Partita finita, termino
						fprintf(stderr, "%c%sPID%d:%c%s%sPartita terminata\n",ESC,CLIENT_COLOR,getpid(),ESC,DEF_COLOR,tab);
						shutdown(fd,2);
						sleep(1);
						close(fd);
						fprintf(stderr, "%c%sPID%d:%c%s%sConnessione chiusa\n",ESC,CLIENT_COLOR,getpid(),ESC,DEF_COLOR,tab);
						fprintf(stderr, "%c%sPID%d:%c%s%sIl processo verra' ora terminato\n",ESC,CLIENT_COLOR,getpid(),ESC,DEF_COLOR,tab);
						exit(0);
				}
				break;
			case 2:
				fprintf(stderr,"%c%sCLIENT:%c%s%sTipo partita: Due giocatori\n",ESC,CLIENT_COLOR,ESC,DEF_COLOR,tab);
				// MULTI PLAYER
				
				if (waitingPid == 0)
				{
					// Non c'è una partita multiplayer in attesa di giocatore2 quindi ne creo una
					
					// Userò la pipe per comunicare tra processo figlio e server
					if(pipe(p)==-1) {
						fprintf(stderr, "%c%sSERVER:%c%s%sCreazione pipe fallita%c%s\n",ESC,SERVER_COLOR,ESC,RED,tab,ESC,DEF_COLOR);
						continue;
					}
					
					switch(pid = fork()) {
						case -1:
							fprintf(stderr, "%c%sSERVER:%c%s%sFork fallita%c%s\n",ESC,SERVER_COLOR,ESC,RED,tab,ESC,DEF_COLOR);
							continue;
						case 0:
							close(p[0]); // Chiusura fd lettura pipe
							
							fprintf(stderr, "%c%sPID%d:%c%s%sConnessione creata. In attesa di giocatori\n",ESC,CLIENT_COLOR,getpid(),ESC,DEF_COLOR,tab);
							send(fd, "CONNECTED\n", 10, 0);
							send(fd, "WAITING FOR PLAYER 2\n", 21, 0);
							waitingFd=fd;
							
							// Mi blocco. Sarà il server a svegliarmi all'arrivo di una nuova richiesta di partita multigiocatore
							raise(SIGSTOP);
							
							fprintf(stderr, "%c%sPID%d:%c%s%sPronto a intercettare connessione del Giocatore 2\n",ESC,CLIENT_COLOR,getpid(),ESC,DEF_COLOR,tab);
							
							// Il server non sta più accettando connessioni quindi la intercetto io
							if((fd = accept(sock, (struct sockaddr *)&client, &client_len)) < 0) {
								fprintf(stderr, "%c%sPID%d:%c%s%sAccettazione connessione fallita%c%s\n",ESC,CLIENT_COLOR,getpid(),ESC,RED,tab,ESC,DEF_COLOR);
								fprintf(stderr, "%c%sPID%d:%c%s%sSblocco del server\n",ESC,CLIENT_COLOR,getpid(),ESC,DEF_COLOR,tab);
								write(p[1], "UNLOCK_FOR_ERROR\0", 17);
								close(p[1]);
								shutdown(fd,2);
								shutdown(waitingFd, 2);
								close(fd);
								close(waitingFd);
								fprintf(stderr, "%c%sPID%d:%c%s%sIl processo verra' ora terminato\n",ESC,CLIENT_COLOR,getpid(),ESC,DEF_COLOR,tab);
								exit(1);
							}
							
							fprintf(stderr, "%c%sPID%d:%c%s%sConnessione intercettata. Inizializzazione\n",ESC,CLIENT_COLOR,getpid(),ESC,DEF_COLOR,tab);
							send(fd, "HI\n", 3, 0);
							
							// Comunico col nuovo client
							recv(fd, buf, LINESIZE, 0);
							send(waitingFd, "PLAYER 2 CONNECTED\n", 19, 0);
							if (recv(waitingFd, buf, 2, 0)<=0) {
								fprintf(stderr, "%c%sPID%d:%c%s%sConnessione interrotta da un client%c%s\n",ESC,CLIENT_COLOR,getpid(),ESC,RED,tab,ESC,DEF_COLOR);
								fprintf(stderr, "%c%sPID%d:%c%s%sInterrompo connessione con l'altro client%c%s\n",ESC,CLIENT_COLOR,getpid(),ESC,RED,tab,ESC,DEF_COLOR);
								shutdown(fd,2);
								shutdown(waitingFd, 2);
								close(fd);
								close(waitingFd);
								fprintf(stderr, "%c%sPID%d:%c%s%sIl processo verra' ora terminato\n",ESC,CLIENT_COLOR,getpid(),ESC,DEF_COLOR,tab);
								exit(1);
							}
							send(fd, "CONNECTED AS PLAYER 2\n", 22, 0);
							
							// Informo il server che può accettare di nuovo connessioni
							fprintf(stderr, "%c%sPID%d:%c%s%sSblocco del server\n",ESC,CLIENT_COLOR,getpid(),ESC,DEF_COLOR,tab);
							write(p[1], "UNLOCK\0", 7);
							close(p[1]);
							
							recv(fd, buf, LINESIZE, 0);
							// Scambio nomi giocatori
							if (exchangePlayerNames(waitingFd, fd)) {
								fprintf(stderr, "%c%sPID%d:%c%s%sConnessione interrotta da un client%c%s\n",ESC,CLIENT_COLOR,getpid(),ESC,RED,tab,ESC,DEF_COLOR);
								fprintf(stderr, "%c%sPID%d:%c%s%sInterrompo connessione con l'altro client%c%s\n",ESC,CLIENT_COLOR,getpid(),ESC,RED,tab,ESC,DEF_COLOR);
								shutdown(fd,2);
								shutdown(waitingFd, 2);
								close(fd);
								close(waitingFd);
								fprintf(stderr, "%c%sPID%d:%c%s%sIl processo verra' ora terminato\n",ESC,CLIENT_COLOR,getpid(),ESC,DEF_COLOR,tab);
								exit(1);
							}
							setPlayerTurns(waitingFd, fd, player_turn);
							
							// Gestisco partita
							while(checkEnd(cells)==0 && checkVictory(NULL, cells, NULL)==0) {
								if (!getMove(waitingFd, fd, &player_turn, cells)) {
									fprintf(stderr, "%c%sPID%d:%c%s%sConnessione interrotta da un client%c%s\n",ESC,CLIENT_COLOR,getpid(),ESC,RED,tab,ESC,DEF_COLOR);
									fprintf(stderr, "%c%sPID%d:%c%s%sInterrompo connessione con l'altro client%c%s\n",ESC,CLIENT_COLOR,getpid(),ESC,RED,tab,ESC,DEF_COLOR);
									shutdown(fd,2);
									shutdown(waitingFd, 2);
									close(fd);
									close(waitingFd);
									fprintf(stderr, "%c%sPID%d:%c%s%sIl processo verra' ora terminato\n",ESC,CLIENT_COLOR,getpid(),ESC,DEF_COLOR,tab);
									exit(1);
								}
							}
							
							// Partita finita, termino
							fprintf(stderr, "%c%sPID%d:%c%s%sPartita terminata\n",ESC,CLIENT_COLOR,getpid(),ESC,DEF_COLOR,tab);
							shutdown(fd,2);
							shutdown(waitingFd, 2);
							close(fd);
							close(waitingFd);
							fprintf(stderr, "%c%sPID%d:%c%s%sConnessione chiusa\n",ESC,CLIENT_COLOR,getpid(),ESC,DEF_COLOR,tab);
							fprintf(stderr, "%c%sPID%d:%c%s%sIl processo verra' ora terminato\n",ESC,CLIENT_COLOR,getpid(),ESC,DEF_COLOR,tab);
							exit(0);
						default:
							close(p[1]); // Chiusura fd scrittura pipe
							
							// Imposto le variabili per permettere di ridirezionare la connessione alla prossima richiesta
							waitingPid=pid;
							waitingFd=fd;
					}
				} else {
					fprintf(stderr, "%c%sSERVER:%c%s%sReindirizzamento connessione a %c%sPID%d%c%s\n",ESC,SERVER_COLOR,ESC,DEF_COLOR,tab,ESC,CLIENT_COLOR,waitingPid,ESC,DEF_COLOR);
					
					send(fd, "CONNECTED\n", 10, 0);
					
					// Risveglia processo figlio
					kill(waitingPid, SIGCONT);
					send(fd, "PLEASE RECONNECT\n", 16, 0);
					
					/* Non mi interessa sapere il valore restituito, in
					 * ogni caso devo considerare "andato" waitingPid,
					 * sia che abbia iniziato una partita, che terminato
					 * l'esecuzione. In entrambi i casi non sarà più in
					 * grado di gestire nuove connessioni quindi dovrò
					 * rifare il fork alla prossima richiesta... */
					lock(p[0],waitingPid);
					close(p[0]);
					
					/* ... quindi waitingPid va settato a 0 */
					waitingPid=0;
				}
				break;
			default:
				fprintf(stderr, "%c%sSERVER:%c%s%sTipo partita non riconosciuto (deve essere SINGLE o MULTI)%c%s\n",ESC,SERVER_COLOR,ESC,RED,tab,ESC,DEF_COLOR);
				send(fd, "CONNECTED\n", 3, 0);
				continue;
		}
	}
}
