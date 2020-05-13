server.out:server.cc libwrap.a
	g++ server.cc  libwrap.a -o server.out -I./include/ -D DEBUG -g  -O0 -lpthread
client.out:client.cc
	g++ client.cc -o client.out -g -Wall -O0
libwrap.a:wrap.o
	ar crv libwrap.a wrap.o
wrap.o:
	g++ -c ./wrap.cc -I./include/

.PHONY:clean
clean:
	rm *.out *.o *.a -f
