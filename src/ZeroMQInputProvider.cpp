#include "ZeroMQInputProvider.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

ZeroMQInputProvider::ZeroMQInputProvider()
    : context(1), socket(context, zmq::socket_type::pull) {}

ZeroMQInputProvider::~ZeroMQInputProvider()
{
    shutdown();
}

void ZeroMQInputProvider::initialize()
{
    // Binder til port 5556 på alle netværksgrænseflader (ifølge README)
    socket.bind("tcp://*:5556");
    std::cout << "[ZeroMQ] Venter paa netvaerksinput fra spillere paa port 5556..." << std::endl;
}

void ZeroMQInputProvider::shutdown()
{
    socket.close();
    context.close();
}

int ZeroMQInputProvider::getSpellInput(const std::string &playerName)
{
    std::cout << "Venter paa at " << playerName << " kaster en spell (ZeroMQ)..." << std::endl;

    while (true)
    {
        zmq::message_t request;

        // Blokerer og venter indtil vi modtager en besked på port 5556
        (void)socket.recv(request, zmq::recv_flags::none);

        std::string msg_str(static_cast<char *>(request.data()), request.size());

        try
        {
            auto j = nlohmann::json::parse(msg_str);

            // Tjek om beskeden er fra den spiller, vi venter på lige nu
            if (j.contains("playerName") && j["playerName"] == playerName)
            {
                return j.value("spellId", 0);
            }
        }
        catch (const nlohmann::json::exception &e)
        {
            std::cerr << "[ZeroMQ] Ugyldig JSON modtaget: " << e.what() << std::endl;
        }
    }
}