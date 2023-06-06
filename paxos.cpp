#include "paxos.h"

#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

#include <unistd.h>
#include <string.h>

#define BUF_SIZE 2048

PaxosHandler::PaxosHandler(int PID) : myPID(PID)
{
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
        int status = read(newSock, &rawPID, sizeof(rawPID));
        if (status > 0) {
            newPID = ntohs(rawPID);

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

void PaxosHandler::msgHandler(std::string msg, int clientSock)
{   
    // 3 second message passing delay
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "RECIEVED: " << msg << std::endl;

    // Parse message into vector
    std::vector<std::string> msgVector;
    size_t pos = 0;
    
    // https://stackoverflow.com/a/14266139
    std::string delimiter = ", ";
    std::string token;
    while ((pos = msg.find(delimiter)) != std::string::npos) {
        token = msg.substr(0, pos);
        msgVector.push_back(token);
        msg.erase(0, pos + delimiter.length());
    }
    msgVector.push_back(msg.substr(0, msg.length()));

    // Pop operation type off front
    std::string opType = msgVector.front();
    msgVector.erase(msgVector.begin());

    if (opType == "DISCONNECT") {

        int target = stoi(msgVector.front());
        
        // Close outgoing socket
        // int clientOut = outSocks[target];
        // close(clientOut);
        outSocks.erase(outSocks.find(target));
        
        // Close incoming socket
        int clientIn = inSocks[target];
        close(clientIn);
        inSocks.erase(inSocks.find(target));

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
        int convertedPID = htons(myPID);
        write(clientSock, &convertedPID, sizeof(convertedPID));

        // Add socket number to outsock map
        outSocks.emplace(newPID, clientSock);

        std::cout << "Done " << std::endl;
    } 
    else {
        std::cout << "Duplicate connection to " << newPID << std::endl;
    }
}

void PaxosHandler::disconnect(int PID)
{   
    if(PID == myPID) {
        std::cout << "Unable to disconnect with self" << std::endl;
        return;
    }
    std::cout << "Disconnecting " << outSocks.begin()->first << "...";

    // Send disconnect message to PID
    std::ostringstream ss;
    ss << "DISCONNECT, " << myPID;

    sendMessage(PID, ss.str());

    // Close incoming socket
    int clientIn = inSocks[PID];
    close(clientIn);  
    inSocks.erase(inSocks.find(PID));
    
    // Close outgoing socket
    // int clientOut = outSocks[PID];
    // close(clientOut);
    outSocks.erase(outSocks.find(PID));
    
    std::cout << "Done" << std::endl;
}

void PaxosHandler::broadcast(std::vector<int> targets, std::string msg)
{
    char out[BUF_SIZE];
    strcpy(out, msg.c_str());
    for(int i : targets) {
        send(outSocks[i], out, sizeof(out), 0);
    }
}

void PaxosHandler::broadcastAll(std::string msg)
{
    std::vector<int> targets;
    for(auto s : outSocks) {
        targets.push_back(s.first);
    }
    broadcast(targets, msg);
}

std::string PaxosHandler::printConnections() 
{   
    std::ostringstream ss;
    ss << "In Socks: \n";
    for (auto s : inSocks) {
        ss << "PID " << s.first << ": " << s.second << "\n";
    }
    ss << "Out Socks: \n";
    for (auto s : outSocks) {
        ss << "PID " << s.first << ": " << s.second << "\n";
    }
    ss << std::endl;
    return ss.str();
}
