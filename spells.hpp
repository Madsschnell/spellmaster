#ifndef SPELLS_HPP
#define SPELLS_HPP

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
    BOTCHED    // failed / not recognised / timeout
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
    case Spell::BOTCHED:
        return "BOTCHED (Failed Spell)";
    default:
        return "NO SPELL";
    }
}

#endif // SPELLS_HPP
