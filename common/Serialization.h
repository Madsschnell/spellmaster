#pragma once
#include "GameStatus.h"
#include "SpellMessage.h"
#include "PlayerAction.h"
#include <stdexcept>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// GamePhase
inline void to_json(json &j, const GamePhase &p)
{
    switch (p)
    {
    case GamePhase::LOBBY:
        j = "LOBBY";
        break;
    case GamePhase::GET_READY:
        j = "GET_READY";
        break;
    case GamePhase::CASTING:
        j = "CASTING";
        break;
    case GamePhase::RESOLVING:
        j = "RESOLVING";
        break;
    case GamePhase::GAME_OVER:
        j = "GAME_OVER";
        break;
    }
}

inline void from_json(const json &j, GamePhase &p)
{
    if (!j.is_string())
        throw std::invalid_argument("GamePhase must be a JSON string");

    std::string s = j.get<std::string>();
    if (s == "LOBBY")
        p = GamePhase::LOBBY;
    else if (s == "GET_READY")
        p = GamePhase::GET_READY;
    else if (s == "CASTING")
        p = GamePhase::CASTING;
    else if (s == "RESOLVING")
        p = GamePhase::RESOLVING;
    else if (s == "GAME_OVER")
        p = GamePhase::GAME_OVER;
    else
        throw std::invalid_argument("Unknown GamePhase value: " + s);
}

// EffectType
inline void to_json(json &j, const EffectType &e)
{
    switch (e)
    {
    case EffectType::NONE:
        j = "NONE";
        break;
    case EffectType::DAMAGE:
        j = "DAMAGE";
        break;
    case EffectType::HEAL:
        j = "HEAL";
        break;
    case EffectType::SHIELD:
        j = "SHIELD";
        break;
    case EffectType::NEUTRAL:
        j = "NEUTRAL";
        break;
    }
}

inline void from_json(const json &j, EffectType &e)
{
    if (!j.is_string())
        throw std::invalid_argument("EffectType must be a JSON string");

    const std::string s = j.get<std::string>();
    if (s == "NONE")
        e = EffectType::NONE;
    else if (s == "DAMAGE")
        e = EffectType::DAMAGE;
    else if (s == "HEAL")
        e = EffectType::HEAL;
    else if (s == "SHIELD")
        e = EffectType::SHIELD;
    else if (s == "NEUTRAL")
        e = EffectType::NEUTRAL;
    else
        throw std::invalid_argument("Unknown EffectType value: " + s);
}
// PlayerInfo
inline void to_json(json &j, const PlayerInfo &p)
{
    j = json{
        {"name", p.name}, {"hp", p.hp}, {"maxHp", p.maxHp}, {"mana", p.mana}, {"maxMana", p.maxMana}, {"spellCast", p.spellCast}, {"spellDescription", p.spellDescription}};
}

inline void from_json(const json &j, PlayerInfo &p)
{
    j.at("name").get_to(p.name);
    j.at("hp").get_to(p.hp);
    j.at("maxHp").get_to(p.maxHp);
    j.at("mana").get_to(p.mana);
    j.at("maxMana").get_to(p.maxMana);
    j.at("spellCast").get_to(p.spellCast);
    j.at("spellDescription").get_to(p.spellDescription);
}

// EffectInfo
inline void to_json(json &j, const EffectInfo &e)
{
    j = json{{"type", e.type}, {"text", e.text}};
}

inline void from_json(const json &j, EffectInfo &e)
{
    j.at("type").get_to(e.type);
    j.at("text").get_to(e.text);
}

// GameStatus
inline void to_json(json &j, const GameStatus &gs)
{
    j = json{
        {"phase", gs.phase}, {"round", gs.round}, {"castTimeLeft", gs.castTimeLeft}, {"self", gs.self}, {"opponent", gs.opponent}, {"selfEffect", gs.selfEffect}, {"opponentEffect", gs.opponentEffect}, {"winner", gs.winner}, {"statusMessage", gs.statusMessage}};
}

inline void from_json(const json &j, GameStatus &gs)
{
    j.at("phase").get_to(gs.phase);
    j.at("round").get_to(gs.round);
    j.at("castTimeLeft").get_to(gs.castTimeLeft);
    j.at("self").get_to(gs.self);
    j.at("opponent").get_to(gs.opponent);
    j.at("selfEffect").get_to(gs.selfEffect);
    j.at("opponentEffect").get_to(gs.opponentEffect);
    j.at("winner").get_to(gs.winner);
    j.at("statusMessage").get_to(gs.statusMessage);
}

// SpellMessage
inline void to_json(json &j, const SpellMessage &sm)
{
    j = json{{"playerId", sm.playerId}, {"spellName", sm.spellName}};
}

inline void from_json(const json &j, SpellMessage &sm)
{
    j.at("playerId").get_to(sm.playerId);
    j.at("spellName").get_to(sm.spellName);
}

// PlayerAction
inline void to_json(json &j, const PlayerAction &pa)
{
    j = json{{"playerId", pa.playerId}, {"action", pa.action}};
}

inline void from_json(const json &j, PlayerAction &pa)
{
    j.at("playerId").get_to(pa.playerId);
    j.at("action").get_to(pa.action);
}

// Typed JSON string helpers for ZeroMQ payloads.
template <typename T>
inline std::string json_to_string(const T &data)
{
    json j = data;
    return j.dump();
}

template <typename T>
inline T json_from_string(const std::string &payload)
{
    json j = json::parse(payload);
    return j.get<T>();
}

template <typename T>
inline bool json_from_string(const std::string &payload, T &out)
{
    try
    {
        out = json_from_string<T>(payload);
        return true;
    }
    catch (const json::exception &)
    {
        return false;
    }
}