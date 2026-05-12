#include "GameLogic.h"
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>

// Hjælpefunktion til at printe en strategi pænt (fx "Drain+Fireball")
std::string stratToString(const std::vector<std::string> &strat)
{
    if (strat.empty())
        return "Ingen";
    std::string s = strat[0];
    for (size_t i = 1; i < strat.size(); i++)
    {
        s += "+" + strat[i];
    }
    return s;
}

// Returnerer 1 hvis p1 vinder, 2 hvis p2 vinder, 0 for uafgjort
int simulateMatchup(const std::vector<std::string> &p1Strat, const std::vector<std::string> &p2Strat, GameLogic &game)
{
    // Tving spillet i gang (virker baade fra LOBBY og GAME_OVER state)
    game.onPlayerAction({"player1", "ready"});
    game.onPlayerAction({"player2", "ready"});

    int max_ticks = 200; // 200 ticks svarer til praecis 10 runder (20 ticks pr. runde)
    int ticks = 0;
    int p1Index = 0;
    int p2Index = 0;
    bool castThisRound = false;

    while (game.getPhase() != GamePhase::GAME_OVER && ticks < max_ticks)
    {
        // Sørg for at vi kun kaster én gang pr CASTING fase
        if (game.getPhase() == GamePhase::CASTING && !castThisRound)
        {
            std::string s1 = p1Strat[p1Index % p1Strat.size()];
            std::string s2 = p2Strat[p2Index % p2Strat.size()];

            game.onSpellMessage({"player1", s1});
            game.onSpellMessage({"player2", s2});

            p1Index++;
            p2Index++;
            castThisRound = true;
        }

        // Gør klar til næste runde når vi er i resolving
        if (game.getPhase() == GamePhase::RESOLVING)
        {
            castThisRound = false;
        }

        // Kør spillet en "sekund" frem i tiden
        game.tick();
        ticks++;
    }

    GameStatus status = game.getStatusForPlayer("player1");
    if (ticks >= max_ticks)
    {
        return 0; // Uafgjort pga timeout (eller heale uendeligt)
    }
    else if (status.winner == "Player 1")
    {
        return 1;
    }
    else if (status.winner == "Player 2")
    {
        return 2;
    }
    return 0;
}

void generateStrategies(const std::vector<std::string> &spells, std::vector<std::string> current, int maxLength, std::vector<std::vector<std::string>> &allStrats)
{
    if (!current.empty())
    {
        allStrats.push_back(current);
    }
    if (current.size() == maxLength)
    {
        return;
    }
    for (const auto &spell : spells)
    {
        std::vector<std::string> next = current;
        next.push_back(spell);
        generateStrategies(spells, next, maxLength, allStrats);
    }
}

struct StratResult
{
    std::vector<std::string> strat;
    int wins = 0;
    int draws = 0;
    int losses = 0;
};

int main()
{
    std::cout << "=== SPELLMASTER BALANCE SIMULATOR ===\n";
    std::cout << "Initialiserer spil og indlaeser spells...\n";

    GameLogic game;
    if (!game.loadSpells("../gamemaster/spells.json"))
    {
        return 1;
    }

    // Opdateret liste der præcist matcher din spells.json
    std::vector<std::string> availableSpells = {
        "Fireball", "Lightning", "Shield", "Heal", "Drain"};

    std::vector<std::vector<std::string>> strategies;
    std::vector<std::string> emptyStrat;
    int maxSequenceLength = 3;

    generateStrategies(availableSpells, emptyStrat, maxSequenceLength, strategies);

    std::vector<StratResult> results(strategies.size());
    for (size_t i = 0; i < strategies.size(); i++)
    {
        results[i].strat = strategies[i];
    }

    int totalMatches = strategies.size() * strategies.size();
    std::cout << "\nGenereret " << strategies.size() << " unikke strategier (laengde 1 til " << maxSequenceLength << ").\n";
    std::cout << "Beregner turnering (" << totalMatches << " kampe alle-mod-alle)... Vent venligst.\n\n";

    std::streambuf *oldCoutStreamBuf = std::cout.rdbuf();
    std::cout.rdbuf(nullptr);

    for (size_t i = 0; i < strategies.size(); i++)
    {
        for (size_t j = 0; j < strategies.size(); j++)
        {
            int result = simulateMatchup(strategies[i], strategies[j], game);
            if (result == 1)
            {
                results[i].wins++;
                results[j].losses++;
            }
            else if (result == 2)
            {
                results[i].losses++;
                results[j].wins++;
            }
            else
            {
                results[i].draws++;
                results[j].draws++;
            }
        }
    }

    std::cout.rdbuf(oldCoutStreamBuf);

    std::sort(results.begin(), results.end(), [](const StratResult &a, const StratResult &b)
              {
                  if (a.wins != b.wins) return a.wins > b.wins;
                  return a.losses < b.losses; });

    std::cout << "--- TOP 20 BEDSTE STRATEGIER ---\n";
    std::cout << std::left << std::setw(35) << "Strategi" << std::setw(10) << "Wins" << std::setw(10) << "Losses" << std::setw(10) << "Draws" << "\n";
    std::cout << "-----------------------------------------------------------------\n";

    for (int i = 0; i < std::min(20, (int)results.size()); i++)
    {
        std::cout << std::left << std::setw(35) << stratToString(results[i].strat)
                  << std::setw(10) << results[i].wins << std::setw(10) << results[i].losses << std::setw(10) << results[i].draws << "\n";
    }

    std::cout << "\n-----------------------------------------------------------------\n";
    std::cout << "BUND 5 (De daarligste strategier):\n";
    for (int i = std::max(0, (int)results.size() - 5); i < (int)results.size(); i++)
    {
        std::cout << std::left << std::setw(35) << stratToString(results[i].strat)
                  << std::setw(10) << results[i].wins << std::setw(10) << results[i].losses << std::setw(10) << results[i].draws << "\n";
    }

    return 0;
}