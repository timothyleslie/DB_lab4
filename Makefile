#makefile
test: test.c extmem.c
	gcc -o test test.c extmem.c -I.

lab4: lab4.c extmem.c
	gcc -o lab4 lab4.c extmem.c -I.