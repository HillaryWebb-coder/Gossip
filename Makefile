INC_DIR = .

CC = gcc
CFLAGS = -Wall -I${INC_DIR} -L/usr/lib 

server: *.c
	mkdir -p build
	${CC} ${CFLAGS} -o build/$@ $^ -lncurses

run_server: server
	./build/server

clean:
	rm -rf build/