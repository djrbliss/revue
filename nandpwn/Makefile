
all: nandpwn

nandpwn: nandpwn.o kstack.o leak.o nand.o
	gcc nandpwn.o kstack.o leak.o nand.o -o nandpwn -m32

nandpwn.o: nandpwn.c
	gcc -c nandpwn.c -m32

leak.o: leak.c
	gcc -c leak.c -m32

kstack.o: kstack.c kstack.h
	gcc -c kstack.c -m32

nand.o: nand.c
	gcc -c nand.c -m32

clean:
	rm -rf *.o nandpwn
