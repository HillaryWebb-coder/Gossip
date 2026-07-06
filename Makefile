INC_DIR = inc/

CC = gcc
CFLAGS = -Wall -I${INC_DIR} -L/usr/lib 

server: src/*.c
	mkdir -p build
	${CC} ${CFLAGS} -o build/$@ $^ -lncurses

run_server: server
	./build/server

clean:
	rm -rf build/