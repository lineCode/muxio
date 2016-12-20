.PHONY:all clean mingw macosx linux

LDFLAG:=-lpthread

mingw:LDFLAG+=-lws2_32

linux macosx mingw:all

all:testserver testclient

testserver: buffer.o coroutine.o muxio.o socket.o test_server.o
	g++ -std=c++0x -I../lib/ -Wall -g -o $@ $^ $(LDFLAG)

testclient: buffer.o coroutine.o muxio.o socket.o test_client.o
	g++ -std=c++0x -I../lib/ -Wall -g -o $@ $^ $(LDFLAG)

%.o:%.c
	gcc -Wall -g -c $<

%.o:%.cc
	g++ -std=c++0x -Wall -g -c $<

%.o:%.cpp
	g++ -std=c++0x -I../lib/ -Wall -g -c $<


clean:
	-rm *.o
	-rm testserver
	-rm testclient

