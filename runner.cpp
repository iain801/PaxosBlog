#include "paxos.h"
#include "blockchain.h"

#include <string>
#include <thread>
#include <iostream>

// https://stackoverflow.com/a/46931770
std::vector<std::string> split(std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

void userInput(PaxosHandler* server)
{   
    std::vector<std::string> msgVector;
    std::string msg;
    std::string delimiter = ", ";
    size_t pos = 0;

    while(true) {
        msgVector.clear();
        pos = 0;

        std::getline(std::cin, msg);
        
        // Parse message into vector
        // Modified from https://stackoverflow.com/a/14266139
        if ((pos = msg.find('(')) != std::string::npos) {
            msgVector.push_back(msg.substr(0, pos));
            msg.erase(0, pos + 1);
            msg.pop_back();

            std::vector<std::string> args = split(msg, delimiter);
            msgVector.insert(msgVector.end(), args.begin(), args.end());
        }
        else msgVector.push_back(msg);

        // Pop operation type off front
        std::string opType = msgVector.front();
        msgVector.erase(msgVector.begin());

        if (opType == "crash") {
            break;
        } 
        else if (opType == "failLink") {
            for (auto s : msgVector) {
                int target = stoi(s);
                server->disconnect(target);
            }
        } 
        else if (opType == "fixLink") {
            for (auto s : msgVector) {
                int target = stoi(s);
                server->interconnect(target);
            }
        } 
        else if (opType == "getConnections") {
            auto inSocks = server->getInSocks();
            auto outSocks = server->getOutSocks();
            std::cout << "In Socks: \n";
            for (auto s : inSocks) {
                std::cout << "PID " << s.first << ": " << s.second << "\n";
            }
            std::cout << "Out Socks: \n";
            for (auto s : outSocks) {
                std::cout << "PID " << s.first << ": " << s.second << "\n";
            }
            std::cout << std::endl;
        }
        else {
            std::cout << "Invalid Command" << std::endl;
        }
    }
}

int main(int argc, char** argv)
{
    Blockchain bc;
    bc.addBlock(OP::POST, "ixw", "How to Train Your Dragon", "You don't");
    bc.addBlock(OP::COMMENT, "ixw", "How to Train Your Dragon", "Wait jk");
    bc.addBlock(OP::POST, "abm", "Top 10 Anime Betrayals", "I'm not quite sure actually");
    bc.addBlock(OP::COMMENT, "abm", "How to Train Your Dragon", "Are you sure about that?");
    bc.addBlock(OP::COMMENT, "ixw", "Top 10 Anime Betrayals", "Damn that's disappointing");

    // std::cout << bc.viewAll();
    // std::cout << bc.viewByUser("ixw");
    // std::cout << bc.viewComments("How to Train Your Dragon");
    // std::cout << bc.str();

    PaxosHandler* server = new PaxosHandler(std::stoi(argv[1]));

    std::thread input(userInput, server);
    input.join();

    delete server;

    return 0;
}