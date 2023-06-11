#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <string>
#include <atomic>

enum OP{COMMENT, POST};
class Block {
        std::string genHash();
        int setHash();
        std::string sha256(const std::string str);

        Block* prev;
        std::string prevHash;
        std::string hash;
        std::atomic<int> nonce;

        enum OP type;
        std::string user;
        std::string title;
        std::string content;

    public:    

        static const std::string zeroHash;

        Block(std::string prevHash,
            OP type,
            std::string user,
            std::string title,
            std::string content);

        Block(std::string prevHash,
            std::string type,
            std::string user,
            std::string title,
            std::string content);

        inline Block* getPrev() const { return prev; }
        bool setPrev(Block* prev);

        inline std::string getUser() const { return user; }

        inline std::string getTitle() const { return title; }

        inline std::string getContent() const { return content; }

        inline std::string getHash() const { return hash; }

        inline int getNonce() const { return nonce; }

        inline std::string getPrevHash() const { return prevHash; }

        inline OP isPost() const { return type; }

        inline std::string getType() const { return type?"POST":"COMMENT"; }

        bool validNonce(int nonce);

        std::string getOperation() const;
        std::string str(bool paren=true) const;

    };

class Blockchain {

    Block* tail = nullptr;

    Block* findPost(std::string title);

    std::string fname;
    void save(Block* newBlock);

public:

    Blockchain(std::string filename);
    ~Blockchain();

    bool addBlock(Block* newBlock);

    Block* makeBlock(std::string type,
        std::string user,
        std::string title,
        std::string content);

    std::string viewAll();
    std::string viewByUser(std::string user);
    std::string viewComments(std::string title);
    std::string str();
    std::string getBlock(int depth, bool paren=true);
    
    int depth();
    
};

#endif //BLOCKCHAIN_H