#include "GameMaster.hpp"
#include "ConsoleInputProvider.hpp"
#include <fstream>

GameMaster::GameMaster()
    : zmqContext(1),
      zmqPublisher(zmqContext, zmq::socket_type::pub),
      currentState(GameState::STARTING),
      roundNumber(1),
      player1("Spiller 1", 100, 50),
      player2("Spiller 2", 100, 50),
      p1Input(0),
      p2Input(0),
      p1Spell(Spell::Miss()),
      p2Spell(Spell::Miss()),
      inputProvider(std::make_unique<ConsoleInputProvider>())
{
    zmqPublisher.bind("tcp://*:5555"); // Åbner port 5555 til broadcasting
    loadSpells("spells.json");
}

GameMaster::GameMaster(std::unique_ptr<InputProvider> provider)
    : zmqContext(1),
      zmqPublisher(zmqContext, zmq::socket_type::pub),
      currentState(GameState::STARTING),
      roundNumber(1),
      player1("Spiller 1", 100, 50),
      player2("Spiller 2", 100, 50),
      p1Input(0),
      p2Input(0),
      p1Spell(Spell::Miss()),
      p2Spell(Spell::Miss()),
      inputProvider(std::move(provider))
{
    zmqPublisher.bind("tcp://*:5555");
    loadSpells("spells.json");
}

void GameMaster::loadSpells(const std::string &filepath)
{
    std::ifstream file(filepath);
    if (file.is_open())
    {
        nlohmann::json j;
        file >> j;

        spellDatabase.clear(); // Ryd indbyggede spells, vi vil kun bruge JSON!

        // Gennemløb alle spells i JSON-filen dynamisk
        for (auto &[key, value] : j.items())
        {
            Spell s = value.value("defensive", false) ? Spell::Shield() : Spell::Fireball();
            s.name = key;
            s.manaCost = value.value("manaCost", 0);
            s.damage = value.value("damage", 0);
            s.healAmount = value.value("heal", 0);

            int id = value.value("id", 0);
            if (id > 0)
            {
                spellDatabase[id] = s;
            }
        }
        std::cout << "[GameMaster] Spells indlaest fra " << filepath << std::endl;
    }
    else
    {
        std::cerr << "[Advarsel] Kunne ikke finde " << filepath << " - bruger indbyggede standard spells." << std::endl;
        spellDatabase[1] = Spell::Fireball();
        spellDatabase[2] = Spell::Lightning();
        spellDatabase[3] = Spell::Shield();
        spellDatabase[4] = Spell::Heal();
    }
}

