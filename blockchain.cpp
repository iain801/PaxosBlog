#include "blockchain.h"

#include <sstream>
#include <fstream>
#include <iomanip>

#include <random>
#include <stack>

#include <openssl/sha.h>

const std::string Block::zeroHash = "0000000000000000000000000000000000000000000000000000000000000000";

Block::Block(std::string prevHash, OP type, std::string user, 
std::string title, std::string content) 
: prevHash(prevHash), nonce(0), type(type), user(user), title(title), content(content)
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

std::string Block::str(bool paren) const
{
    std::ostringstream ss;
    if (paren) ss << '(';
    ss << prevHash << ", " << getType() << ", " << getUser() 
        << ", " << getTitle() << ", " << getContent() << ", " << nonce;
    if (paren) ss << ')';
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
    } while (hash.front() > '1');

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
    auto newHash = genHash();
    bool out = (hash.front() <= '1');
    if (out) hash = newHash;
    else nonce = temp;
    return out;
}

bool Block::setPrev(Block* prev)
{  
    if(prev) {
        if(prev->hash == this->prevHash) {
            this->prev = prev;
            return true;
        }
    }
    else {
        if(this->prevHash == zeroHash) {
            this->prev = prev;
            return true;
        }
    }
    return false;
}

Blockchain::Blockchain(std::string filename) : tail(nullptr), fname(filename) 
{
    std::string transaction, token;
    std::ifstream f(filename);
    char sep = '~';
    while (std::getline(f, transaction, '\n')) {
        std::vector<std::string> transVector;
        std::istringstream tokenStream(transaction);
        while (std::getline(tokenStream, token, sep)) {
            transVector.push_back(token);
        }
        Block* newBlock = makeBlock(transVector[2], transVector[3], transVector[4], transVector[5]);
        if(newBlock->setPrev(tail) && newBlock->validNonce(std::stoi(transVector[6]))) {
            if(newBlock->getHash() == transVector[0]) {
                tail = newBlock;
            }
        }
    }
    f.close();
}

Blockchain::~Blockchain()
{
    std::unique_lock<std::mutex> lock(chainMutex);
    while (tail) {
        Block* prev = tail->getPrev();
        delete tail;
        tail = prev;
    }
}

Block* Blockchain::makeBlock(std::string type, std::string user, std::string title, std::string content)
{
    std::unique_lock<std::mutex> lock(chainMutex);
    if (type == "POST" || findPost(title)) {
        std::string hash = Block::zeroHash;
        
        if(tail)
            hash = tail->getHash();
            
        Block* newBlock = new Block(hash, type, user, title, content);
        return newBlock;
    }
    return nullptr;
}

bool Blockchain::addBlock(Block* newBlock) 
{   
    std::unique_lock<std::mutex> lock(chainMutex);
    if(!newBlock)
        return false;
    if(tail) {
        if(newBlock->getPrevHash() == tail->getHash()) {
            if(newBlock->setPrev(tail)) {
                tail = newBlock;
                save(newBlock);
                return true;
            }
        }
    }
    else {
        if(newBlock->getPrevHash() == Block::zeroHash) {
            if(newBlock->setPrev(tail)) {
                tail = newBlock;
                save(newBlock);
                return true;
            }
        }
    }
    return false;
}

void Blockchain::save(Block* newBlock)
{
    std::unique_lock<std::mutex> lock(saveMutex);
    char sep = '~';
    std::ostringstream ss;
    ss << newBlock->getHash() << sep << newBlock->getPrevHash() << sep << newBlock->getType()
        << sep << newBlock->getUser() << sep << newBlock->getTitle() << sep << newBlock->getContent()
        << sep << newBlock->getNonce();
    std::ofstream f(fname, std::ios_base::app);
    f << ss.str() << '\n';
    f.close();
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

//return string of block at given depth
std::string Blockchain::getBlock(int depth, bool paren)
{
    std::unique_lock<std::mutex> lock(chainMutex);
    Block* target = tail;
    int diff = this->depth() - depth;

    if (depth < 1 || diff < 0) {
        return "Invalid depth";
    }

    for (int i=0; i<diff; ++i) {
        target = target->getPrev();
    }

    return target->str(paren);
}
