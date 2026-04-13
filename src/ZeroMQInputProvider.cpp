#include "ZeroMQInputProvider.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

#include "ZeroMQInputProvider.hpp"
#include <nlohmann/json.hpp>
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
    // Binder til port 5556 på alle netværksgrænseflader (ifølge README)
    socket.bind("tcp://*:5556");

    int timeout = 5000;
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

int ZeroMQInputProvider::getSpellInput(const std::string &playerName)
{
    // 1. Tjek om vi allerede har modtaget input fra denne spiller i en tidligere kørsel
    if (inputBuffer.count(playerName))
    {
        int spellId = inputBuffer[playerName];
        inputBuffer.erase(playerName); // Ryd op i bufferen
        std::cout << "[ZeroMQ] Brugte gemt input fra " << playerName << std::endl;
        return spellId;
    }

    std::cout << "Venter paa at " << playerName << " kaster en spell (ZeroMQ)..." << std::endl;

    while (true)
    {
        zmq::message_t request;

        // Forsøg at modtage besked. Hvis du har sat timeout i initialize(),
        // returnerer den false hvis tiden løber ud.
        auto result = socket.recv(request, zmq::recv_flags::none);

        if (!result)
        {
            std::cout << "[ZeroMQ] Timeout: " << playerName << " svarede ikke. Returnerer Miss." << std::endl;
            return 0; // Spell ID 0 = Miss
        }

        std::string msg_str(static_cast<char *>(request.data()), request.size());

        try
        {
            auto j = nlohmann::json::parse(msg_str);
            std::string senderName = j.value("playerName", "");
            int spellId = j.value("spellId", 0);

            // 2. Er det den spiller vi venter på?
            if (senderName == playerName)
            {
                return spellId;
            }
            else if (!senderName.empty())
            {
                // 3. Det var en anden spiller (f.eks. Spiller 2 sendte mens vi ventede på 1).
                // Gem det i bufferen til senere!
                inputBuffer[senderName] = spellId;
                std::cout << "[ZeroMQ] Modtog input fra " << senderName << " (gemt i buffer)" << std::endl;
            }
        }
        catch (const nlohmann::json::exception &e)
        {
            std::cerr << "[ZeroMQ] Ugyldig JSON modtaget: " << e.what() << std::endl;
        }
    }
}