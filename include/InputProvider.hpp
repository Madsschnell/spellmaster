#pragma once
#include <string>

// Interface for input providers
// Gør det nemt at skifte mellem mock (std::cin) og rigtig netværkskommunikation (ZeroMQ/MQTT)
class InputProvider
{
public:
    virtual ~InputProvider() = default;

    // Henter spell input fra en spiller (0 = Miss, 1 = Fireball, 2 = Lightning, etc.)
    virtual int getSpellInput(const std::string& playerName) = 0;

    // Initialiserer input provider (åbner netværksforbindelser hvis nødvendigt)
    virtual void initialize() = 0;

    // Rydder op (lukker netværksforbindelser hvis nødvendigt)
    virtual void shutdown() = 0;
};
