all: client-shell get-one-file-sig get-one-file server-slow

get-one-file: get-one-file.c
	gcc -o get-one-file get-one-file.c

get-one-file-sig: get-one-file-sig.c
	gcc -o get-one-file-sig get-one-file-sig.c

server-slow: server-slow.c
	gcc -o server-slow server-slow.c -lpthread

client-shell: client-shell.c functions.h
	g++ -o -w client-shell -std=c++11 client-shell.c -lpthread

clean: 
	rm get-one-file
	rm get-one-file-sig
	rm server-slow
	rm client-shell
