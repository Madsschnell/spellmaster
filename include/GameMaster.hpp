#pragma once
#include "Player.hpp"
#include "Spell.hpp"
#include "InputProvider.hpp"
#include <iostream>
#include <string>
#include <memory>
#include <map>
#include <nlohmann/json.hpp>
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
    // ZeroMQ netværk til at broadcaste (sende) spillets tilstand
    zmq::context_t zmqContext;
    zmq::socket_t zmqPublisher;

    GameState currentState;
    Player player1;
    Player player2;
    int roundNumber;

    // Input provider - kan være ConsoleInputProvider eller netværksbaseret
    std::unique_ptr<InputProvider> inputProvider;

    // Midlertidige variabler til at gemme terminal-input
    int p1Input;
    int p2Input;

    // Valgte spells til denne runde
    Spell p1Spell;
    Spell p2Spell;

    // Database til at gemme vores spells indlæst fra JSON
    std::map<int, Spell> spellDatabase;
    void loadSpells(const std::string &filepath);

    // Hjælpefunktion til at konvertere input til Spell
    Spell inputToSpell(int input);

public:
    GameMaster();
    explicit GameMaster(std::unique_ptr<InputProvider> provider);
    void run(); // Selve spil-loopet

    // Genererer en JSON-repræsentation af spillets tilstand til BROADCASTING
    nlohmann::json getGameStateJson() const;
};