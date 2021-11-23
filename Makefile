all: test1 test2

test1: test1.o jsonParser.o
	gcc -o test1 jsonParser.o test1.o

test2: test2.o jsonParser.o
	gcc -o test2 jsonParser.o test2.o

jsonParser.o: jsonParser.c
	gcc -o jsonParser.o -O3 -c jsonParser.c

test1.o: test1.c
	gcc -o test1.o -O3 -c test1.c

test2.o: test2.c
	gcc -o test2.o -O3 -c test2.c

clean:
	rm -rf jsonParser.o test1.o test2.o test1 test2