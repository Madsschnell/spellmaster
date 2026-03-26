#pragma once
#include <string>

class Player
{
private:
    std::string name;
    int hp;
    int mana;

public:
    // Constructor til at bygge spilleren
    Player(std::string playerName, int startingHp, int startingMana);

    // Getters så GameMaster kan læse værdierne
    std::string getName() const;
    int getHp() const;
    int getMana() const;

    // Funktioner til at ændre tilstanden
    void takeDamage(int amount);
    void spendMana(int amount);
    bool isAlive() const;
};