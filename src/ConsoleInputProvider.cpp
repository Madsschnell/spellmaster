#include "ConsoleInputProvider.hpp"
#include <iostream>

// skal nok slettes, skal bruge zeroQT til selve spillet
// huske at optimere gamemaster efter dette, det vil gøre det pænere
// og mere overskuligt

void ConsoleInputProvider::initialize()
{
    // Ingen initialisering nødvendig for console input
}

void ConsoleInputProvider::shutdown()
{
    // Ingen oprydning nødvendig for console input
}

SpellMessage ConsoleInputProvider::getSpellInput(const std::string &playerName)
{
    std::string spellName;
    std::cout << "Indtast spell navn for " << playerName << " (Fireball/Lightning/Shield/Heal): ";
    std::cin >> spellName;
    return SpellMessage{playerName, spellName};
}
