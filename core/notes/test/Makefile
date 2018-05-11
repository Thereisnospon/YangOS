main.o : main.c
	gcc -c -o  main.o main.c
main.bin:main.o
	ld main.o -Ttext 0xc0001500 -e main -o main.bin 
main.S:main.c
	gcc -S -o main.S main.c