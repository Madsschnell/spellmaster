#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cassert>
#include <zmq.hpp>

#include "GameStatus.h"
#include "Serialization.h"
#include "SpellMessage.h"
#include "PlayerAction.h"
#include "Transport.h"

using namespace std::chrono_literals;

int main()
{
    std::cout << "Starting module test for ZMQ and Serialization...\n\n";

    zmq::context_t context(1);

    // GameMaster side (publishes GameStatus, receives actions/spells)
    zmq::socket_t gm_pub(context, zmq::socket_type::pub);
    gm_pub.bind("tcp://*:5555");

    zmq::socket_t gm_pull(context, zmq::socket_type::pull);
    gm_pull.bind("tcp://*:5556");

    // Player side (receives GameStatus, sends actions/spells)
    // NOTE: Replace localhost with actual IP when testing across two RPi nodes
    zmq::socket_t player_sub(context, zmq::socket_type::sub);
    player_sub.connect("tcp://localhost:5555");
    player_sub.set(zmq::sockopt::subscribe, "player1");

    zmq::socket_t player_push(context, zmq::socket_type::push);
    player_push.connect("tcp://localhost:5556");

    std::this_thread::sleep_for(100ms);

    // --- Normal operation ---
    std::cout << "--- Normal operation ---\n";

    // PUB/SUB (GameStatus)
    GameStatus gsOut;
    gsOut.phase = GamePhase::CASTING;
    gsOut.selfEffect.type = EffectType::HEAL;
    zmq_publish(gm_pub, "player1", gsOut);

    std::this_thread::sleep_for(10ms);

    std::string topic;
    GameStatus gsIn;
    bool hasGameStatus = zmq_subscribe_recv(player_sub, topic, gsIn);

    assert(hasGameStatus);
    assert(topic == "player1");
    assert(gsIn.phase == GamePhase::CASTING);
    assert(gsIn.selfEffect.type == EffectType::HEAL);
    std::cout << "[OK] Serialization: GameStatus round-trip successful.\n";

    // PUSH/PULL (SpellMessage)
    SpellMessage smOut{"player1", "Fireball"};
    zmq_push(player_push, smOut);

    std::this_thread::sleep_for(10ms);

    json gmReceivedJson;
    bool hasSpellMsg = zmq_pull_raw(gm_pull, gmReceivedJson);
    assert(hasSpellMsg);

    SpellMessage smIn;
    gmReceivedJson.get_to(smIn);
    assert(smIn.spellName == "Fireball");
    std::cout << "[OK] Deserialization: SpellMessage round-trip successful.\n";

    // --- Stress test ---
    std::cout << "\n--- Stress test ---\n";

    const int NUM_MESSAGES = 50;

    // Send 50 messages concurrently
    for (int i = 0; i < NUM_MESSAGES; ++i)
    {
        PlayerAction pa{"player1", "ready_" + std::to_string(i)};
        zmq_push(player_push, pa);
    }

    std::this_thread::sleep_for(50ms);

    int receivedCount = 0;
    PlayerAction paIn;

    // Read from PULL socket until buffer is empty
    while (zmq_pull_raw(gm_pull, gmReceivedJson))
    {
        gmReceivedJson.get_to(paIn);
        // Verify messages arrive in the same order they were sent
        assert(paIn.action == "ready_" + std::to_string(receivedCount));
        receivedCount++;
    }

    assert(receivedCount == NUM_MESSAGES);
    std::cout << "[OK] Stress test: 50 PlayerAction messages sent and received in order.\n";

    // --- Error handling (robustness against corrupt data) ---
    std::cout << "\n--- Error handling ---\n";

    // Corrupt JSON 1: Wrong data type. 'mana' expects int, but we send a string.
    std::string badJsonType = "{\"name\":\"Player\",\"hp\":100,\"maxHp\":100,\"mana\":\"halvtreds\",\"maxMana\":50,\"spellCast\":\"\",\"spellDescription\":\"\"}";
    player_push.send(zmq::buffer(badJsonType), zmq::send_flags::none);

    // Corrupt JSON 2: Malformed syntax. Missing closing brace.
    std::string badJsonSyntax = "{\"phase\": \"CASTING\"";
    player_push.send(zmq::buffer(badJsonSyntax), zmq::send_flags::none);

    std::this_thread::sleep_for(10ms);

    // Read first corrupt message
    PlayerInfo piBad;
    bool successType = zmq_pull_raw(gm_pull, piBad);
    if (!successType)
    {
        std::cout << "[OK] Error handling: Caught type error ('mana'). Rejected without crash.\n";
    }

    // Read second corrupt message (syntax error)
    GameStatus gsBad;
    bool successSyntax = zmq_pull_raw(gm_pull, gsBad);
    if (!successSyntax)
    {
        std::cout << "[OK] Error handling: Caught syntax error. Rejected without crash.\n";
    }

    std::cout << "\n>>> ALL TESTS PASSED <<<\n";

    return 0;
}
