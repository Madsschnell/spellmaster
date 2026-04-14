#pragma once
#include <string>

// Sent from UI to GameMaster via ZeroMQ (PUSH)
// Used when player presses READY or performs other UI actions
struct PlayerAction {
    std::string playerId;
    std::string action;  // "ready"
};
