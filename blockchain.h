#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <string>

enum OP{COMMENT, POST};
class Block {
        std::string genHash();
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
        Block(std::string prevHash,
            OP type,
            std::string user,
            std::string title,
            std::string content);

        inline Block* getPrev() const { return prev; }
        int setPrev(Block* prev);

        inline std::string getUser() const { return user; }

        inline std::string getTitle() const { return title; }

        inline std::string getContent() const { return content; }

        inline std::string getHash() const { return hash; }

        inline std::string getPrevHash() const { return prevHash; }

        inline OP isPost() const { return type; }

        inline std::string getType() const { return type?"POST":"COMMENT"; }

        bool validNonce();

        std::string getOperation() const;
        std::string str() const;

    };

class Blockchain {

    Block* tail = nullptr;

    Block* findPost(std::string title);


public:

    Blockchain();
    ~Blockchain();

    int addBlock(Block* newBlock);

    Block* makeBlock(OP type,
        std::string user,
        std::string title,
        std::string content);

    std::string viewAll();
    std::string viewByUser(std::string user);
    std::string viewComments(std::string title);
    std::string str();
    
    int depth();
    
};

#endif //BLOCKCHAIN_H