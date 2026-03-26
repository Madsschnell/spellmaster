#include "GameMaster.hpp"
#include "ConsoleInputProvider.hpp"

GameMaster::GameMaster()
    : currentState(GameState::STARTING),
      roundNumber(1),
      player1("Spiller 1", 100, 50),
      player2("Spiller 2", 100, 50),
      p1Input(0),
      p2Input(0),
      p1Spell(Spell::Miss()),
      p2Spell(Spell::Miss()),
      inputProvider(std::make_unique<ConsoleInputProvider>()) {}

GameMaster::GameMaster(std::unique_ptr<InputProvider> provider)
    : currentState(GameState::STARTING),
      roundNumber(1),
      player1("Spiller 1", 100, 50),
      player2("Spiller 2", 100, 50),
      p1Input(0),
      p2Input(0),
      p1Spell(Spell::Miss()),
      p2Spell(Spell::Miss()),
      inputProvider(std::move(provider)) {}

Spell GameMaster::inputToSpell(int input)
{
    switch (input)
    {
    case 1:
        return Spell::Fireball();
    case 2:
        return Spell::Lightning();
    case 3:
        return Spell::Shield();
    case 4:
        return Spell::Heal();
    default:
        return Spell::Miss();
    }
}

void GameMaster::run()
{
    bool isRunning = true;

    std::cout << "--- Velkommen til Spellmaster GameMaster ---" << std::endl;

    while (isRunning)
    {
        switch (currentState)
        {
        case GameState::STARTING:
            std::cout << "\n[Runde " << roundNumber << " starter!]" << std::endl;
            std::cout << "Spells: 1=Fireball(10 mana, 20 dmg), 2=Lightning(20 mana, 30 dmg),"
                      << " 3=Shield(15 mana), 4=Heal(25 mana, +30 HP), 0=Miss" << std::endl;
            currentState = GameState::INPUT_PHASE;
            break;

        case GameState::INPUT_PHASE:
            // Bruger InputProvider - kan være console eller netværk
            p1Input = inputProvider->getSpellInput(player1.getName());
            p2Input = inputProvider->getSpellInput(player2.getName());

            // Konverter input til Spell objekter
            p1Spell = inputToSpell(p1Input);
            p2Spell = inputToSpell(p2Input);

            currentState = GameState::CALCULATING;
            break;

        case GameState::CALCULATING:
        {
            std::cout << "Beregner..." << std::endl;

            // Tjek om spillerne har nok mana til deres valgte spells
            bool p1CanCast = (p1Spell.manaCost == 0) || (player1.getMana() >= p1Spell.manaCost);
            bool p2CanCast = (p2Spell.manaCost == 0) || (player2.getMana() >= p2Spell.manaCost);

            // Håndter Player 1's spell
            if (p1CanCast && p1Spell.type != SpellType::Miss)
            {
                // Brug mana
                if (p1Spell.manaCost > 0)
                {
                    player1.spendMana(p1Spell.manaCost);
                }

                // Anvend effekt
                if (p1Spell.damage > 0)
                {
                    player2.takeDamage(p1Spell.damage);
                    std::cout << player1.getName() << " kaster " << p1Spell.name
                              << " på " << player2.getName() << " for " << p1Spell.damage << " skade!" << std::endl;
                }
                else if (p1Spell.healAmount > 0)
                {
                    // Heal logic (ikke implementeret i Player endnu)
                    std::cout << player1.getName() << " kaster " << p1Spell.name << "!" << std::endl;
                }
            }
            else if (!p1CanCast && p1Spell.type != SpellType::Miss)
            {
                std::cout << player1.getName() << " har ikke nok mana til " << p1Spell.name << "!" << std::endl;
            }

            // Håndter Player 2's spell
            if (p2CanCast && p2Spell.type != SpellType::Miss)
            {
                // Brug mana
                if (p2Spell.manaCost > 0)
                {
                    player2.spendMana(p2Spell.manaCost);
                }

                // Anvend effekt
                if (p2Spell.damage > 0)
                {
                    player1.takeDamage(p2Spell.damage);
                    std::cout << player2.getName() << " kaster " << p2Spell.name
                              << " på " << player1.getName() << " for " << p2Spell.damage << " skade!" << std::endl;
                }
                else if (p2Spell.healAmount > 0)
                {
                    std::cout << player2.getName() << " kaster " << p2Spell.name << "!" << std::endl;
                }
            }
            else if (!p2CanCast && p2Spell.type != SpellType::Miss)
            {
                std::cout << player2.getName() << " har ikke nok mana til " << p2Spell.name << "!" << std::endl;
            }

            // Tjek win condition
            if (player1.getHp() <= 0 || player2.getHp() <= 0)
            {
                currentState = GameState::GAME_OVER;
            }
            else
            {
                currentState = GameState::BROADCASTING;
            }
            break;
        }

        case GameState::BROADCASTING:
            std::cout << "\nStatus efter runde " << roundNumber << ":" << std::endl;
            std::cout << player1.getName() << " - HP: " << player1.getHp() << ", Mana: " << player1.getMana() << std::endl;
            std::cout << player2.getName() << " - HP: " << player2.getHp() << ", Mana: " << player2.getMana() << std::endl;

            roundNumber++;
            currentState = GameState::STARTING; // Start næste runde
            break;

        case GameState::GAME_OVER:
            std::cout << "\n=== SPILLET ER SLUT! ===" << std::endl;
            if (player1.getHp() > player2.getHp())
                std::cout << player1.getName() << " VINDER!" << std::endl;
            else if (player2.getHp() > player1.getHp())
                std::cout << player2.getName() << " VINDER!" << std::endl;
            else
                std::cout << "Det blev uafgjort!" << std::endl;

            isRunning = false;
            break;
        }
    }
}