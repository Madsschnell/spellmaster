#pragma once
#include <string>

class Player
{
private:
    std::string name;
    int hp;
    int mana;
    bool shieldUp = false;

public:
    // Constructor til at bygge spilleren
    Player(std::string playerName, int startingHp, int startingMana);

    // Getters så GameMaster kan læse værdierne
    std::string getName() const;
    int getHp() const;
    int getMana() const;

    // Funktioner til at ændre tilstanden
    void takeDamage(int amount);
    void heal(int amount);
    void spendMana(int amount);
    void regenerateMana(int amount);
    bool isAlive() const;
    void setShield(bool active); // activere/deactivere
    bool activeShield() const;   // tjekker tilstand
};