#include "paxos.h"
#include "blockchain.h"

#include <string>
#include <thread>
#include <algorithm>
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
    std::string delimiter = ",";
    size_t pos = 0;

    while(true) {
        msgVector.clear();
        pos = 0;

        std::getline(std::cin, msg);
        // std::remove_if(msg.begin(), msg.end(), isspace);
        // msg.erase(remove(msg.begin(), msg.end(), ')'), msg.end());
        
        // Parse message into vector
        // Modified from https://stackoverflow.com/a/14266139
        if ((pos = msg.find('(')) != std::string::npos) {
            // push operation and erase up to opening paretheses
            msgVector.push_back(msg.substr(0, pos));
            msg.erase(0, pos + 1);
            // Remove end parentheses
            msg.pop_back();

            std::vector<std::string> args = split(msg, delimiter);
            msgVector.insert(msgVector.end(), args.begin(), args.end());
        }
        else msgVector.push_back(msg);

        // Pop operation type off front
        std::string opType = msgVector.front();
        msgVector.erase(msgVector.begin());

        if (opType == "crash") {
            std::cout << "Exiting..." << std::endl;
            break;
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
            // TODO: create a post <OP::POST, username, title, content>
            std::cout << "Feature not complete" << std::endl;
        }
        else if (opType == "comment") {
            // TODO: create a comment <OP::COMMENT, username, title, content>
            std::cout << "Feature not complete" << std::endl;
        }
        else if (opType == "test") {
            std::string str = msgVector.front();
            msgVector.erase(msgVector.begin());
            for (auto s : msgVector) {
                server->sendMessage(std::stoi(s), str);
            }
        }
        else if (opType == "getLinks") {
            std::cout << server->printConnections() << std::endl;
        }
        else {
            std::cout << "Invalid Command\n" << std::endl;
        }
    }
}

int main(int argc, char** argv)
{
    Blockchain blog;
    auto newBlock1 = blog.makeBlock(OP::POST, "ixw", "Hello World", "It's a nice day today");
    
    auto newBlock2 = blog.makeBlock(OP::COMMENT, "ixw", "Hello World", "No it's cloudy");
    
    auto newBlock3 = blog.makeBlock(OP::COMMENT, "abm", "Hello World", "It's raining today!");
    blog.addBlock(newBlock1);
    blog.addBlock(newBlock2);
    blog.addBlock(newBlock3);

    std::cout << blog.viewAll() << std::endl;
    std::cout << blog.viewComments("Hello World") << std::endl;

    PaxosHandler* server = new PaxosHandler(std::stoi(argv[1]));

    std::thread input(userInput, server);
    input.join();

    delete server;

    return 0;
}