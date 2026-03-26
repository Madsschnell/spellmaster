#pragma once
#include <string>

enum class SpellType
{
    Miss,
    Fireball,
    Lightning,
    Shield,
    Heal
};

struct Spell
{
    SpellType type;
    std::string name;
    int damage;
    int manaCost;
    int healAmount;
    bool isDefensive;

    // Factory methods for creating spells
    static Spell Miss();
    static Spell Fireball();
    static Spell Lightning();
    static Spell Shield();
    static Spell Heal();
};
