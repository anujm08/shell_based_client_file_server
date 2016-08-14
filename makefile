all: client-shell get-one-file-sig get-one-file server-slow

get-one-file: get-one-file.c
	gcc -w -o get-one-file get-one-file.c

get-one-file-sig: get-one-file-sig.c
	gcc -w -o get-one-file-sig get-one-file-sig.c

server-slow: server-slow.c
	gcc -w -o server-slow server-slow.c -lpthread

client-shell: client-shell.c
	gcc -w -o client-shell client-shell.c

clean: 
	rm get-one-file
	rm get-one-file-sig
	rm server-slow
	rm client-shell
