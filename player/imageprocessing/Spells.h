#ifndef SPELLS_H
#define SPELLS_H

#include <string>

enum class Spell
{
    NONE = 0,
    FIREBALL,  // circle
    SHIELD,    // square
    LIGHTNING, // zig-zag
    SUMMON,    // triangle
    METEOR,    // star
    SLASH,     // line
    HEAL,      // spiral
};

inline std::string spellToString(Spell s)
{
    switch (s)
    {
    case Spell::FIREBALL:
        return "FIREBALL (Circle)";
    case Spell::SHIELD:
        return "SHIELD (Square)";
    case Spell::LIGHTNING:
        return "LIGHTNING (Zig-Zag)";
    case Spell::SUMMON:
        return "SUMMON (Triangle)";
    case Spell::METEOR:
        return "METEOR (Star)";
    case Spell::SLASH:
        return "SLASH (Line)";
    case Spell::HEAL:
        return "HEAL (Spiral)";
    default:
        return "Unknown gesture";
    }
}

#endif // SPELLS_H
