#pragma once
#include "InputProvider.hpp"
#include "SpellMessage.hpp"
#include <zmq.hpp>
#include <string>
#include <memory>
#include <map>

class ZeroMQInputProvider : public InputProvider
{
private:
    zmq::context_t *context;
    zmq::socket_t socket;
    std::map<std::string, SpellMessage> inputBuffer;
    bool ownsContext;

public:
    // Constructor der accepterer en ekstern context (bruges sammen med GameMaster)
    explicit ZeroMQInputProvider(zmq::context_t &sharedContext);

    // Default constructor (ejer sin egen context)
    ZeroMQInputProvider();

    ~ZeroMQInputProvider();

    void initialize() override;
    void shutdown() override;
    SpellMessage getSpellInput(const std::string &playerName) override;
};