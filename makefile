OBJS	= blockchain.o paxos.o runner.o
SOURCE	= blockchain.cpp paxos.cpp runner.cpp
HEADER	= blockchain.h paxos.h
OUT		= paxos_blog.out
CC	 	= icpx
FLAGS	= -c -Wall -O2 
LFLAGS	= -lssl -lcrypto -lpthread -fsycl

all: $(OBJS)
	$(CC) $(OBJS) -o $(OUT) $(LFLAGS)

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