all: csocket.so cepoll.so
.PNONY: all

#	gcc -O2 -fpic -D_GNU_SOURCE -std=c99 -I/usr/local/include/lua -c csocket.c
csocket.o: csocket.c
	gcc -O2 -fpic -I/usr/local/include/lua -c csocket.c

csocket.so: csocket.o
	gcc -O -fpic -shared -o csocket.so csocket.o

cpoll.o: cpoll.c
	gcc -O2 -fpic -I/usr/local/include/lua -c cpoll.c

cepoll.so: cpoll.o
	gcc -O -fpic -shared -o cepoll.so cpoll.o

clean:
	rm -f *.o *.so
