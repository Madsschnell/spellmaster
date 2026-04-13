#include "GameMaster.hpp"
#include "ZeroMQInputProvider.hpp"
#include <memory>

int main()
{
    // Opret GameMaster (den laver sin egen ZeroMQ context)
    GameMaster gm;

    // Start spillet - GameMaster bruger ConsoleInputProvider som default
    gm.run();

    return 0;
}