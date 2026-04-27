#pragma once
#include "Player.hpp"
#include "Spell.hpp"
#include "InputProvider.hpp"
#include "SpellMessage.hpp"
#include "GameStatus.hpp"
#include <iostream>
#include <string>
#include <memory>
#include <map>
#include <zmq.hpp>

// Vores State Machine
enum class GameState
{
    STARTING,
    INPUT_PHASE,
    CALCULATING,
    BROADCASTING,
    GAME_OVER
};

class GameMaster
{
private:
    zmq::context_t zmqContext;
    zmq::socket_t zmqPublisher;

    GameState currentState;
    Player player1;
    Player player2;
    int roundNumber;

    std::unique_ptr<InputProvider> inputProvider;

    SpellMessage p1Input;
    SpellMessage p2Input;

    Spell p1Spell;
    Spell p2Spell;

    std::map<int, Spell> spellDatabase;
    void loadSpells(const std::string &filepath);

    Spell spellMessageToSpell(const SpellMessage &msg);

public:
    GameMaster();
    explicit GameMaster(std::unique_ptr<InputProvider> provider);
    ~GameMaster();
    void run();

    GameStatus createGameStatus(bool isPlayer1) const;
};