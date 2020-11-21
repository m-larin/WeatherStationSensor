CC=gcc                  
CFLAGS=-I.
DEPS =                  

all: compile 
	gcc weather-server.o bmp085.o am2322.o -o weather-server -lwiringPi -lmysqlclient

compile: clean 
	gcc -I . -c weather-server.cpp bmp085.cpp am2322.cpp

clean:
	rm -rf *.o
	rm -rf weather-server
