#include "paxos.h"
#include "blockchain.h"

#include <string>
#include <thread>
#include <algorithm>
#include <iostream>
#include <atomic>

// https://stackoverflow.com/a/46931770
std::deque<std::string> split(std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::deque<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

void handleInput(PaxosHandler* server, std::deque<std::string> msgVector, std::atomic<bool>* flag) 
{
    // Pop operation type off front
    std::string opType = msgVector.front();
    msgVector.erase(msgVector.begin());

    if (opType == "crash") {
        std::cout << "Exiting..." << std::endl;
        *flag = false;
        return;
    } 
    else if (opType == "failLink") {
        for (auto s : msgVector) {
            int target = std::stoi(s);
            server->disconnect(target);
        }
    } 
    else if (opType == "fixLink") {
        for (auto s : msgVector) {
            int target = std::stoi(s);
            server->interconnect(target);
        }
    } 
    else if (opType == "connect") {
        server->connectAll(msgVector);
    } 
    else if (opType == "blockchain") {
        std::cout << server->printBlockchain() << std::endl;
    }
    else if (opType == "blog") {
        std::cout << server->printBlog() << std::endl;
    }
    else if (opType == "view") {
        std::cout << server->printByUser(msgVector.front()) << std::endl;
    }
    else if (opType == "read") {
        std::cout << server->printComments(msgVector.front()) << std::endl;
    }
    else if (opType == "queue") {
        std::cout << server->printQueue() << std::endl;
    }
    else if (opType == "post") {
        std::ostringstream ss;
        ss << "POST, " << msgVector[0] << ", " << msgVector[1] << ", " << msgVector[2];
        server->startPaxos(ss.str());
    }
    else if (opType == "comment") {
        std::ostringstream ss;
        ss << "COMMENT, " << msgVector[0] << ", " << msgVector[1] << ", " << msgVector[2];
        server->startPaxos(ss.str());
    }
    else if (opType == "test") {
        std::string str = msgVector.front();
        msgVector.erase(msgVector.begin());
        for (auto s : msgVector) {
            server->sendMessage(std::stoi(s), str);
        }
    }
    else if (opType == "leader") {
        int leader = server->getLeader();
        std::cout << "Leader: ";
        if (leader == -1)
            std::cout << "Not Elected\n" << std::endl;
        else   
            std::cout << leader << std::endl << std::endl;
    }
    else if (opType == "links") {
        std::cout << server->printConnections() << std::endl;
    }
    else {
        std::cout << "Invalid Command\n" << std::endl;
    }
}

void userInput(PaxosHandler* server, std::atomic<bool>* flag)
{   
    std::deque<std::string> msgVector;
    std::string msg;
    std::string delimiter = ",";
    size_t pos = 0;

    while(*flag) {
        msgVector.clear();
        pos = 0;

        std::getline(std::cin, msg);
        // std::remove_if(msg.begin(), msg.end(), isspace);
        // msg.erase(remove(msg.begin(), msg.end(), ')'), msg.end());
        
        // Parse message into deque
        // Modified from https://stackoverflow.com/a/14266139
        if ((pos = msg.find('(')) != std::string::npos) {
            // push operation and erase up to opening paretheses
            msgVector.push_back(msg.substr(0, pos));
            msg.erase(0, pos + 1);
            // Remove end parentheses
            msg.pop_back();

            std::deque<std::string> args = split(msg, delimiter);
            msgVector.insert(msgVector.end(), args.begin(), args.end());
        }
        else msgVector.push_back(msg);

        std::thread inputThread(&handleInput, server, msgVector, flag);
        inputThread.detach();
    }
}

int main(int argc, char** argv)
{
    PaxosHandler* server = new PaxosHandler(std::stoi(argv[1]));
    std::atomic<bool> flag = true;

    std::thread input(userInput, server, &flag);
    input.detach();

    while(flag) {
        std::this_thread::yield();
    }

    delete server;

    return 0;
}