Spell GameMaster::inputToSpell(int input)
{
    // Prøv at slå spell-nummeret op i vores map
    if (spellDatabase.find(input) != spellDatabase.end())
    {
        return spellDatabase[input];
    }
    return Spell::Miss(); // Hvis spilleren taster noget forkert, bliver det et Miss
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

            std::cout << "Spells: ";
            for (const auto &pair : spellDatabase)
            {
                std::cout << pair.first << "=" << pair.second.name << "(" << pair.second.manaCost << " mana";
                if (pair.second.damage > 0)
                    std::cout << ", " << pair.second.damage << " dmg";
                if (pair.second.healAmount > 0)
                    std::cout << ", +" << pair.second.healAmount << " HP";
                std::cout << "), ";
            }
            std::cout << "0=Miss" << std::endl;

            currentState = GameState::INPUT_PHASE;
            break;

        case GameState::INPUT_PHASE:
            p1Input = inputProvider->getSpellInput(player1.getName());
            p2Input = inputProvider->getSpellInput(player2.getName());

            p1Spell = inputToSpell(p1Input);
            p2Spell = inputToSpell(p2Input);

            currentState = GameState::CALCULATING;
            break;

        case GameState::CALCULATING:
        { // <--- Scope starter her
            std::cout << "Beregner..." << std::endl;

            bool p1CanCast = (p1Spell.manaCost == 0) || (player1.getMana() >= p1Spell.manaCost);
            bool p2CanCast = (p2Spell.manaCost == 0) || (player2.getMana() >= p2Spell.manaCost);

            // ... logik for P1 ...
            if (p1CanCast && p1Spell.type != SpellType::Miss)
            {
                if (p1Spell.manaCost > 0)
                    player1.spendMana(p1Spell.manaCost);
                if (p1Spell.type == SpellType::Shield)
                {
                    player1.setShield(true);
                    std::cout << player1.getName() << " aktiverer sit skjold!" << std::endl;
                }
                if (p1Spell.damage > 0)
                {
                    player2.takeDamage(p1Spell.damage);
                    std::cout << player1.getName() << " kaster " << p1Spell.name << " på " << player2.getName() << " for " << p1Spell.damage << " skade!" << std::endl;
                }
                else if (p1Spell.healAmount > 0)
                {
                    player1.heal(p1Spell.healAmount);
                    std::cout << player1.getName() << " kaster " << p1Spell.name << " og healer sig selv for " << p1Spell.healAmount << " HP!" << std::endl;
                }
            }
            else if (!p1CanCast && p1Spell.type != SpellType::Miss)
            {
                std::cout << player1.getName() << " har ikke nok mana til " << p1Spell.name << "!" << std::endl;
            }

            // ... logik for P2 ...
            if (p2CanCast && p2Spell.type != SpellType::Miss)
            {
                if (p2Spell.manaCost > 0)
                    player2.spendMana(p2Spell.manaCost);
                if (p2Spell.type == SpellType::Shield)
                {
                    player2.setShield(true);
                    std::cout << player2.getName() << " aktiverer sit skjold!" << std::endl;
                }
                if (p2Spell.damage > 0)
                {
                    player1.takeDamage(p2Spell.damage);
                    std::cout << player2.getName() << " kaster " << p2Spell.name << " på " << player1.getName() << " for " << p2Spell.damage << " skade!" << std::endl;
                }
                else if (p2Spell.healAmount > 0)
                {
                    player2.heal(p2Spell.healAmount);
                    std::cout << player2.getName() << " kaster " << p2Spell.name << " og healer sig selv for " << p2Spell.healAmount << " HP!" << std::endl;
                }
            }
            else if (!p2CanCast && p2Spell.type != SpellType::Miss)
            {
                std::cout << player2.getName() << " har ikke nok mana til " << p2Spell.name << "!" << std::endl;
            }

            if (player1.getHp() <= 0 || player2.getHp() <= 0)
            {
                currentState = GameState::GAME_OVER;
            }
            else
            {
                currentState = GameState::BROADCASTING;
            }
        } // <--- HER var din manglende parentes!
        break;

        case GameState::BROADCASTING:
        {
            std::cout << "\nStatus efter runde " << roundNumber << ":" << std::endl;
            std::cout << player1.getName() << " - HP: " << player1.getHp() << ", Mana: " << player1.getMana() << std::endl;
            std::cout << player2.getName() << " - HP: " << player2.getHp() << ", Mana: " << player2.getMana() << std::endl;

            std::string jsonStr = getGameStateJson().dump();
            std::cout << "\n[Broadcaster JSON via ZeroMQ port 5555]\n"
                      << getGameStateJson().dump(4) << "\n"
                      << std::endl;

            zmq::message_t message(jsonStr.size());
            memcpy(message.data(), jsonStr.c_str(), jsonStr.size());
            zmqPublisher.send(message, zmq::send_flags::none);

            roundNumber++;
            currentState = GameState::STARTING;
        }
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
        } // Slut på switch
    } // Slut på while
} // Slut på run()

nlohmann::json GameMaster::getGameStateJson() const
{
    nlohmann::json j;
    j["gameState"] = static_cast<int>(currentState);
    j["roundNumber"] = roundNumber;

    j["player1"] = {
        {"hp", player1.getHp()},
        {"mana", player1.getMana()},
        {"shieldActive", player1.activeShield()}};

    j["player2"] = {
        {"hp", player2.getHp()},
        {"mana", player2.getMana()},
        {"shieldActive", player2.activeShield()}};

    return j;
}