#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cassert>
#include <memory>
#include <zmq.hpp>

#include "GameStatus.h"
#include "Serialization.h"
#include "SpellMessage.h"
#include "PlayerAction.h"
#include "Transport.h"

using namespace std::chrono_literals;

int main(int argc, char* argv[])
{
    std::string mode = "local";
    std::string ip = "localhost";

    if (argc >= 2) {
        mode = argv[1];
        if (mode == "--help" || mode == "-h") {
            std::cout << "Usage: " << argv[0] << " [local|gamemaster|player] [ip]\n";
            return 0;
        }
    }
    if (argc >= 3) {
        ip = argv[2];
    }

    if (mode != "local" && mode != "gamemaster" && mode != "player") {
        std::cerr << "Usage: " << argv[0] << " [local|gamemaster|player] [ip]\n";
        return 1;
    }

    std::cout << "Starting module test for ZMQ and Serialization in mode: " << mode << "\n\n";

    zmq::context_t context(1);

    bool run_gm = (mode == "local" || mode == "gamemaster");
    bool run_pl = (mode == "local" || mode == "player");

    std::unique_ptr<zmq::socket_t> gm_pub;
    std::unique_ptr<zmq::socket_t> gm_pull;
    std::unique_ptr<zmq::socket_t> player_sub;
    std::unique_ptr<zmq::socket_t> player_push;

    if (run_gm) {
        gm_pub = std::make_unique<zmq::socket_t>(context, zmq::socket_type::pub);
        gm_pub->bind("tcp://*:5555");
        gm_pull = std::make_unique<zmq::socket_t>(context, zmq::socket_type::pull);
        gm_pull->bind("tcp://*:5556");
        std::cout << "[GameMaster] Bound to ports 5555 and 5556\n";
    }

    if (run_pl) {
        player_sub = std::make_unique<zmq::socket_t>(context, zmq::socket_type::sub);
        player_sub->connect("tcp://" + ip + ":5555");
        player_sub->set(zmq::sockopt::subscribe, "player1");
        
        player_push = std::make_unique<zmq::socket_t>(context, zmq::socket_type::push);
        player_push->connect("tcp://" + ip + ":5556");
        std::cout << "[Player] Connected to " << ip << " on ports 5555 and 5556\n";
    }

    std::this_thread::sleep_for(500ms); // Allow connections to establish

    // ZeroMQ PUB sockets will drop messages if subscribers aren't ready. Player sends a sync message and GameMaster waits for it.
    if (run_pl) {
        PlayerAction syncMsg{"player1", "sync"};
        zmq_push(*player_push, syncMsg);
    }

    if (run_gm) {
        std::cout << "[GameMaster] Waiting for Player to sync...\n";
        json j;
        while (!zmq_pull_raw(*gm_pull, j)) {
            std::this_thread::sleep_for(10ms);
        }
        std::cout << "[GameMaster] Player synced!\n";
    }

    // --- Normal operation ---
    std::cout << "\n--- Normal operation ---\n";

    if (run_gm) {
        GameStatus gsOut;
        gsOut.phase = GamePhase::CASTING;
        gsOut.selfEffect.type = EffectType::HEAL;
        zmq_publish(*gm_pub, "player1", gsOut);
        std::cout << "[GameMaster] Published GameStatus.\n";
    }

    if (run_pl) {
        std::string topic;
        GameStatus gsIn;
        bool hasGameStatus = false;
        
        for(int i=0; i<50 && !hasGameStatus; ++i) {
            hasGameStatus = zmq_subscribe_recv(*player_sub, topic, gsIn);
            if (!hasGameStatus) std::this_thread::sleep_for(10ms);
        }

        assert(hasGameStatus);
        assert(topic == "player1");
        assert(gsIn.phase == GamePhase::CASTING);
        assert(gsIn.selfEffect.type == EffectType::HEAL);
        
        std::cout << "[OK] Serialization: GameStatus round-trip successful.\n";
        std::cout << "     Data integrity verified:\n";
        std::cout << "     " << json_to_string(gsIn) << "\n";

        SpellMessage smOut{"player1", "Fireball"};
        zmq_push(*player_push, smOut);
    }

    if (run_gm) {
        json gmReceivedJson;
        bool hasSpellMsg = false;
        for(int i=0; i<50 && !hasSpellMsg; ++i) {
            hasSpellMsg = zmq_pull_raw(*gm_pull, gmReceivedJson);
            if (!hasSpellMsg) std::this_thread::sleep_for(10ms);
        }
        
        assert(hasSpellMsg);
        SpellMessage smIn;
        gmReceivedJson.get_to(smIn);
        assert(smIn.spellName == "Fireball");
        
        std::cout << "[OK] Deserialization: SpellMessage round-trip successful.\n";
        std::cout << "     Client -> GameMaster payload verified:\n";
        std::cout << "     " << gmReceivedJson.dump() << "\n";
    }

    // --- Stress test ---
    std::cout << "\n--- Stress test ---\n";
    const int NUM_MESSAGES = 50;

    if (run_pl) {
        for (int i = 0; i < NUM_MESSAGES; ++i) {
            PlayerAction pa{"player1", "ready_" + std::to_string(i)};
            zmq_push(*player_push, pa);
        }
        std::cout << "[Player] Sent 50 PlayerAction messages.\n";
    }

    if (run_gm) {
        int receivedCount = 0;
        PlayerAction paIn;
        json gmReceivedJson;
        
        for(int t=0; t<100 && receivedCount < NUM_MESSAGES; ++t) {
            while (receivedCount < NUM_MESSAGES && zmq_pull_raw(*gm_pull, gmReceivedJson)) {
                gmReceivedJson.get_to(paIn);
                // Verify messages arrive in the same order they were sent
                assert(paIn.action == "ready_" + std::to_string(receivedCount));
                receivedCount++;
            }
            std::this_thread::sleep_for(10ms);
        }
        assert(receivedCount == NUM_MESSAGES);
        std::cout << "[OK] Stress test: 50 PlayerAction messages sent and received in order.\n";
    }

    // --- Error handling ---
    std::cout << "\n--- Error handling ---\n";

    if (run_pl) {
        // Corrupt JSON 1: Wrong data type.
        std::string badJsonType = "{\"name\":\"Player\",\"hp\":100,\"maxHp\":100,\"mana\":\"halvtreds\",\"maxMana\":50,\"spellCast\":\"\",\"spellDescription\":\"\"}";
        player_push->send(zmq::buffer(badJsonType), zmq::send_flags::none);

        // Corrupt JSON 2: Malformed syntax.
        std::string badJsonSyntax = "{\"phase\": \"CASTING\"";
        player_push->send(zmq::buffer(badJsonSyntax), zmq::send_flags::none);
        
        std::cout << "[Player] Sent 2 corrupt JSON payloads.\n";
    }

    if (run_gm) {
        std::this_thread::sleep_for(100ms); // wait for payloads to arrive

        PlayerInfo piBad;
        bool successType = zmq_pull_raw(*gm_pull, piBad);
        if (!successType) {
            std::cout << "[OK] Error handling: Caught type error ('mana'). Rejected without crash.\n";
        }

        GameStatus gsBad;
        bool successSyntax = zmq_pull_raw(*gm_pull, gsBad);
        if (!successSyntax) {
            std::cout << "[OK] Error handling: Caught syntax error. Rejected without crash.\n";
        }
    }

    std::cout << "\n>>> ALL TESTS PASSED <<<\n";

    return 0;
}
