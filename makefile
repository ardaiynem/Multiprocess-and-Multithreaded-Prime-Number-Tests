all: primeP primeT

primeP: primeP.c
	gcc -Wall -g -o primeP primeP.c -lm
	
primeT: primeT.c linkedList
	gcc -Wall -g -o primeT primeT.c linkedList.c linkedList.h -lm

linkedList: linkedList.c linkedList.h
	
clean:
	rm -fr primeP primeP.o primeT primeT.o *~
