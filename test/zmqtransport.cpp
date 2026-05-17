#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cassert>
#include <zmq.hpp>

// vores structs
#include "GameStatus.h"
#include "Serialization.h"
#include "SpellMessage.h"
#include "PlayerAction.h"
#include "Transport.h"

using namespace std::chrono_literals;

int main()
{
    std::cout << "Starter modultest for ZMQ og Serialisering...\n\n";

    zmq::context_t context(1);

    // Gamemaster (sender af GameStatus, Modtager af actions/spells)
    zmq::socket_t gm_pub(context, zmq::socket_type::pub);
    gm_pub.bind("tcp://*:5555"); // erstat * med en specific IP addresse

    zmq::socket_t gm_pull(context, zmq::socket_type::pull);
    gm_pull.bind("tcp://*:5556"); // erstat * med en specific IP addresse

    // Player (mdotager af GameStatus, Afsender af actions/spells)
    zmq::socket_t player_sub(context, zmq::socket_type::sub);
    player_sub.connect("tcp://*:5555");                 // erstat * med en specific IP addresse
    player_sub.set(zmq::sockopt::subscribe, "player1"); // sub på topic "player1"

    zmq::socket_t player_push(context, zmq::socket_type::push);
    player_push.connect("tcp://*:5556"); // erstat * med en specific IP addresse

    std::this_thread::sleep_for(100ms);

    // NORMAL DRIFT
    std::cout << "--- Normal drift ---\n";

    // PUB/SUB (GameStatus)
    GameStatus gsOut;
    gsOut.phase = GamePhase::CASTING;
    gsOut.selfEffect.type = EffectType::HEAL;
    zmq_publish(gm_pub, "player1", gsOut);

    std::this_thread::sleep_for(10ms); // Vent

    std::string topic;
    GameStatus gsIn;
    bool hasGameStatus = zmq_subscribe_recv(player_sub, topic, gsIn);

    assert(hasGameStatus);
    assert(topic == "player1");
    assert(gsIn.phase == GamePhase::CASTING);
    assert(gsIn.selfEffect.type == EffectType::HEAL);
    std::cout << "[OK] Serialisering: GameStatus er fejlfrit.\n";

    // PUSH/PULL (SpellMessage)
    SpellMessage smOut{"player1", "Fireball"};
    zmq_push(player_push, smOut);

    std::this_thread::sleep_for(10ms);

    json gmReceivedJson;
    bool hasSpellMsg = zmq_pull_raw(gm_pull, gmReceivedJson);
    assert(hasSpellMsg);

    SpellMessage smIn;
    gmReceivedJson.get_to(smIn); // Parse JSON via Serialization.h overloads
    assert(smIn.spellName == "Fireball");
    std::cout << "[OK] Deserialisering: SpellMessage er fejlfrit.\n";

    // STRESSTEST
    std::cout << "\n--- Stresstest ---\n";

    const int NUM_MESSAGES = 50;

    // Sender 50 concurrently
    for (int i = 0; i < NUM_MESSAGES; ++i)
    {
        PlayerAction pa{"player1", "ready_" + std::to_string(i)};
        zmq_push(player_push, pa);
    }

    std::this_thread::sleep_for(50ms); // Vent 50 ms for at sikre at alle er fremme i PULL-bufferen

    int receivedCount = 0;
    PlayerAction paIn;

    // Læser fra PULL-socket indtil bufferen er tom
    while (zmq_pull_raw(gm_pull, gmReceivedJson))
    {
        gmReceivedJson.get_to(paIn);
        // Tjekker at de ankommer i præcis samme rækkefølge
        assert(paIn.action == "ready_" + std::to_string(receivedCount));
        receivedCount++;
    }

    assert(receivedCount == NUM_MESSAGES);
    std::cout << "[OK] Stresstest: 50 playeraction beskeder sendt og modtaget :).\n";

    // FEJLTEST (Robusthed mod korrupt data)
    std::cout << "\n--- Fejltest ---\n";

    // Korrupt JSON 1: Forkert datatype. 'mana' forventer en int, men vi sender en string ("halvtreds").
    std::string badJsonType = "{\"name\":\"Player\",\"hp\":100,\"maxHp\":100,\"mana\":\"halvtreds\",\"maxMana\":50,\"spellCast\":\"\",\"spellDescription\":\"\"}";
    player_push.send(zmq::buffer(badJsonType), zmq::send_flags::none);

    // Korrupt JSON 2: Malformateret syntaks. Mangler afslutningsklamme.
    std::string badJsonSyntax = "{\"phase\": \"CASTING\"";
    player_push.send(zmq::buffer(badJsonSyntax), zmq::send_flags::none);

    std::this_thread::sleep_for(10ms);

    // Aflæser første korrupte besked
    PlayerInfo piBad;
    bool successType = zmq_pull_raw(gm_pull, piBad);
    if (!successType)
    {
        std::cout << "[OK] Fejlhåndtering: Fanget type-fejl ('mana'). Afvist uden crash.\n";
    }

    // Aflæser anden korrupte besked (Syntaksfejl) direkte til en GameStatus struct
    GameStatus gsBad;
    bool successSyntax = zmq_pull_raw(gm_pull, gsBad);
    if (!successSyntax)
    {
        std::cout << "[OK] Fejlhåndtering: Fanget syntaks-fejl. Afvist uden crash.\n";
    }

    std::cout << "\n>>> ALLE TESTS PASSED!!!!! 🚀 <<<\n";

    return 0;
}