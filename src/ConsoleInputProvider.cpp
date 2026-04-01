#include "ConsoleInputProvider.hpp"
#include <iostream>

void ConsoleInputProvider::initialize()
{
    // Ingen initialisering nødvendig for console input
}

void ConsoleInputProvider::shutdown()
{
    // Ingen oprydning nødvendig for console input
}

int ConsoleInputProvider::getSpellInput(const std::string &playerName)
{
    int input;
    std::cout << "Indtast spell for " << playerName << ": ";
    std::cin >> input;
    return input;
}
