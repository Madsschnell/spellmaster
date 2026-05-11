// GameLogic.h
// Core game logic: phase management, spell resolution, player state

#pragma once
#include "Player.h"
#include "GameStatus.h"
#include "SpellMessage.h"
#include "PlayerAction.h"
#include <nlohmann/json.hpp>
#include <string>
#include <map>

/// Spell data loaded from spells.json at startup
struct Spell {
    std::string name;
    int damage;
    int manaCost;
    int healAmount;
    bool isDefensive;
    std::string description;  ///< Short effect text for UI, e.g. "20 dmg"
};

/// Manages game state and spell resolution for two players.
/// Pure game logic with no ZeroMQ dependency; main.cpp handles networking.
class GameLogic {
    GamePhase phase = GamePhase::LOBBY;
    int round = 0;
    int castTimeLeft = 0;

    Player player1;
    Player player2;

    std::string p1SpellName;
    std::string p2SpellName;

    EffectInfo p1Effect;
    EffectInfo p2Effect;

    bool p1Ready = false;
    bool p2Ready = false;
    bool p1Shielded = false;
    bool p2Shielded = false;

    std::string p1SpellRejected;
    std::string p2SpellRejected;

    std::string winner;
    std::map<std::string, Spell> spells;

public:
    GameLogic();

    /// Load spell definitions from a JSON file
    /// @param path Path to spells.json
    /// @return true if file was loaded and parsed successfully
    bool loadSpells(const std::string& path);

    /// Process an incoming player action (e.g. "ready")
    /// @param action PlayerAction received via ZeroMQ
    void onPlayerAction(const PlayerAction& action);

    /// Process an incoming spell cast from a player
    /// @param msg SpellMessage received via ZeroMQ
    void onSpellMessage(const SpellMessage& msg);

    /// Advance game state by one tick (called once per second by main loop)
    void tick();

    /// Build a GameStatus for a specific player with self/opponent swapped
    /// @param playerId "player1" or "player2"
    /// @return GameStatus ready to publish via ZeroMQ
    GameStatus getStatusForPlayer(const std::string& playerId);

    /// @return Current game phase
    GamePhase getPhase() const;

private:
    void startGame();
    void resolve();
    void nextRound();
    EffectInfo computeEffect(int delta, bool shielded);
    std::string getStatusMessage(const std::string& playerId);
};
