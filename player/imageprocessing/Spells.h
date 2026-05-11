// Spells.h
// Gesture classification output types and display names

#pragma once

#include <string>

/// Gesture types recognized by GestureClassifier.
/// Each value maps to a drawn shape detected by IR tracking.
enum class Spell
{
    NONE = 0,   ///< Not recognized or too short
    FIREBALL,   ///< Circle gesture
    SHIELD,     ///< Square gesture
    LIGHTNING,  ///< Zig-zag gesture
    DRAIN,      ///< Triangle gesture
    METEOR,     ///< Star gesture
    SLASH,      ///< Line gesture
    HEAL,       ///< Spiral gesture
};

/// Convert gesture type to human-readable string for UI feedback
/// @param s Gesture type from classifier
/// @return Display string including shape name, e.g. "FIREBALL (Circle)"
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
    case Spell::DRAIN:
        return "DRAIN (Triangle)";
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
