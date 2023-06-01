#ifndef PAXOS_H
#define PAXOS_H

#include <map>
#include <string>
#include <vector>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>


#define BASE_PORT 8000

class PaxosHandler {

    PaxosHandler(int PID);
    ~PaxosHandler();

    // "Server-Side" Variables
    int serverSock, newSock;
    struct sockaddr_in inAddr;
    struct sockaddr_storage serverStorage;
    std::vector<std::thread> tid;

    // "Client-Side" Variables
    std::map<int, int> outSocks;
    
    void msgListener(int newSock);
    void msgHandler(std::string msg);

public:
    inline std::map<int, int> getOutAddrs() const { return outSocks; }
    inline struct sockaddr_in getInAddr() const { return inAddr; }

    void init();
    void interconnect(int PID);
    void disconnect(int PID);

};

#endif //PAXOS_H