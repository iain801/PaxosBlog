#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <string>

enum OP{COMMENT, POST};
class Blockchain {

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

        inline Block* getPrev() const { return prev; }

        inline std::string getUser() const { return user; }

        inline std::string getTitle() const { return title; }

        inline std::string getContent() const { return content; }

        inline OP isPost() const { return type; }

        inline std::string getType() const { return type?"POST":"COMMENT"; }

        std::string getOperation() const;
        std::string str() const;

    };

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
    std::string str();
    
};

#endif //BLOCKCHAIN_H