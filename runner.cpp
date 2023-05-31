#include "paxos.h"
#include "blockchain.h"

#include <thread>
#include <iostream>

int main(int argc, char** argv)
{
    Blockchain bc;
    bc.addBlock(OP::POST, "ixw", "How to Train Your Dragon", "You don't");
    bc.addBlock(OP::COMMENT, "ixw", "How to Train Your Dragon", "Wait jk");

    std::cout << "done!" << std::endl;
    return 0;
}