#include "blockchain.h"

#include <sstream>
#include <iomanip>

#include <random>
#include <openssl/sha.h>

#include <stack>
#include <iostream>

Blockchain::Block::Block(Block* prev, OP type, std::string user, 
std::string title, std::string content) 
: prev(prev), type(type), user(user), title(title), content(content)
{
    if (prev) {
        prevHash = prev->hash;
    }
    else {
        prevHash = "00000000000000000000000000000000";
    }
    setHash();
}

std::string Blockchain::Block::getOperation() const
{
    std::ostringstream ss;
    ss << getType() << ", " << getUser() << ", " << getTitle() << ", " << getContent();
    return ss.str();
}

std::string Blockchain::Block::str() const
{
    std::ostringstream ss;
    ss << '(' << prevHash << ", " << getType() << ", " << getUser() 
        << ", " << getTitle() << ", " << getContent() << ", " << nonce << ')';
    return ss.str();
}

int Blockchain::Block::setHash() 
{   
    // setup random nonce generator
    std::random_device            rand_dev;
    std::mt19937                  gen(rand_dev());
    std::uniform_int_distribution dist(0, INT32_MAX);

    // reset nonce and rehash until leading digit is 1 or 0 (3-bits)
    do {
        nonce = dist(gen);
        hash = getHash();
    } while (hash[0] > '1');

    return nonce;
}

std::string Blockchain::Block::getHash()
{   
    // compile data 
    std::ostringstream ss;
    ss << prevHash << getOperation() << nonce;
    return sha256(ss.str());
}

/* https://stackoverflow.com/a/10632725 
    Updated to use non-deprecated function SHA256*/
std::string Blockchain::Block::sha256(const std::string str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*) str.c_str(), str.length(), hash);

    std::ostringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

Blockchain::Blockchain() : tail(nullptr) {}

Blockchain::~Blockchain()
{
    while (tail) {
        Block* prev = tail->getPrev();
        delete tail;
        tail = prev;
    }
}

int Blockchain::addBlock(OP type, std::string user, std::string title, std::string content)
{
    if (type == OP::POST || findPost(title)) {
        Block* newBlock = new Block(tail, type, user, title, content);
        tail = newBlock;
        return 0;
    }
    return 1;
}

Blockchain::Block* Blockchain::findPost(std::string title) 
{
    Block* curr = tail;
    while (curr) {
        if (curr->isPost() && curr->getTitle() == title) {
            return curr;
        }
        curr = curr->getPrev();
    }
    return nullptr;
}

std::string Blockchain::viewAll()
{
    std::stack<Block*> posts;
    Block* curr = tail;
    while (curr) {
        if (curr->isPost()) {
            posts.push(curr);
        }
        curr = curr->getPrev();
    }

    if (posts.empty()) return "BLOG EMPTY\n\n";

    std::ostringstream ss;
    while(!posts.empty()) {
        curr = posts.top();
        ss << '"' << curr->getTitle() << '"' << " by " << curr->getUser() << std::endl;
        posts.pop();
    }
    ss << std::endl;
    return ss.str();
}

std::string Blockchain::viewByUser(std::string user)
{
    std::stack<Block*> posts;
    Block* curr = tail;
    while (curr) {
        if (curr->getUser() == user) {
            posts.push(curr);
        }
        curr = curr->getPrev();
    }

    if (posts.empty()) return "NO POST\n\n";

    std::ostringstream ss;
    while(!posts.empty()) {
        curr = posts.top();
        ss << '"' << curr->getTitle() << '"' << std::endl;
        ss << curr->getContent() << std::endl << std::endl;
        posts.pop();
    }

    return ss.str();
}

std::string Blockchain::viewComments(std::string title) 
{
    std::stack<Block*> posts;
    Block* curr = tail;
    while (curr) {
        if (curr->getTitle() == title) {
            posts.push(curr);
            if(curr->isPost()) {
                break; //break once we reach the post
            }
        }
        curr = curr->getPrev();
    }

    // return message if the post not found
    if (posts.empty()) return "POST NOT FOUND\n\n";

    // print content of post
    std::ostringstream ss;
    ss << posts.top()->getContent() << std::endl << std::endl;
    posts.pop();

    // print comments in chronological order
    while(!posts.empty()) {
        curr = posts.top();
        ss << curr->getContent() << std::endl;
        ss << " - "<< curr->getUser() << std::endl << std::endl;
        posts.pop();
    }

    return ss.str();
}

std::string Blockchain::str()
{
    std::stack<Block*> posts;
    Block* curr = tail;
    while (curr) {
        posts.push(curr);
        curr = curr->getPrev();
    }

    std::ostringstream ss;
    ss << '[';

    while(!posts.empty()) {
        curr = posts.top();
        posts.pop();
        ss << curr->str();

        if(!posts.empty()) 
            ss << ", ";
    }

    ss << ']' << std::endl;

    return ss.str();
}
