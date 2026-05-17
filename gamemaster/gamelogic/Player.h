// Player.h
// Single player state: HP, mana, and basic operations

#pragma once
#include <string>

/// Represents a player in the game with HP and mana.
/// Used by GameLogic to track and modify player state during spell resolution.
class Player
{
private:
    std::string name;
    int hp;
    int mana;

public:
    /// @param playerName Display name (e.g. "Player 1")
    /// @param startingHp Initial HP (typically 100)
    /// @param startingMana Initial mana (typically 50)
    Player(std::string playerName, int startingHp, int startingMana);

    std::string getName() const;
    int getHp() const;
    int getMana() const;

    /// Apply damage to player (clamped to 0)
    /// @param amount Damage points to subtract from HP
    void takeDamage(int amount);

    /// Heal player (clamped to max HP of 100)
    /// @param amount Health points to add
    void heal(int amount);

    /// Spend mana (clamped to 0)
    /// @param amount Mana points to subtract
    void spendMana(int amount);

    /// Regenerate mana between rounds (clamped to max mana of 50)
    /// @param amount Mana points to add
    void regenerateMana(int amount);

    /// @return true if HP > 0
    bool isAlive() const;
};
