#include "paxos.h"
#include "blockchain.h"

#include <thread>
#include <iostream>

int main(int argc, char** argv)
{
    Blockchain bc;
    bc.addBlock(OP::POST, "ixw", "How to Train Your Dragon", "You don't");
    bc.addBlock(OP::COMMENT, "ixw", "How to Train Your Dragon", "Wait jk");
    bc.addBlock(OP::POST, "abm", "Top 10 Anime Betrayals", "I'm not quite sure actually");
    bc.addBlock(OP::COMMENT, "abm", "How to Train Your Dragon", "Are you sure about that?");
    bc.addBlock(OP::COMMENT, "ixw", "Top 10 Anime Betrayals", "Damn that's disappointing");

    std::cout << bc.viewAll();
    std::cout << bc.viewByUser("ixw");
    std::cout << bc.viewComments("How to Train Your Dragon");

    std::cout << "done!" << std::endl;
    return 0;
}