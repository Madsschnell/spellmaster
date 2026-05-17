#include "Player.h"

Player::Player(std::string playerName, int startingHp, int startingMana)
    : name(playerName), hp(startingHp), mana(startingMana) {}

std::string Player::getName() const { return name; }
int Player::getHp() const { return hp; }
int Player::getMana() const { return mana; }

void Player::takeDamage(int amount)
{
    hp -= amount;
    if (hp < 0) hp = 0;
}

void Player::heal(int amount)
{
    hp += amount;
    if (hp > 100) hp = 100;
}

void Player::spendMana(int amount)
{
    mana -= amount;
    if (mana < 0) mana = 0;
}

void Player::regenerateMana(int amount)
{
    mana += amount;
    if (mana > 50) mana = 50;
}

bool Player::isAlive() const
{
    return hp > 0;
}
