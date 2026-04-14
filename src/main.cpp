#include "GameMaster.hpp"
#include "ZeroMQInputProvider.hpp"
#include <memory>

int main()
{
    // Opret GameMaster med ZeroMQ Input Provider
    // Venter nu på input fra anden RPi på port 5556 (i stedet for console)
    GameMaster gm(std::make_unique<ZeroMQInputProvider>());

    // Start spillet - broadcaster game state på port 5555
    gm.run();

    return 0;
}