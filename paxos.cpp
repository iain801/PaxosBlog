#include "paxos.h"

#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

#include <unistd.h>
#include <string.h>

#define BUF_SIZE 2048
#define TIME_OUT 15

PaxosHandler::PaxosHandler(int PID) : myPID(PID), leaderPID(-1), tempLeader(-1)
{
    blog = new Blockchain();
    queue = new TSQueue();

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

std::deque<std::string> PaxosHandler::split(std::string msg)
{
    // Parse message into deque
    std::deque<std::string> msgVector;
    size_t pos = 0;
    
    // https://stackoverflow.com/a/14266139
    std::string token;
    while ((pos = msg.find(sep)) != std::string::npos) {
        token = msg.substr(0, pos);
        msgVector.push_back(token);
        msg.erase(0, pos + sep.length());
    }
    msgVector.push_back(msg.substr(0, msg.length()));

    return msgVector;
}

//TODO: fix weird one-way connection problem
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

    if (PID == leaderPID) {
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

std::string PaxosHandler::printConnections() 
{   
    std::ostringstream ss;
    ss << "In Socks: \n";
    for (auto s : inSocks) {
        ss << "PID " << s.first << ": " << s.second << std::endl;
    }
    ss << "Out Socks: \n";
    for (auto s : outSocks) {
        ss << "PID " << s.first << ": " << s.second << std::endl;
    }
    return ss.str();
}

void PaxosHandler::msgHandler(std::string msg, int clientSock)
{   
    // 3 second message passing delay
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "RECIEVED: " << msg << std::endl;

    // Parse message into deque
    auto msgVector = split(msg);

    // Pop operation type off front
    std::string opType = msgVector.front();
    msgVector.pop_front();

    int inBallot, inRequest, inPID, inDepth;

    if (opType == "DISCONNECT") {

        inPID = std::stoi(msgVector.front());
        
        if(inPID == leaderPID)
            leaderPID = -1;
            
        // Close incoming socket
        if(inSocks.find(inPID) != inSocks.end()) {
            int clientIn = inSocks[inPID];
            close(clientIn);  
            inSocks.erase(inSocks.find(inPID));
        }
        
        // Close outgoing socket
        if(outSocks.find(inPID) != outSocks.end()) {
            outSocks.erase(outSocks.find(inPID));
        }

    }
    else if (opType == "CONNECT") {
        connectAll(msgVector);
    }
    else if (opType == "PREPARE") {
        inBallot = std::stoi(msgVector[0]);
        inPID = std::stoi(msgVector[1]);
        inDepth = std::stoi(msgVector[2]);
        
        // return if valid leader
        if(leaderPID != -1) {
            std::ostringstream ss;
            ss << "LEADER" << sep << ballotNum << sep << leaderPID << sep << blog->depth() << sep << myPID;
            sendMessage(inPID, ss.str());
            return;
        }

        if (inDepth >= blog->depth()) {
            if ((inBallot > ballotNum) || 
                (inBallot == ballotNum && inPID > tempLeader)) {
                
                tempLeader = inPID;
                ballotNum = inBallot;

                std::ostringstream ss;
                ss << "PROMISE" << sep << inBallot << sep << inPID << sep << inDepth << sep << myPID;
                sendMessage(inPID, ss.str());
            }
        }
    }
    else if (opType == "PROMISE") {
        inBallot = std::stoi(msgVector[0]);
        if(ballotVotes.find(inBallot) != ballotVotes.end())
            ++ballotVotes[inBallot];
    }
    else if (opType == "LEADER") {
        inBallot = std::stoi(msgVector[0]);
        inPID = std::stoi(msgVector[1]);
        inDepth = std::stoi(msgVector[2]);
        if (inDepth >= blog->depth()) {
            if ((leaderPID == -1) || (inBallot > ballotNum) || 
                (inBallot == ballotNum && inPID > leaderPID)) {
                
                leaderPID = inPID;
                ballotNum = inBallot;
            }
        }
    }
    else if (opType == "ACCEPTED") {
        inRequest = std::stoi(msgVector[0]);
        if(requestVotes.find(inRequest) != requestVotes.end())
            ++requestVotes[inRequest];
    }
    else if (opType == "ACCEPT") {   
        inRequest = std::stoi(msgVector[0]);
        inPID = std::stoi(msgVector[1]);
        inDepth = std::stoi(msgVector[2]);
        msgVector.erase(msgVector.begin(), msgVector.begin() + 3);

        if(acceptLocks.find(inRequest) == acceptLocks.end())
            acceptLocks.emplace(inRequest, false);

        if (inDepth >= blog->depth() && requestNum <= inRequest) {
            requestNum = inRequest;

            Block* newBlock = blog->makeBlock(msgVector[1], msgVector[2], msgVector[3], msgVector[4]);

            if(!newBlock->validNonce(std::stoi(msgVector[5]))) {
                std::cout << "Proposed Block <" << inRequest << sep << inPID << "> has invalid nonce" << std::endl;
                delete newBlock;
                return;
            }

            if (newBlock->getPrevHash() != msgVector[0]) {
                std::cout << "Proposed Block <" << inRequest << sep << inPID << "> has incorrect hash pointer" << std::endl;
                std::cout << "Expected " << newBlock->getPrevHash() << " received " << msgVector[0] << std::endl;
                delete newBlock;
                return;
            }

            //Return if request had been accepted
            if(acceptLocks[inRequest]) {
                delete newBlock;
                return;
            }
            acceptLocks[inRequest] = true; // Lock from accepting any further requests w/ that number

            std::ostringstream ss;
            ss << "ACCEPTED" << sep << inRequest << sep << inPID << sep << inDepth << sep << myPID;
            sendMessage(inPID, ss.str());
        }
    }
    else if (opType == "DECIDE") {
        inRequest = std::stoi(msgVector[0]);
        inPID = std::stoi(msgVector[1]);
        inDepth = std::stoi(msgVector[2]);
        msgVector.erase(msgVector.begin(), msgVector.begin() + 3);

        // Pop off prev_hash
        msgVector.pop_front();

        if (inDepth >= blog->depth()) {
            if ((leaderPID == -1) || (inRequest > requestNum) || 
                (inRequest == requestNum && inPID > leaderPID)) {
                
                leaderPID = inPID;
                requestNum = inRequest;
            }

            Block* newBlock = blog->makeBlock(msgVector[0], msgVector[1], msgVector[2], msgVector[3]);
    
            if(!newBlock->validNonce(std::stoi(msgVector[4]))) {
                std::cout << "Invalid nonce in " << newBlock->str() << std::endl;
                delete newBlock;
                return;
            }
            
            if (!blog->addBlock(newBlock)) {
                std::cout << "Error adding " << newBlock->str() << std::endl;
                delete newBlock;
                return;
            }
        }
    }
    else if (opType == "I WIN") {
        inBallot = std::stoi(msgVector[0]);
        inPID = std::stoi(msgVector[1]);
        inDepth = std::stoi(msgVector[2]);

        if (inDepth >= blog->depth()) {
            if ((leaderPID == -1) || (inBallot > ballotNum) || 
                (inBallot == ballotNum && inPID > leaderPID)) {
                
                leaderPID = inPID;
                ballotNum = inBallot;
            }
        }
    }
    else if (opType == "FORWARD") {
        inPID = std::stoi(msgVector[0]);
        msgVector.pop_front();
        if (leaderPID != -1 && leaderPID != myPID) {
            std::ostringstream ss;
            ss << "LEADER" << sep << ballotNum << sep << leaderPID << sep << blog->depth() << sep << myPID;
            sendMessage(inPID, ss.str());
        }
        std::ostringstream ss;
        ss << msgVector[0] << sep << msgVector[1] << sep << msgVector[2] << sep << msgVector[3];
        startPaxos(ss.str());
    }
    else {
        std::cout << "Invalid message" << std::endl;
    }
}

void PaxosHandler::prepareBallot() 
{
    int thisBallot = ++ballotNum;
    std::ostringstream ss;
    ss << "PREPARE" << sep << thisBallot << sep << myPID << sep << blog->depth();

    ballotVotes.emplace(thisBallot, 1);

    broadcast(ss.str());

    int timer = 0;
    while (!isMajority(ballotVotes[thisBallot])) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // timeout timer
        if(++timer > TIME_OUT) { 
            std::cout << "Timing out ballot <" << thisBallot << sep << myPID << ">" << std::endl;
            ballotVotes.erase(ballotVotes.find(thisBallot));
            return; 
        }
        if(leaderPID != -1) {
            std::cout << "Another leader elected: " << leaderPID << std::endl;
            ballotVotes.erase(ballotVotes.find(thisBallot));
            return; 
        }
    }

    ballotVotes.erase(ballotVotes.find(thisBallot));

    // Won election
    if(leaderPID == -1) {
        leaderPID = myPID;
        std::cout << "Won Election <" << thisBallot << sep << leaderPID << ">" << std::endl;

        std::ostringstream iwin;
        iwin << "I WIN" << sep << thisBallot << sep << myPID << sep << blog->depth();

        broadcast(iwin.str());

        std::thread requestThread(&PaxosHandler::acceptRequests, this);
        requestThread.detach();
    }
}

void PaxosHandler::startPaxos(std::string transaction)
{
    // Wait until successful leader election
    while (leaderPID == -1) {
        prepareBallot();
        std::this_thread::sleep_for(std::chrono::seconds(6));
    }

    if (leaderPID == myPID) {
        queue->push(transaction);
    }
    else {
        forwardRequest(transaction);
    }

}

void PaxosHandler::forwardRequest(std::string transaction)
{
    std::ostringstream ss;
    ss << "FORWARD" << sep << myPID << sep << transaction;

    sendMessage(leaderPID, ss.str());
}

void PaxosHandler::acceptRequests()
{   
    while (leaderPID == myPID) {
        auto transaction = queue->pop();
        auto transArray = split(transaction);
        Block* newBlock = blog->makeBlock(transArray[0], transArray[1], transArray[2], transArray[3]);
        int thisRequest = ++requestNum;

        std::ostringstream ss;
        ss  << "ACCEPT" << sep << thisRequest << sep << myPID << sep << blog->depth() << sep
            << newBlock->str(false);

        requestVotes.emplace(thisRequest, 1);

        broadcast(ss.str());

        int timer = 0;
        while (!isMajority(requestVotes[thisRequest])) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // timeout timer
            if(++timer > TIME_OUT) { 
                std::cout << "Timing out request <" << thisRequest << sep << myPID << ">" << std::endl;
                requestVotes.erase(requestVotes.find(thisRequest));
                delete newBlock;
                newBlock = nullptr;
                break; 
            }
        }

        // If timed out, then newBlock was freed, so we can skip to next in queue
        if(!newBlock)
            continue;

        requestVotes.erase(requestVotes.find(thisRequest));

        std::cout << "Decided request <" << thisRequest << sep << leaderPID << ">" << std::endl;
        decideRequest(thisRequest, newBlock);
    }
}

void PaxosHandler::decideRequest(int thisRequest, Block* newBlock)
{
    std::ostringstream ss;
    ss  << "DECIDE" << sep << thisRequest << sep << myPID << sep << blog->depth() << sep << newBlock->str(false);

    broadcast(ss.str());

    blog->addBlock(newBlock);
}