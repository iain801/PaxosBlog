#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <string>
#include <vector>

enum OP{COMMENT, POST};

class Block {
    std::string getHash();
    int setHash();
    std::string sha256(const std::string str);

    Block* prev;
    std::string prevHash;
    std::string hash;
    int nonce;
    enum OP type;
    std::string user;
    std::string title;
    std::string content;

public:    
    Block(Block* prev,
        OP type,
        std::string user,
        std::string title,
        std::string content);

    Block* getPrev();
    std::string getUser();
    std::string getTitle();
    std::string getContent();
    OP isPost();

};

class Blockchain {

    Block* tail = nullptr;

    Block* findPost(std::string title);


public:

    Blockchain();
    ~Blockchain();

    int addBlock(OP type,
        std::string user,
        std::string title,
        std::string content);

    std::string viewAll();
    std::string viewByUser(std::string user);
    std::string viewComments(std::string title);
    
};

#endif //BLOCKCHAIN_H