all: client server

client: get-one-file.c get-one-file-sig.c
	gcc -w -o get-one-file get-one-file.c
	gcc -w -o get-one-file-sig get-one-file-sig.c

server: server-slow.c
	gcc -w -o server-slow server-slow.c -lpthread

clean: 
	rm get-one-file
	rm server-slow