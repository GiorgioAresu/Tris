COMPILER = gcc
CCFLAGS = -lncurses

all: tris server
	make tris server
	
tris: main.o aus.o precompDefs.h
	${COMPILER} ${CCFLAGS} -o tris main.o aus.o precompDefs.h

main.o: main.h main.c
	${COMPILER} -c main.c
	
aus.o: aus.h aus.c
	${COMPILER} -c aus.c

server: server.o aus.o precompDefs.h
	${COMPILER} ${CCFLAGS} -o server server.o aus.o precompDefs.h
	
server.o: server.c server.h
	${COMPILER} -c server.c
	
clean:
	rm -f tris server *.o
