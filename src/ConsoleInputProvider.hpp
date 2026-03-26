#pragma once
#include "InputProvider.hpp"

// Mock implementation der bruger std::cin
// Erstattes senere med ZeroMQ eller MQTT implementation
class ConsoleInputProvider : public InputProvider
{
public:
    void initialize() override;
    void shutdown() override;
    int getSpellInput(const std::string& playerName) override;
};
