#include "GameLogic.h"
#include <fstream>
#include <iostream>

GameLogic::GameLogic()
    : player1("Player 1", 100, 50)
    , player2("Player 2", 100, 50)
{
}

bool GameLogic::loadSpells(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Could not open " << path << std::endl;
        return false;
    }

    nlohmann::json j;
    file >> j;

    for (auto& [name, data] : j.items()) {
        Spell s;
        s.name = name;
        s.manaCost = data["manaCost"];
        s.damage = data["damage"];
        s.healAmount = data["heal"];
        s.isDefensive = data["defensive"];
        s.description = data["description"];

        spells[name] = s;
    }

    std::cout << "Loaded " << spells.size() << " spells from " << path << std::endl;
    return true;
}

void GameLogic::onPlayerAction(const PlayerAction& action)
{
    if (action.action == "ready") {
        if (action.playerId == "player1") p1Ready = true;
        if (action.playerId == "player2") p2Ready = true;
        std::cout << action.playerId << " is ready" << std::endl;

        if (p1Ready && p2Ready &&
            (phase == GamePhase::LOBBY || phase == GamePhase::GAME_OVER)) {
            startGame();
        }
    }
}

void GameLogic::onSpellMessage(const SpellMessage& msg)
{
    if (phase != GamePhase::CASTING) return;

    auto it = spells.find(msg.spellName);
    if (it == spells.end()) {
        std::cout << "Unknown spell: " << msg.spellName << std::endl;
        return;
    }

    const Spell& spell = it->second;
    bool isP1 = (msg.playerId == "player1");
    Player& caster = isP1 ? player1 : player2;
    std::string& castName = isP1 ? p1SpellName : p2SpellName;

    if (!castName.empty()) return;

    if (caster.getMana() < spell.manaCost) {
        std::string &rejected = isP1 ? p1SpellRejected : p2SpellRejected;
        rejected = "Not enough mana for " + spell.name;
        std::cout << caster.getName() << " does not have enough mana for "
                  << spell.name << std::endl;
        return;
    }

    castName = spell.name;
    caster.spendMana(spell.manaCost);

    // Clear any previous rejection now that a spell was accepted
    std::string &rejected = isP1 ? p1SpellRejected : p2SpellRejected;
    rejected = "";

    if (spell.isDefensive) {
        if (isP1) p1Shielded = true;
        else p2Shielded = true;
    }

    std::cout << caster.getName() << " casts " << spell.name << std::endl;
}

void GameLogic::tick()
{
    switch (phase) {
    case GamePhase::LOBBY:
        break;

    case GamePhase::GET_READY:
        castTimeLeft--;
        if (castTimeLeft <= 0) {
            phase = GamePhase::CASTING;
            castTimeLeft = 10;
            std::cout << "CASTING started (Round " << round << ")" << std::endl;
        }
        break;

    case GamePhase::CASTING:
        castTimeLeft--;
        if (castTimeLeft <= 0) {
            resolve();
        }
        break;

    case GamePhase::RESOLVING:
        castTimeLeft--;
        if (castTimeLeft <= 0) {
            if (!winner.empty()) {
                phase = GamePhase::GAME_OVER;
                std::cout << "GAME OVER: " << winner << " wins!" << std::endl;
            } else {
                nextRound();
            }
        }
        break;

    case GamePhase::GAME_OVER:
        break;
    }
}

GameStatus GameLogic::getStatusForPlayer(const std::string& playerId)
{
    GameStatus gs;
    gs.phase = phase;
    gs.round = round;
    gs.castTimeLeft = castTimeLeft;
    gs.winner = winner;

    auto makeInfo = [this](const Player& p, const std::string& spellName) {
        PlayerInfo info;
        info.name = p.getName();
        info.hp = p.getHp();
        info.maxHp = 100;
        info.mana = p.getMana();
        info.maxMana = 50;
        info.spellCast = spellName;
        if (!spellName.empty() && spellName != "Miss") {
            auto it = spells.find(spellName);
            if (it != spells.end())
                info.spellDescription = it->second.description;
        }
        return info;
    };

    if (playerId == "player1") {
        gs.self = makeInfo(player1, p1SpellName);
        gs.opponent = makeInfo(player2, p2SpellName);
        gs.selfEffect = p1Effect;
        gs.opponentEffect = p2Effect;
        gs.spellRejected = p1SpellRejected;
    } else {
        gs.self = makeInfo(player2, p2SpellName);
        gs.opponent = makeInfo(player1, p1SpellName);
        gs.selfEffect = p2Effect;
        gs.opponentEffect = p1Effect;
        gs.spellRejected = p2SpellRejected;
    }

    gs.statusMessage = getStatusMessage(playerId);
    return gs;
}

