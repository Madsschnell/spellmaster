#pragma once
#include "InputProvider.hpp"
#include "SpellMessage.hpp"

// Mock implementation der bruger std::cin
// Erstattes senere med ZeroMQ eller MQTT implementation
class ConsoleInputProvider : public InputProvider
{
public:
    void initialize() override;
    void shutdown() override;
    SpellMessage getSpellInput(const std::string &playerName) override;
};
