#pragma once
#include <string>

// Sent from ImageProcessing to GameMaster via ZeroMQ (PUSH)
// ImageProcessing only knows the spell name (from pattern recognition)
// GameMaster looks up damage, mana cost etc. from its spell definitions
struct SpellMessage {
    std::string playerId;
    std::string spellName;
};
