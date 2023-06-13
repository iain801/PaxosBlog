OBJS	= blockchain.o paxos.o paxos_sockets.o runner.o tsqueue.o
SOURCE	= blockchain.cpp paxos.cpp paxos_sockets.cpp runner.cpp tsqueue.cpp
HEADER	= blockchain.h paxos.h
OUT		= paxos_blog.out
CC	 	= icpx
OFLAGS	= -g -O0
FLAGS	= -c -Wall $(OFLAGS)
LFLAGS	= -lssl -lcrypto

OBJDIR	= out/

all: $(OBJS)
	$(CC) $(OFLAGS) $(OBJDIR)*.o -o $(OUT) $(LFLAGS)

blockchain.o: blockchain.cpp
	$(CC) $(FLAGS) blockchain.cpp -o $(OBJDIR)blockchain.o

paxos.o: paxos.cpp
	$(CC) $(FLAGS) paxos.cpp -o $(OBJDIR)paxos.o

paxos_sockets.o: paxos_sockets.cpp
	$(CC) $(FLAGS) paxos_sockets.cpp -o $(OBJDIR)paxos_sockets.o

tsqueue.o: tsqueue.cpp
	$(CC) $(FLAGS) tsqueue.cpp -o $(OBJDIR)tsqueue.o

runner.o: runner.cpp
	$(CC) $(FLAGS) runner.cpp -o $(OBJDIR)runner.o

clean:
	rm -f $(OUT) $(OBJDIR)*.o saves/*.blog
	
