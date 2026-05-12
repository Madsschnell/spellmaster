#include <gtest/gtest.h>
#include "GameLogic.h"
#include <fstream>
#include <cstdio>

class GameLogicTest : public ::testing::Test
{
protected:
    GameLogic game;
    std::string testSpellsPath = "test_spells.json";

    // SetUp køres automatisk før hver eneste test
    void SetUp() override
    {
        // 1. Opret en dummy spells.json med forudsigelige værdier til testen
        // Opdateret med de nye spells fra den aktuelle branch
        std::ofstream out(testSpellsPath);
        out << R"({
            "Fireball": { "manaCost": 10, "damage": 20, "heal": 0, "defensive": false, "description": "20 dmg" },
            "Lightning": { "manaCost": 20, "damage": 30, "heal": 0, "defensive": false, "description": "30 dmg" },
            "Shield":    { "manaCost": 15, "damage": 0,  "heal": 0, "defensive": true,  "description": "1/2 dmg" },
            "Heal":      { "manaCost": 25, "damage": 0,  "heal": 30, "defensive": false, "description": "+30 HP" },
            "Drain":     { "manaCost": 30, "damage": 15, "heal": 10, "defensive": false, "description": "15 dmg, +10 HP" }
        })";
        out.close();

        // 2. Indlæs spells i spillet
        ASSERT_TRUE(game.loadSpells(testSpellsPath));
    }

    // TearDown køres automatisk efter hver test
    void TearDown() override
    {
        // Slet den midlertidige json fil
        std::remove(testSpellsPath.c_str());
    }

    // Hjælpefunktion: Spol tiden X sekunder frem
    void tickSeconds(int seconds)
    {
        for (int i = 0; i < seconds; ++i)
        {
            game.tick();
        }
    }

    // Hjælpefunktion: Få spillet i gang (direkte til CASTING fasen)
    void forceGameToCastingPhase()
    {
        game.onPlayerAction({"player1", "ready"});
        game.onPlayerAction({"player2", "ready"});
        tickSeconds(5); // Spol forbi GET_READY
    }
};

// --- TEST CASES ---

TEST_F(GameLogicTest, LobbyRequiresBothPlayersReady)
{
    EXPECT_EQ(game.getPhase(), GamePhase::LOBBY);

    game.onPlayerAction({"player1", "ready"});
    EXPECT_EQ(game.getPhase(), GamePhase::LOBBY); // Kun én er klar

    game.onPlayerAction({"player2", "ready"});
    EXPECT_EQ(game.getPhase(), GamePhase::GET_READY); // Nu starter spillet!
}

TEST_F(GameLogicTest, CombatResolutionDamagesOpponent)
{
    forceGameToCastingPhase();
    ASSERT_EQ(game.getPhase(), GamePhase::CASTING);

    // Player 1 kaster Fireball (20 dmg, 10 mana)
    game.onSpellMessage({"player1", "Fireball"});
    // Player 2 gør intet (bliver et "Miss")

    tickSeconds(10); // Spol tiden forbi CASTING fasen (10 sek)
    EXPECT_EQ(game.getPhase(), GamePhase::RESOLVING);

    GameStatus status = game.getStatusForPlayer("player1");

    EXPECT_EQ(status.self.mana, 50 - 10);    // Player 1 brugte 10 mana
    EXPECT_EQ(status.opponent.hp, 100 - 20); // Player 2 tog 20 skade
}

TEST_F(GameLogicTest, ShieldReducesDamage)
{
    forceGameToCastingPhase();

    // Player 1 kaster Fireball (20 dmg)
    game.onSpellMessage({"player1", "Fireball"});
    // Player 2 kaster Shield
    game.onSpellMessage({"player2", "Shield"});

    tickSeconds(10); // Resolving

    GameStatus p2Status = game.getStatusForPlayer("player2");

    // Normal skade er 20, men Shield halverer skaden
    EXPECT_EQ(p2Status.self.hp, 100 - (20 / 2));
    EXPECT_EQ(p2Status.selfEffect.type, EffectType::SHIELD);
}

// Tilføjet en test for den nye "Drain" spell, som både giver skade og heal
TEST_F(GameLogicTest, DrainDamagesAndHeals)
{
    forceGameToCastingPhase();

    // Player 1 kaster Drain (15 dmg, 10 heal, 30 mana)
    game.onSpellMessage({"player1", "Drain"});
    // Player 2 kaster Lightning for at skade Player 1 (30 dmg), så vi er sikre på at heal fungerer ordentligt.
    game.onSpellMessage({"player2", "Lightning"});

    tickSeconds(10); // Spol tiden forbi CASTING fasen (10 sek)

    GameStatus p1Status = game.getStatusForPlayer("player1");

    // Player 1 startede på 100 hp, fik 30 skade fra Lightning og helte 10 via Drain = 80 HP tilbage.
    EXPECT_EQ(p1Status.self.hp, 100 - 30 + 10);
    // Player 1 brugte 30 mana
    EXPECT_EQ(p1Status.self.mana, 50 - 30);

    // Player 2 tog 15 skade fra Drain
    EXPECT_EQ(p1Status.opponent.hp, 100 - 15);
}
