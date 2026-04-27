#include "ZeroMQInputProvider.hpp"
#include <iostream>

ZeroMQInputProvider::ZeroMQInputProvider(zmq::context_t &sharedContext)
    : context(&sharedContext), socket(*context, zmq::socket_type::pull), ownsContext(false) {}

ZeroMQInputProvider::ZeroMQInputProvider()
    : context(new zmq::context_t(1)), socket(*context, zmq::socket_type::pull), ownsContext(true) {}

ZeroMQInputProvider::~ZeroMQInputProvider()
{
    shutdown();
}

void ZeroMQInputProvider::initialize()
{
    socket.bind("tcp://*:5556");
    int timeout = 100; // 100ms non-blocking timeout
    socket.set(zmq::sockopt::rcvtimeo, timeout);
}

void ZeroMQInputProvider::shutdown()
{
    socket.close();
    if (ownsContext && context)
    {
        context->close();
        delete context;
        context = nullptr;
    }
}

SpellMessage ZeroMQInputProvider::getSpellInput(const std::string &playerName)
{
    // Check if we already have input from this player from a previous call
    if (inputBuffer.count(playerName))
    {
        SpellMessage msg = inputBuffer[playerName];
        inputBuffer.erase(playerName);
        return msg;
    }

    while (true)
    {
        zmq::message_t request;
        auto result = socket.recv(request, zmq::recv_flags::none);

        if (!result)
        {
            return SpellMessage{playerName, ""}; // Timeout - return Miss
        }

        // Receive SpellMessage struct as binary data
        if (request.size() == sizeof(SpellMessage))
        {
            SpellMessage msg;
            memcpy(&msg, request.data(), sizeof(SpellMessage));

            // Check if this is the player we're waiting for
            if (msg.playerId == playerName)
            {
                return msg;
            }
            else if (!msg.playerId.empty())
            {
                // Store for later if it's from another player
                inputBuffer[msg.playerId] = msg;
            }
        }
    }
}