#pragma once
#include <string>

// Sent from ImageProcessing/UI to GameMaster via ZeroMQ (port 5556)
// GameMaster receives this and looks up spell details from spellDatabase
struct SpellMessage
{
    std::string playerId;
    std::string spellName;
};
