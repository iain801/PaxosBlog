OBJS	= blockchain.o paxos.o paxos_sockets.o runner.o tsqueue.o
SOURCE	= blockchain.cpp paxos.cpp paxos_sockets.cpp runner.cpp tsqueue.cpp
HEADER	= blockchain.h paxos.h
OUT		= paxos_blog.out
CC	 	= icpx
OFLAGS	= -g -O0
FLAGS	= -c -Wall $(OFLAGS)
LFLAGS	= -lssl -lcrypto

all: $(OBJS)
	$(CC) $(OFLAGS) $(OBJS) -o $(OUT) $(LFLAGS)

blockchain.o: blockchain.cpp
	$(CC) $(FLAGS) blockchain.cpp

paxos.o: paxos.cpp
	$(CC) $(FLAGS) paxos.cpp

paxos_sockets.o: paxos_sockets.cpp
	$(CC) $(FLAGS) paxos_sockets.cpp

tsqueue.o: tsqueue.cpp
	$(CC) $(FLAGS) tsqueue.cpp

runner.o: runner.cpp
	$(CC) $(FLAGS) runner.cpp


clean:
	rm -f $(OBJS) $(OUT)
