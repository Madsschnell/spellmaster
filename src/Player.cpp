#include "Player.hpp"

// Initialiserer spilleren med de værdier, GameMasteren giver os
Player::Player(std::string playerName, int startingHp, int startingMana)
    : name(playerName), hp(startingHp), mana(startingMana) {}

std::string Player::getName() const
{
    return name;
}

int Player::getHp() const
{
    return hp;
}

int Player::getMana() const
{
    return mana;
}

void Player::takeDamage(int amount)
{
    hp -= amount;
    if (hp < 0)
    {
        hp = 0; // Vi sørger for, at HP ikke går i minus
    }
}

void Player::heal(int amount)
{
    hp += amount;

    if (hp > 100)
    {
        hp = 100; // sørger for man ikke kan få mere liv end man startede med
    }
}

void Player::spendMana(int amount)
{
    mana -= amount;
    if (mana < 0)
    {
        mana = 0;
    }
}

bool Player::isAlive() const
{
    return hp > 0;
}