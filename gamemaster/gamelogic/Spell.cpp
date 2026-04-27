#include "Spell.h"

Spell Spell::Miss()
{
    return Spell{SpellType::Miss, "Miss", 0, 0, 0, false, ""};
}

Spell Spell::Fireball()
{
    return Spell{SpellType::Fireball, "Fireball", 20, 10, 0, false, "20 dmg"};
}

Spell Spell::Lightning()
{
    return Spell{SpellType::Lightning, "Lightning", 30, 20, 0, false, "30 dmg"};
}

Spell Spell::Shield()
{
    return Spell{SpellType::Shield, "Shield", 0, 15, 0, true, "1/2 dmg"};
}

Spell Spell::Heal()
{
    return Spell{SpellType::Heal, "Heal", 0, 25, 30, false, "+30 HP"};
}
