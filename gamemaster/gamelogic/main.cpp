#include "GameLogic.h"
#include "Serialization.h"
#include "Transport.h"
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <fstream>

using json = nlohmann::json;

static std::atomic<bool> running{true};

void signalHandler(int)
{
    running = false;
}

std::string findSpellsJson(const std::string& argv0)
{
    std::ifstream test1("spells.json");
    if (test1.good()) return "spells.json";

    std::ifstream test2("gamemaster/spells.json");
    if (test2.good()) return "gamemaster/spells.json";

    std::string exePath = argv0;
    auto slash = exePath.find_last_of('/');
    if (slash != std::string::npos) {
        std::string dir = exePath.substr(0, slash + 1);

        std::ifstream test3(dir + "spells.json");
        if (test3.good()) return dir + "spells.json";

        std::ifstream test4(dir + "../../gamemaster/spells.json");
        if (test4.good()) return dir + "../../gamemaster/spells.json";
    }

    return "";
}

int main(int argc, char* argv[])
{
    signal(SIGINT, signalHandler);

    std::string spellsPath = findSpellsJson(argv[0]);
    if (spellsPath.empty()) {
        std::cerr << "Error: Could not find spells.json" << std::endl;
        return 1;
    }

    GameLogic game;
    if (!game.loadSpells(spellsPath)) {
        std::cerr << "Error: Failed to load spells from " << spellsPath << std::endl;
        return 1;
    }

    zmq::context_t context(1);

    zmq::socket_t publisher(context, zmq::socket_type::pub);
    publisher.bind("tcp://*:5555");
    std::cout << "PUB (GameStatus) bound to port 5555" << std::endl;

    zmq::socket_t receiver(context, zmq::socket_type::pull);
    receiver.bind("tcp://*:5556");
    std::cout << "PULL (spells/actions) bound to port 5556" << std::endl;

    std::cout << "GameMaster running. Waiting for players..." << std::endl;

    while (running) {
        json j;
        while (zmq_pull_raw(receiver, j)) {
            try {
                if (j.contains("spellName")) {
                    SpellMessage sm = j.get<SpellMessage>();
                    game.onSpellMessage(sm);
                } else if (j.contains("action")) {
                    PlayerAction pa = j.get<PlayerAction>();
                    game.onPlayerAction(pa);
                }
            } catch (const std::exception& e) {
                std::cerr << "Parse error: " << e.what() << std::endl;
            }
        }

        if (game.getPhase() != GamePhase::LOBBY &&
            game.getPhase() != GamePhase::GAME_OVER) {
            game.tick();
        }

        GameStatus gs1 = game.getStatusForPlayer("player1");
        GameStatus gs2 = game.getStatusForPlayer("player2");
        zmq_publish(publisher, "player1", gs1);
        zmq_publish(publisher, "player2", gs2);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "\nGameMaster stopped." << std::endl;
    return 0;
}
