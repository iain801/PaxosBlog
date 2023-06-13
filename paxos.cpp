#include "paxos.h"

#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

#include <unistd.h>
#include <string.h>

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

        if ((inDepth >= blog->depth()) && ((inBallot > ballotNum) || 
            (inBallot == ballotNum && inPID > tempLeader))) {
            
            tempLeader = inPID;
            ballotNum = inBallot;

            std::ostringstream ss;
            ss << "PROMISE" << sep << inBallot << sep << inPID << sep << blog->depth() << sep << myPID;
            sendMessage(inPID, ss.str());
        }

        else if ((inDepth >= blog->depth()) && (myPID > tempLeader)) {
            prepareBallot();
        }
    }
    else if (opType == "PROMISE") {
        inBallot = std::stoi(msgVector[0]);
        inPID = std::stoi(msgVector[3]);
        inDepth = std::stoi(msgVector[2]);
        if(ballotVotes[inBallot] > 0) {
            ++ballotVotes[inBallot];
        }
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
        inPID = std::stoi(msgVector[3]);
        inDepth = std::stoi(msgVector[2]);
        
        if(requestVotes[inRequest] > 0) {
            // if acceptor is shallower, then send the decided messages for all unknown nodes
            if(inDepth < blog->depth()) {
                catchUpNode(inPID, inDepth);
            }
            ++requestVotes[inRequest];
        }
        
    }
    else if (opType == "ACCEPT") {   
        inRequest = std::stoi(msgVector[0]);
        inPID = std::stoi(msgVector[1]);
        inDepth = std::stoi(msgVector[2]);
        msgVector.erase(msgVector.begin(), msgVector.begin() + 3);
        
        if(inDepth < blog->depth()) {
            catchUpNode(inPID, inDepth);
        }

        if(acceptLocks.find(inRequest) == acceptLocks.end())
            acceptLocks.emplace(inRequest, false);

        if (inDepth >= blog->depth() && requestNum <= inRequest) {
            requestNum = inRequest;

            Block* newBlock = blog->makeBlock(msgVector[1], msgVector[2], msgVector[3], msgVector[4]);

            if (!newBlock) {
                std::cout << "Post not found for: " << msgVector[3] << std::endl;
                return;
            }

            if(!newBlock->validNonce(std::stoi(msgVector[5]))) {
                std::cout << "Proposed Block <" << inRequest << sep << inPID << "> has invalid nonce" << std::endl;
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
            ss << "ACCEPTED" << sep << inRequest << sep << inPID << sep << blog->depth() << sep << myPID;
            sendMessage(inPID, ss.str());
        }
    }
    else if (opType == "DECIDE") {
        inRequest = std::stoi(msgVector[0]);
        inPID = std::stoi(msgVector[1]);
        inDepth = std::stoi(msgVector[2]);
        msgVector.erase(msgVector.begin(), msgVector.begin() + 3);

        if (inDepth <= blog->depth())
            return;

        while (inDepth != blog->depth() + 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::unique_lock<std::mutex> lock(decideMutex);
        if (inDepth == blog->depth() + 1) {
            if ((leaderPID == -1) || (inRequest > requestNum) || 
                (inRequest == requestNum && inPID > leaderPID)) {
                
                leaderPID = inPID;
                requestNum = inRequest;
            }

            Block* newBlock = blog->makeBlock(msgVector[1], msgVector[2], msgVector[3], msgVector[4]);

            if (!newBlock) {
                std::cout << "Post not found for: " << msgVector[3] << std::endl;
                return;
            }
    
            if(!newBlock->validNonce(std::stoi(msgVector[5]))) {
                std::cout << "Invalid nonce in " << newBlock->str() << std::endl;
                delete newBlock;
                return;
            }

            if (newBlock->getPrevHash() != msgVector[0]) {
                std::cout << "Decided Block <" << inRequest << sep << inPID << "> has incorrect hash pointer" << std::endl;
                std::cout << "Expected " << newBlock->getPrevHash() << " received " << msgVector[0] << std::endl;
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

    tempLeader = myPID;

    broadcast(ss.str());

    int timer = 0;
    while (!isMajority(ballotVotes[thisBallot])) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // timeout timer
        if(++timer > TIME_OUT) { 
            std::cout << "TIMEOUT <" << thisBallot << sep << myPID << ">" << std::endl;
            ballotVotes[thisBallot] = -1;
            return; 
        }
        if(leaderPID != -1) {
            std::cout << "Another leader elected: " << leaderPID << std::endl;
            ballotVotes[thisBallot] = -1;
            return; 
        }
        if(tempLeader > myPID) {
            std::cout << "Higher ballot recieved: " << tempLeader << std::endl;
            ballotVotes[thisBallot] = -1;
            return; 
        }
    }

    ballotVotes[thisBallot] = -1;

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
        auto transaction = queue->top();
        auto transArray = split(transaction);
        Block* newBlock = blog->makeBlock(transArray[0], transArray[1], transArray[2], transArray[3]);
        int thisRequest = ++requestNum;

        if (!newBlock) {
            std::cout << "Post not found for: " << transArray[2] << std::endl;
            queue->pop();
            continue;
        }

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
                std::cout << "TIMEOUT <" << thisRequest << sep << myPID << ">" << std::endl;
                requestVotes[thisRequest] = -1;
                delete newBlock;
                newBlock = nullptr;
                break; 
            }
        }

        // If timed out, then newBlock was freed, so we can skip to next in queue
        if(!newBlock) {
            queue->pop();
            continue;
        }

        // Wait for a bit to catch any remaining accepted messages
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        requestVotes[thisRequest] = -1;

        std::cout << "Decided request <" << thisRequest << sep << leaderPID << ">" << std::endl;
        decideRequest(thisRequest, newBlock);

        queue->pop();
    }

    // Forward/restart paxos for all incompleted transactions
    while(!queue->empty()) {
        auto transaction = queue->pop();
        startPaxos(transaction);
    }
}

void PaxosHandler::decideRequest(int thisRequest, Block* newBlock)
{
    // Isn't strictly nessecisary, but just to be safe
    std::unique_lock<std::mutex> lock(decideMutex);
    blog->addBlock(newBlock);

    std::ostringstream ss;
    ss  << "DECIDE" << sep << thisRequest << sep << myPID << sep << blog->depth() << sep << newBlock->str(false);

    broadcast(ss.str());
}

void PaxosHandler::catchUpNode(int clientPID, int clientDepth)
{
    std::ostringstream ss;
    
    for (int i = clientDepth + 1; i <= blog->depth(); ++i) {
        // request num of -1 to not interfere with request counting
        ss  << "DECIDE" << sep << i << sep << myPID << sep 
            << i << sep << blog->getBlock(i, false);

        sendMessage(clientPID, ss.str());

        ss.str(std::string());
        ss.clear();
    }
}