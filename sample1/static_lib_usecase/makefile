use_hello.out:use_hello.cpp libhello.a
	g++ use_hello.cpp  libhello.a -o use_hello.out -I./
libhello.a:hello.o
	ar crv libhello.a hello.o
hello.o:hello.cpp
	g++  hello.cpp -c 
