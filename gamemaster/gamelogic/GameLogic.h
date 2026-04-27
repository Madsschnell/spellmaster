#pragma once
#include "Player.h"
#include "Spell.h"
#include "gamestatus.h"
#include "spellmessage.h"
#include "playeraction.h"
#include <nlohmann/json.hpp>
#include <string>
#include <map>

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

    std::string winner;
    std::map<std::string, Spell> spells;

public:
    GameLogic();

    bool loadSpells(const std::string& path);
    void onPlayerAction(const PlayerAction& action);
    void onSpellMessage(const SpellMessage& msg);
    void tick();
    GameStatus getStatusForPlayer(const std::string& playerId);
    GamePhase getPhase() const;

private:
    void startGame();
    void resolve();
    void nextRound();
    EffectInfo computeEffect(int delta, bool shielded);
    std::string getStatusMessage(const std::string& playerId);
};
