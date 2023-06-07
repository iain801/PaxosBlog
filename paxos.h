#ifndef PAXOS_H
#define PAXOS_H

#include "blockchain.h"
#include "tsqueue.cpp"

#include <unordered_map>
#include <string>
#include <vector>
#include <math.h>

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

    // Paxos Variables
    int leaderPID;
    int ballotNum;
    std::pair<int,int> acceptNum;
    std::pair<int, std::string> acceptVal;
    std::unordered_map<int, int> ballotVotes;
    TSQueue* queue;

    Blockchain* blog;

    // Threaded Functions
    void init();
    void msgListener(int newSock);
    void msgHandler(std::string msg, int clientSock);
    
    // Paxos Functions
    void prepareBallot();
    void forwardBallot(std::string transaction);
    void acceptBallot();
    void decideBallot();

    // Returns true if new ballot is higher than current, false if not
    inline bool isDeeper(int ballot, int PID)
    {
        if (ballot > ballotNum) {
            return true;
        }
        else if (ballot == ballotNum) {
            return PID > leaderPID;
        }
        return false;
    }

    inline int numClients() { return inSocks.size(); }
    inline int majoritySize() { return (int) ceil(numClients() / 2.0); }
    inline bool isMajority(int votes) { return votes >= majoritySize(); }
    inline OP toOP(std::string s) { return (s == "POST") ? OP::POST : OP::COMMENT; }

    std::vector<std::string> split(std::string msg);

public:
    PaxosHandler(int PID);
    ~PaxosHandler();
    
    // Connection Functions
    void interconnect(int PID);
    void disconnect(int PID);

    // Message Passing Functions
    void broadcast(std::vector<int> targets, std::string msg);
    void broadcast(std::string msg);
    inline void sendMessage(int target, std::string msg) { broadcast({target}, msg); }

    // Paxos Functions
    void startPaxos(std::string transaction);

    // Printing Functions
    inline std::string printBlog() { return blog->viewAll(); }
    inline std::string printByUser(std::string user) { return blog->viewByUser(user); }
    inline std::string printComments(std::string title) {return blog->viewComments(title); }
    inline std::string printBlockchain() { return blog->str(); }
    inline std::string printQueue() { return queue->str(); }
    std::string printConnections();

};

#endif //PAXOS_H