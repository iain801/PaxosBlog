#ifndef PAXOS_H
#define PAXOS_H

#include "blockchain.h"

#include <unordered_map>
#include <string>
#include <vector>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

    Blockchain blog;

public:
    PaxosHandler(int PID);
    ~PaxosHandler();
    void closeall();
    
    void interconnect(int PID);
    void disconnect(int PID);

    void broadcast(std::vector<int> targets, std::string msg);
    void broadcastAll(std::string msg);
    inline void sendMessage(int target, std::string msg) { broadcast({target}, msg); }

    inline std::unordered_map<int, int> getInSocks() { return inSocks; }
    inline std::unordered_map<int, int> getOutSocks() { return outSocks; }

    inline std::string printBlog() { return blog.viewAll(); }
    inline std::string printByUser(std::string user) { return blog.viewByUser(user); }
    inline std::string printComments(std::string title) {return blog.viewComments(title); }
    inline std::string printBlockchain() { return blog.str();}
    std::string printConnections();

};

#endif //PAXOS_H