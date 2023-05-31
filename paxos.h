#ifndef PAXOS_H
#define PAXOS_H

#include <map>
#include <string>

#include <sys/socket.h>

#define sock_t int

class PaxosHandler {


    PaxosHandler();
    ~PaxosHandler();

    std::map<int, sock_t> outsocks;
    sock_t insock;

    

};

#endif //PAXOS_H