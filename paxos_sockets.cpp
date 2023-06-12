#include "paxos.h"

#include <iostream>
#include <sstream>
#include <thread>

#include <unistd.h>
#include <string.h>

PaxosHandler::PaxosHandler(int PID) : myPID(PID), leaderPID(-1), tempLeader(-1)
{
    std::string filename = "saves/" + std::to_string(PID) + ".blog";
    blog = new Blockchain(filename);
    queue = new TSQueue();

    requestNum = blog->depth() + 1;

    int opt = 1;
    socklen_t addrLen = sizeof(inAddr);

    if ((serverSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "socket failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    int port = BASE_PORT + myPID;
    if (setsockopt(serverSock, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        std::cerr << "setsockopt" << std::endl;
        exit(EXIT_FAILURE);
    }
    inAddr.sin_family = AF_INET;
    inAddr.sin_addr.s_addr = INADDR_ANY;
    inAddr.sin_port = htons(port);

    // Forcefully bind socket to the port
    if (bind(serverSock, (struct sockaddr*)&inAddr, addrLen)
        < 0) {
        std::cerr << "bind failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::thread listener(&PaxosHandler::init, this);
    listener.detach();
}

PaxosHandler::~PaxosHandler() 
{
    while(!outSocks.empty()) {
        disconnect(outSocks.begin()->first);
    }
    close(serverSock);

    delete blog;
    delete queue;
}

void PaxosHandler::init()
{
    socklen_t addrSize = sizeof(serverStorage);
    // Listen on the socket,
    // with 40 max connection
    // requests queued
    if (listen(serverSock, 50) == 0) {
        std::cout << "Listening\n" << std::endl;
    } else {
        std::cerr << "Error in listening" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    while (true) {
 
        // Extract the first
        // connection in the queue
        int newSock = accept(serverSock, (struct sockaddr*)&serverStorage, &addrSize);
        int newPID = 0, rawPID = 0;

        // https://stackoverflow.com/a/9141716
        int status = recv(newSock, &rawPID, sizeof(rawPID), 0);
        if (status > 0) {
            newPID = ntohl(rawPID);

            if (inSocks.find(newPID) == inSocks.end()) {
                inSocks.emplace(newPID, newSock);

                std::thread outth(&PaxosHandler::interconnect, this, newPID);
                outth.detach();

                // tid.emplace(newPID, std::thread(&PaxosHandler::msgListener, this, newSock));
                std::thread inth(&PaxosHandler::msgListener, this, newSock);
                inth.detach();
            }
            else 
                std::cout << "Duplicate connection from " << newPID << std::endl;
        }
        else {
            std::cerr << "Error receiving PID" << std::endl;
            break;
        }
    }
}

void PaxosHandler::msgListener(int clientSock)
{   
    std::cout << "msgListener Start" << std::endl;
    char buf[BUF_SIZE];

    while (true) {
        // empty buf
        for(char &c : buf) { c = 0; }
        int bytesReceived = recv(clientSock, buf, sizeof(buf), 0);

        if (bytesReceived < 0) {
            std::cerr << "Error in receiving" << std::endl;
            break;
        }

        std::string input(buf, 0, bytesReceived); 
        
        if(input.length()) {
            std::thread th(&PaxosHandler::msgHandler, this, input, clientSock);
            th.detach();
        }
    }
}

void PaxosHandler::interconnect(int newPID)
{           
    if(newPID == myPID) {
        std::cout << "Unable to connect with self" << std::endl;
        return;
    }

    if (outSocks.find(newPID) == outSocks.end()) {
        
        std::cout << "Connecting " << newPID << "...";
        int newPort = BASE_PORT + newPID;
        struct sockaddr_in newAddr;
        int clientSock = socket(AF_INET, SOCK_STREAM, 0);
        newAddr.sin_family = AF_INET;
        newAddr.sin_port = htons(newPort);

        inet_pton(AF_INET, "127.0.0.1", &newAddr.sin_addr);
        int status = connect(clientSock, (struct sockaddr*)&newAddr, sizeof(newAddr));
        if(status < 0) {
            std::cerr << "Error in connecting to server" << std::endl;
        }
        
        // Send message of PID to connecting server
        // https://stackoverflow.com/a/9141716
        int convertedPID = htonl(myPID);
        send(clientSock, &convertedPID, sizeof(convertedPID), 0);

        // Add socket number to outsock map
        outSocks.emplace(newPID, clientSock);

        std::cout << "Done " << std::endl;
    } 
    else {
        std::cout << "Duplicate connection to " << newPID << std::endl;
    }
}

void PaxosHandler::connectAll(std::deque<std::string> targets)
{
    // Make requested connections
    for(auto s : targets) {
        int inPID = std::stoi(s);
        interconnect(inPID);
    }

    // Continue with remaining connection messages
    if (targets.size() > 1) {
        std::ostringstream ss;
        ss << "CONNECT";
        int outPID = std::stoi(targets.front());
        targets.pop_front();
        for (auto s : targets) {
            ss << sep << s;
        }
        sendMessage(outPID, ss.str());
    }
}

void PaxosHandler::disconnect(int PID)
{   
    if(inSocks.find(PID) == inSocks.end() && outSocks.find(PID) == outSocks.end()) {
        std::cout << "No links found with PID: " << PID << std::endl;
        return;
    }

    if(PID == myPID) {
        std::cout << "Unable to disconnect with self" << std::endl;
        return;
    }

    std::cout << "Disconnecting " << outSocks.begin()->first << "...";

    if (PID == leaderPID || myPID == leaderPID) {
        leaderPID = -1;
    }

    // Send disconnect message to PID
    std::ostringstream ss;
    ss << "DISCONNECT" << sep << myPID;

    sendMessage(PID, ss.str());

    // Close incoming socket
    if(inSocks.find(PID) != inSocks.end()) {
        int clientIn = inSocks[PID];
        close(clientIn);  
        inSocks.erase(inSocks.find(PID));
    }
    
    // Close outgoing socket
    if(outSocks.find(PID) != outSocks.end()) {
        outSocks.erase(outSocks.find(PID));
    }
    
    std::cout << "Done" << std::endl;
}

void PaxosHandler::broadcast(std::deque<int> targets, std::string msg)
{
    char out[BUF_SIZE];
    strcpy(out, msg.c_str());
    for(int i : targets) {
        std::cout << "SENT: " << msg << std::endl;
        send(outSocks[i], out, sizeof(out), 0);
    }
}

void PaxosHandler::broadcast(std::string msg)
{
    std::deque<int> targets;
    for(auto s : outSocks) {
        targets.push_back(s.first);
    }
    broadcast(targets, msg);
}