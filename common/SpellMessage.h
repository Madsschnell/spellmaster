// SpellMessage.h
// Sent from ImageProcessing to GameMaster via ZeroMQ (PUSH)

#pragma once
#include <string>

/// Spell cast message from player to GameMaster.
/// ImageProcessing sends only the spell name from pattern recognition.
/// GameMaster looks up damage, mana cost etc. from spells.json.
struct SpellMessage {
    std::string playerId;   ///< "player1" or "player2"
    std::string spellName;  ///< Spell name matching a key in spells.json
};
