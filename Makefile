build:
	gcc -Wall -O3 ./src/main.c -luring -o hexserver

run:
	./hexserver -p 8081

clear:
	rm hexserver