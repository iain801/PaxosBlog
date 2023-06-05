OBJS	= blockchain.o paxos.o runner.o
SOURCE	= blockchain.cpp paxos.cpp runner.cpp
HEADER	= blockchain.h paxos.h
OUT		= paxos_blog.out
CC	 	= icpx
OFLAGS	= -Ofast
FLAGS	= -c -Wall $(OFLAGS)
LFLAGS	= -lssl -lcrypto

all: $(OBJS)
	$(CC) $(OFLAGS) $(OBJS) -o $(OUT) $(LFLAGS)

blockchain.o: blockchain.cpp
	$(CC) $(FLAGS) blockchain.cpp

paxos.o: paxos.cpp
	$(CC) $(FLAGS) paxos.cpp

runner.o: runner.cpp
	$(CC) $(FLAGS) runner.cpp


clean:
	rm -f $(OBJS) $(OUT)

run: $(OUT)
	./$(OUT)