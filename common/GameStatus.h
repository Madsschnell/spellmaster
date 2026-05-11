// GameStatus.h
// Shared data structures between GameMaster and Player UI.
// GameMaster fills GameStatus and publishes it via ZeroMQ.
// UI receives it and updates the display.

#pragma once
#include <string>

/// Game phases as seen from the UI perspective
enum class GamePhase {
    LOBBY,      ///< Waiting for players to press ready
    GET_READY,  ///< Countdown before casting begins
    CASTING,    ///< Round active, players are casting spells
    RESOLVING,  ///< Both spells received, showing results
    GAME_OVER   ///< Winner determined, game finished
};

/// Status for one player (from the UI's perspective)
struct PlayerInfo {
    std::string name = "Player";
    int hp = 100;
    int maxHp = 100;
    int mana = 50;
    int maxMana = 50;
    std::string spellCast = "";         ///< Empty if not cast yet this round
    std::string spellDescription = "";  ///< Effect text, e.g. "20 dmg"
};

/// Effect types for UI coloring (sent by GameMaster)
enum class EffectType {
    NONE,     ///< No effect to display
    DAMAGE,   ///< Red
    HEAL,     ///< Green
    SHIELD,   ///< Blue
    NEUTRAL   ///< Default/grey
};

/// Visual effect info for a single player after spell resolution
struct EffectInfo {
    EffectType type = EffectType::NONE;
    std::string text = "";  ///< e.g. "-20 HP", "+30 HP", "Shielded!"
};

/// Complete game status sent from GameMaster to each Player UI.
/// GameMaster creates one per player with self/opponent swapped.
struct GameStatus {
    GamePhase phase = GamePhase::LOBBY;
    int round = 0;
    int castTimeLeft = 10;       ///< Seconds remaining in current phase
    PlayerInfo self;             ///< This player's info
    PlayerInfo opponent;         ///< The other player's info
    EffectInfo selfEffect;       ///< Effect shown on self panel
    EffectInfo opponentEffect;   ///< Effect shown on opponent panel
    std::string winner = "";     ///< Empty until GAME_OVER
    std::string statusMessage = "Waiting for players...";
    std::string spellRejected = ""; ///< Non-empty if spell was rejected (e.g. not enough mana)
};
