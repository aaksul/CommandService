output: server.o client.o

server.o: server.c
	gcc -g -Wall server.c -o server.o
client.o: client.c
	gcc -g -Wall client.c -o client.o
clean:
	rm *.o