GamePhase GameLogic::getPhase() const
{
    return phase;
}

void GameLogic::startGame()
{
    round = 1;
    player1 = Player("Player 1", 100, 50);
    player2 = Player("Player 2", 100, 50);
    p1SpellName = "";
    p2SpellName = "";
    p1Effect = {};
    p2Effect = {};
    p1Ready = false;
    p2Ready = false;
    p1Shielded = false;
    p2Shielded = false;
    p1SpellRejected = "";
    p2SpellRejected = "";
    winner = "";
    phase = GamePhase::GET_READY;
    castTimeLeft = 5;
    std::cout << "Game started! GET_READY (Round " << round << ")" << std::endl;
}

void GameLogic::resolve()
{
    if (p1SpellName.empty()) p1SpellName = "Miss";
    if (p2SpellName.empty()) p2SpellName = "Miss";

    int p1HpBefore = player1.getHp();
    int p2HpBefore = player2.getHp();

    // Apply player 1's spell
    auto it1 = spells.find(p1SpellName);
    if (it1 != spells.end()) {
        const Spell& spell = it1->second;
        if (spell.healAmount > 0) player1.heal(spell.healAmount);
        if (spell.damage > 0) {
            int dmg = spell.damage;
            if (p2Shielded) dmg /= 2;
            player2.takeDamage(dmg);
        }
    }

    // Apply player 2's spell
    auto it2 = spells.find(p2SpellName);
    if (it2 != spells.end()) {
        const Spell& spell = it2->second;
        if (spell.healAmount > 0) player2.heal(spell.healAmount);
        if (spell.damage > 0) {
            int dmg = spell.damage;
            if (p1Shielded) dmg /= 2;
            player1.takeDamage(dmg);
        }
    }

    int p1Delta = player1.getHp() - p1HpBefore;
    int p2Delta = player2.getHp() - p2HpBefore;

    p1Effect = computeEffect(p1Delta, p1Shielded);
    p2Effect = computeEffect(p2Delta, p2Shielded);

    std::cout << "RESOLVING: " << p1SpellName << " vs " << p2SpellName << std::endl;

    if (!player1.isAlive() || !player2.isAlive()) {
        if (player1.getHp() > player2.getHp()) winner = player1.getName();
        else if (player2.getHp() > player1.getHp()) winner = player2.getName();
        else winner = "Nobody";
    }

    phase = GamePhase::RESOLVING;
    castTimeLeft = 5;
}

void GameLogic::nextRound()
{
    round++;
    p1SpellName = "";
    p2SpellName = "";
    p1Effect = {};
    p2Effect = {};
    p1Shielded = false;
    p2Shielded = false;
    p1SpellRejected = "";
    p2SpellRejected = "";

    player1.regenerateMana(10);
    player2.regenerateMana(10);

    phase = GamePhase::GET_READY;
    castTimeLeft = 5;
    std::cout << "GET_READY (Round " << round << ")" << std::endl;
}

EffectInfo GameLogic::computeEffect(int delta, bool shielded)
{
    if (shielded && delta < 0)
        return {EffectType::SHIELD, "Reduced! " + std::to_string(delta) + " HP"};
    if (shielded && delta == 0)
        return {EffectType::SHIELD, "Shielded!"};
    if (delta < 0)
        return {EffectType::DAMAGE, std::to_string(delta) + " HP"};
    if (delta > 0)
        return {EffectType::HEAL, "+" + std::to_string(delta) + " HP"};
    return {};
}

std::string GameLogic::getStatusMessage(const std::string& playerId)
{
    switch (phase) {
    case GamePhase::LOBBY:
        return "Press READY to start";
    case GamePhase::GET_READY:
        return "Get ready!";
    case GamePhase::CASTING: {
        const std::string& castName = (playerId == "player1") ? p1SpellName : p2SpellName;
        if (!castName.empty()) return "Waiting for opponent...";
        return "Cast your spell!";
    }
    case GamePhase::RESOLVING:
        return p1SpellName + " vs " + p2SpellName;
    case GamePhase::GAME_OVER:
        return winner + " wins!";
    }
    return "";
}
