#include <gtest/gtest.h>
#include "GameLogic.h"
#include <fstream>
#include <cstdio>

class GameLogicTest : public ::testing::Test
{
protected:
    GameLogic game;
    std::string testSpellsPath = "test_spells.json";

    // SetUp runs automatically before each test
    void SetUp() override
    {
        // Create a temporary spells.json with predictable test values
        std::ofstream out(testSpellsPath);
        out << R"({
            "Fireball": { "manaCost": 10, "damage": 20, "heal": 0, "defensive": false, "description": "20 dmg" },
            "Lightning": { "manaCost": 20, "damage": 30, "heal": 0, "defensive": false, "description": "30 dmg" },
            "Shield":    { "manaCost": 15, "damage": 0,  "heal": 0, "defensive": true,  "description": "1/2 dmg" },
            "Heal":      { "manaCost": 25, "damage": 0,  "heal": 30, "defensive": false, "description": "+30 HP" },
            "Drain":     { "manaCost": 30, "damage": 15, "heal": 10, "defensive": false, "description": "15 dmg, +10 HP" }
        })";
        out.close();

        // Load spells into game
        ASSERT_TRUE(game.loadSpells(testSpellsPath));
    }

    // TearDown runs automatically after each test
    void TearDown() override
    {
        // Delete the temporary json file
        std::remove(testSpellsPath.c_str());
    }

    // Helper: advance time by X seconds
    void tickSeconds(int seconds)
    {
        for (int i = 0; i < seconds; ++i)
        {
            game.tick();
        }
    }

    // Helper: start the game and advance directly to CASTING phase
    void forceGameToCastingPhase()
    {
        game.onPlayerAction({"player1", "ready"});
        game.onPlayerAction({"player2", "ready"});
        tickSeconds(5); // Skip past GET_READY
    }
};

// --- TEST CASES ---

TEST_F(GameLogicTest, LobbyRequiresBothPlayersReady)
{
    EXPECT_EQ(game.getPhase(), GamePhase::LOBBY);

    game.onPlayerAction({"player1", "ready"});
    EXPECT_EQ(game.getPhase(), GamePhase::LOBBY); // Only one player ready

    game.onPlayerAction({"player2", "ready"});
    EXPECT_EQ(game.getPhase(), GamePhase::GET_READY); // Game starts
}

TEST_F(GameLogicTest, CombatResolutionDamagesOpponent)
{
    forceGameToCastingPhase();
    ASSERT_EQ(game.getPhase(), GamePhase::CASTING);

    // Player 1 casts Fireball (20 dmg, 10 mana)
    game.onSpellMessage({"player1", "Fireball"});
    // Player 2 does nothing (becomes a "Miss")

    tickSeconds(10); // Advance past CASTING phase (10 sec)
    EXPECT_EQ(game.getPhase(), GamePhase::RESOLVING);

    GameStatus status = game.getStatusForPlayer("player1");

    EXPECT_EQ(status.self.mana, 50 - 10);    // Player 1 spent 10 mana
    EXPECT_EQ(status.opponent.hp, 100 - 20); // Player 2 took 20 damage
}

TEST_F(GameLogicTest, ShieldReducesDamage)
{
    forceGameToCastingPhase();

    // Player 1 casts Fireball (20 dmg)
    game.onSpellMessage({"player1", "Fireball"});
    // Player 2 casts Shield
    game.onSpellMessage({"player2", "Shield"});

    tickSeconds(10); // Resolving

    GameStatus p2Status = game.getStatusForPlayer("player2");

    // Normal damage is 20, but Shield halves it
    EXPECT_EQ(p2Status.self.hp, 100 - (20 / 2));
    EXPECT_EQ(p2Status.selfEffect.type, EffectType::SHIELD);
}

// Test for Drain spell, which both deals damage and heals the caster
TEST_F(GameLogicTest, DrainDamagesAndHeals)
{
    forceGameToCastingPhase();

    // Player 1 casts Drain (15 dmg, 10 heal, 30 mana)
    game.onSpellMessage({"player1", "Drain"});
    // Player 2 casts Lightning (30 dmg) to verify heal works correctly
    game.onSpellMessage({"player2", "Lightning"});

    tickSeconds(10); // Advance past CASTING phase (10 sec)

    GameStatus p1Status = game.getStatusForPlayer("player1");

    // Player 1 started at 100 hp. Drain heals +10 (capped at 100), then
    // Lightning deals 30 damage = 70 HP. Heal is applied before opponent's damage.
    EXPECT_EQ(p1Status.self.hp, 100 - 30);
    // Player 1 spent 30 mana
    EXPECT_EQ(p1Status.self.mana, 50 - 30);

    // Player 2 took 15 damage from Drain
    EXPECT_EQ(p1Status.opponent.hp, 100 - 15);
}
