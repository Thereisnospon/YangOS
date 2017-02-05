

img:FAT12.o	img.o
	gcc FAT12.o img.o -o img 



FAT12.o: FAT12.c FAT12.h Makefile
	gcc -c FAT12.c

img.o:img.c FAT12.h Makefile
	gcc -c img.c
