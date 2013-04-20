all:
	gcc -pthread Thread/Thread.c -o Thread.out
r:
	make all
	./Thread.out