#include "paxos.h"

#include <semaphore>
#include <iostream>


PaxosHandler::PaxosHandler(int PID)
{
    int opt = 1;
    socklen_t addrLen = sizeof(inAddr);

    if ((serverSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "socket failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    int port = BASE_PORT + PID;
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
        std::cerr << "listening" << std::endl;
        exit(EXIT_FAILURE);
    }

    while (true) {
 
        // Extract the first
        // connection in the queue
        newSock = accept(serverSock, (struct sockaddr*)&serverStorage, &addrSize);
        int newPID = 0;
        recv(newSock, &newPID, sizeof(newPID), 0);

        // TODO: Connect to new node

        std::thread th(&PaxosHandler::interconnect, newPID);

        tid.push_back(std::thread(&PaxosHandler::msgListener, this, newSock));
    }
}

void PaxosHandler::msgListener(int newSock)
{
    char buf[4096];
    std::thread::id thread_id = std::this_thread::get_id();

    while (true) {
        // empty buf
        for(char &c : buf) { c = 0; }
        int bytesReceived = recv(newSock, buf, 4096, 0);

        std::string input(buf, 0, bytesReceived); 

        std::thread th(&PaxosHandler::msgHandler, this, input);
    }
}

void PaxosHandler::msgHandler(std::string msg)
{   
    std::vector<std::string> msgVector;
    size_t pos = 0;
    
    if ((pos = msg.find('(')) != std::string::npos) {
        msgVector.push_back(msg.substr(0, pos));
        msg.erase(0, pos + 1);

        std::string delimiter = ", ";
        std::string token;
        while ((pos = msg.find(delimiter)) != std::string::npos) {
            token = msg.substr(0, pos);
            msgVector.push_back(token);
            msg.erase(0, pos + delimiter.length());
        }
        msgVector.push_back(msg.substr(0, msg.length()-1));
    }

    std::string opType = msgVector.front();
    if (opType == "") {

    }
}

void PaxosHandler::interconnect(int newPID)
{
    int newPort = BASE_PORT + newPID;
    struct sockaddr_in newAddr;
    int clientSock = socket(AF_INET, SOCK_STREAM, 0);
    newAddr.sin_family = AF_INET;
    newAddr.sin_port = htons(newPort);
    inet_pton(AF_INET, "127.0.0.1", &newAddr.sin_addr);
    int status = connect(clientSock, (struct sockaddr*)&newAddr, sizeof(newAddr));

    outSocks.emplace(newPID, clientSock);
}
