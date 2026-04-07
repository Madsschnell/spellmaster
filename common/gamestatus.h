// gamestatus.h
// Shared data structure between GameMaster and Player UI.
// GameMaster fills this struct and publishes it via ZeroMQ.
// UI receives it and updates the display.
//
// Note: GameMaster uses "GameState" internally for its state machine enum.
// This struct is called "GameStatus" to avoid naming conflicts.

#pragma once
#include <string>

// Game phases as seen from the UI perspective
enum class GamePhase {
    LOBBY,      // Waiting for players to press ready
    GET_READY,  // Countdown before casting begins (e.g. 5s)
    CASTING,    // Round active, players are casting spells (timer running)
    RESOLVING,  // Both spells received, showing results (e.g. 5s)
    GAME_OVER   // Winner determined, game finished
};

// Status for one player (from the UI's perspective)
struct PlayerInfo {
    std::string name = "Player";
    int hp = 100;
    int maxHp = 100;
    int mana = 50;
    int maxMana = 50;
    std::string spellCast = "";         // empty if not cast yet this round
    std::string spellDescription = "";  // effect text from GameMaster, e.g. "20 dmg"
};

// Effect types for UI coloring (sent by GameMaster)
enum class EffectType {
    NONE,     // no effect to display
    DAMAGE,   // red
    HEAL,     // green
    SHIELD,   // blue
    NEUTRAL   // default/grey
};

struct EffectInfo {
    EffectType type = EffectType::NONE;
    std::string text = "";  // e.g. "-20 HP", "+30 HP", "Shielded!"
};

// Complete game status sent from GameMaster to each Player UI
// GameMaster creates one per player with self/opponent swapped
struct GameStatus {
    GamePhase phase = GamePhase::LOBBY;
    int round = 0;
    int castTimeLeft = 10;       // seconds remaining in casting phase
    PlayerInfo self;             // "you" from this screen's perspective
    PlayerInfo opponent;         // the other player
    EffectInfo selfEffect;       // effect shown on self panel
    EffectInfo opponentEffect;   // effect shown on opponent panel
    std::string winner = "";     // empty until GAME_OVER
    std::string statusMessage = "Waiting for players...";
};
