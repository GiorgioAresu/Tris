#ifndef PRECOMPDEFS_H
#define PRECOMPDEFS_H
	
	// Per writeSlowly
	#define BETWEEN_CHARS_LO		0
	#define BETWEEN_CHARS_ME		15000
	#define BETWEEN_CHARS_HI		50000
	#define BETWEEN_LINES_LO		0
	#define BETWEEN_LINES_HI		500000
	#define AFTER_LAST_LINE_LO		0
	#define AFTER_LAST_LINE_ME		150000
	#define AFTER_LAST_LINE_HI		750000

	// Per la posizione delle cells all'interno della griglia di gioco
	#define VERTICAL_1				0
	#define VERTICAL_2				6
	#define VERTICAL_3				12
	#define HORIZONTAL_1			0
	#define HORIZONTAL_2			12
	#define HORIZONTAL_3			24
	
	// Colori
	#define BG_COLOR				COLOR_GREEN
	#define FG_COLOR1				COLOR_BLACK
	#define FG_COLOR2				COLOR_RED
	#define TXT_COLOR				COLOR_WHITE
	
	// Soglie IA
	#define PUT_IN_CENTER_PERCENT	75
	#define PUT_IN_CORNER_PERCENT	95
	
	// Titolo e tempistiche main
	#define TITLE " CurTris 0.6 (CURsesTRIS) "
	#define TITLE_SCROLLING_SPEED	50000
	#define SIGNS_SPEED_IN_LOADING	200000
	/* Durante il disegno dell'interfaccia iniziale viene riempita la griglia
	 * SIGNS_SPEED_IN_LOADING è l'intervallo tra ogni simbolo */

	// Connessione client-server
	#define SERVER_PORT 3000
	#define LINESIZE 80

	// Varie
	#define ESC 27 // Carattere di Escape ANSI
	#define SERVER_COLOR "[34m" // BLU
	#define CLIENT_COLOR "[36m" // CIANO
	#define DEF_COLOR    "[39m" // DEFAULT
	#define RED	         "[31m"
	
#endif
