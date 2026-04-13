#pragma once
#include "InputProvider.hpp"
#include <zmq.hpp>
#include <string>
#include <memory>

class ZeroMQInputProvider : public InputProvider
{
private:
    zmq::context_t context;
    zmq::socket_t socket;

public:
    // Sætter vores ZeroMQ socket op som en PULL-socket (modtager data)
    ZeroMQInputProvider();
    ~ZeroMQInputProvider();

    void initialize() override;
    void shutdown() override;
    int getSpellInput(const std::string &playerName) override;
};