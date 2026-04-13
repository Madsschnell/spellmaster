#pragma once
#include "InputProvider.hpp"
#include <zmq.hpp>
#include <string>
#include <memory>
#include <map>

class ZeroMQInputProvider : public InputProvider
{
private:
    zmq::context_t *context;
    zmq::socket_t socket;
    std::map<std::string, int> inputBuffer;
    bool ownsContext;

public:
    // Constructor der accepterer en ekstern context (bruges sammen med GameMaster)
    explicit ZeroMQInputProvider(zmq::context_t &sharedContext);

    // Default constructor (ejer sin egen context)
    ZeroMQInputProvider();

    ~ZeroMQInputProvider();

    void initialize() override;
    void shutdown() override;
    int getSpellInput(const std::string &playerName) override;
};