#include "Spell.hpp"

Spell Spell::Miss()
{
    return Spell{SpellType::Miss, "Miss", 0, 0, 0, false};
}

Spell Spell::Fireball()
{
    return Spell{SpellType::Fireball, "Fireball", 20, 10, 0, false};
}

Spell Spell::Lightning()
{
    return Spell{SpellType::Lightning, "Lightning", 30, 20, 0, false};
}

Spell Spell::Shield()
{
    return Spell{SpellType::Shield, "Shield", 0, 15, 0, true};
}

Spell Spell::Heal()
{
    return Spell{SpellType::Heal, "Heal", 0, 25, 30, false};
}
