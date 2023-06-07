#include "blockchain.h"

#include <sstream>
#include <iomanip>

#include <random>
#include <openssl/sha.h>

#include <stack>
#include <iostream>

Block::Block(std::string prevHash, OP type, std::string user, 
std::string title, std::string content) 
: prevHash(prevHash), type(type), user(user), title(title), content(content)
{
    prev = nullptr;
    setHash();
}

Block::Block(std::string prevHash, std::string type, std::string user, 
std::string title, std::string content) 
: prevHash(prevHash), user(user), title(title), content(content)
{
    this->type = (type=="POST") ? OP::POST : OP::COMMENT;
    prev = nullptr;
    setHash();
}

std::string Block::getOperation() const
{
    std::ostringstream ss;
    ss << getType() << ", " << getUser() << ", " << getTitle() << ", " << getContent();
    return ss.str();
}

std::string Block::str() const
{
    std::ostringstream ss;
    ss << '(' << prevHash << ", " << getType() << ", " << getUser() 
        << ", " << getTitle() << ", " << getContent() << ", " << nonce << ')';
    return ss.str();
}

int Block::setHash() 
{   
    // setup random nonce generator
    std::random_device            rand_dev;
    std::mt19937                  gen(rand_dev());
    std::uniform_int_distribution dist(0, INT32_MAX);

    // reset nonce and rehash until leading digit is 1 or 0 (3-bits)
    do {
        nonce = dist(gen);
        hash = genHash();
    } while (hash[0] > '1');

    return nonce;
}

std::string Block::genHash()
{   
    // compile data 
    std::ostringstream ss;
    ss << prevHash << getOperation() << nonce;
    return sha256(ss.str());
}

/* https://stackoverflow.com/a/10632725 
    Updated to use non-deprecated function SHA256*/
std::string Block::sha256(const std::string str)
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

// Sets nonce to input if valid, returns true if successful
bool Block::validNonce(int testNonce)
{
    int temp = nonce;
    nonce = testNonce;
    bool out = (hash[0] <= '1') && (getHash() == hash);
    if (!out) nonce = temp;
    return out;
}

int Block::setPrev(Block* prev)
{  
    if(prev) {
        if(prev->hash == this->prevHash) {
            this->prev = prev;
            return 1;
        }
    }
    else {
        if(this->prevHash == "00000000000000000000000000000000") {
            this->prev = prev;
            return 1;
        }
    }
    return 0;
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

Block* Blockchain::makeBlock(std::string type, std::string user, std::string title, std::string content)
{
    if (type == "POST" || findPost(title)) {
        std::string hash = "00000000000000000000000000000000";
        
        if(tail)
            hash = tail->getHash();
            
        Block* newBlock = new Block(hash, type, user, title, content);
        return newBlock;
    }
    return nullptr;
}

int Blockchain::addBlock(Block* newBlock) 
{   
    if(!newBlock)
        return 0;
    if(tail) {
        if(newBlock->getPrevHash() == tail->getHash()) {
            if(newBlock->setPrev(tail)) {
                tail = newBlock;
                return 1;
            }
        }
    }
    else {
        if(newBlock->getPrevHash() == "00000000000000000000000000000000") {
            if(newBlock->setPrev(tail)) {
                tail = newBlock;
                return 1;
            }
        }
    }
    return 0;
}

Block* Blockchain::findPost(std::string title) 
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

int Blockchain::depth()
{
    int c = 0;
    Block* curr = tail;
    while (curr) {
        ++c;
        curr = curr->getPrev();
    }
    return c;
}
