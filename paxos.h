#ifndef PAXOS_H
#define PAXOS_H

#include <unordered_map>
#include <string>
#include <vector>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>


#define BASE_PORT 8000

class PaxosHandler {
    
    int myPID;

    // "Server-Side" Variables
    int serverSock;
    struct sockaddr_in inAddr;
    struct sockaddr_storage serverStorage;
    std::unordered_map<int, int> inSocks;

    // "Client-Side" Variables
    std::unordered_map<int, int> outSocks;

    void init();
    void msgListener(int newSock);
    void msgHandler(std::string msg, int clientSock);

public:
    PaxosHandler(int PID);
    ~PaxosHandler();
    
    void interconnect(int PID);
    void disconnect(int PID);

    inline std::unordered_map<int, int> getInSocks() { return inSocks; }
    inline std::unordered_map<int, int> getOutSocks() { return outSocks; }

};

#endif //PAXOS_H