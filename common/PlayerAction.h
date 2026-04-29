// PlayerAction.h
// Sent from UI to GameMaster via ZeroMQ (PUSH)

#pragma once
#include <string>

/// Player action message (e.g. pressing the READY button)
struct PlayerAction {
    std::string playerId;  ///< "player1" or "player2"
    std::string action;    ///< Action type, currently only "ready"
};
