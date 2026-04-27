#include "GameMaster.h"
#include "ConsoleInputProvider.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <chrono>

GameMaster::~GameMaster()
{
    zmqPublisher.close();
    zmqContext.close();
}

GameMaster::GameMaster()
    : zmqContext(1),
      zmqPublisher(zmqContext, zmq::socket_type::pub),
      currentState(GameState::STARTING),
      roundNumber(1),
      player1("Spiller 1", 100, 50),
      player2("Spiller 2", 100, 50),
      p1Input({"Spiller 1", ""}),
      p2Input({"Spiller 2", ""}),
      p1Spell(Spell::Miss()),
      p2Spell(Spell::Miss()),
      inputProvider(std::make_unique<ConsoleInputProvider>())
{
    int linger = 0;
    zmqPublisher.set(zmq::sockopt::linger, linger);
    zmqPublisher.bind("tcp://*:5555");
    loadSpells("spells.json");
}

GameMaster::GameMaster(std::unique_ptr<InputProvider> provider)
    : zmqContext(1),
      zmqPublisher(zmqContext, zmq::socket_type::pub),
      currentState(GameState::STARTING),
      roundNumber(1),
      player1("Spiller 1", 100, 50),
      player2("Spiller 2", 100, 50),
      p1Input(),
      p2Input(),
      p1Spell(Spell::Miss()),
      p2Spell(Spell::Miss()),
      inputProvider(std::move(provider))
{
    int linger = 0;
    zmqPublisher.set(zmq::sockopt::linger, linger); // Luk socket øjeblikkeligt uden at vente
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

        // TODO: Fjern hardcoded spells senere - implementer kun JSON-baseret spell loading for production
        // Dette gør koden mere professionel og fleksibel for rapporten
        spellDatabase[1] = Spell::Fireball();
        spellDatabase[2] = Spell::Lightning();
        spellDatabase[3] = Spell::Shield();
        spellDatabase[4] = Spell::Heal();
    }
}

Spell GameMaster::spellMessageToSpell(const SpellMessage &msg)
{
    // Find spell med navn i databasen
    for (const auto &[id, spell] : spellDatabase)
    {
        if (spell.name == msg.spellName)
        {
            return spell;
        }
    }
    return Spell::Miss(); // Hvis spell navn ikke findes, blive det et Miss
}

void GameMaster::run()
{
    bool isRunning = true;

    std::cout << "--- Velkommen til Spellmaster GameMaster ---" << std::endl;
    inputProvider->initialize(); // Initialisér input provider (åbner netværksforbindelser om nødvendigt)

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
        {
            bool p1HasInput = !p1Input.spellName.empty();
            bool p2HasInput = !p2Input.spellName.empty();

            auto lastBroadcast = std::chrono::high_resolution_clock::now();

            while (!p1HasInput || !p2HasInput)
            {
                auto now = std::chrono::high_resolution_clock::now();
                auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastBroadcast).count();

                // Send GameStatus every 1 second
                if (elapsedMs >= 1000)
                {
                    GameStatus status1 = createGameStatus(true);
                    GameStatus status2 = createGameStatus(false);

                    zmq::message_t msg1(sizeof(GameStatus));
                    zmq::message_t msg2(sizeof(GameStatus));
                    memcpy(msg1.data(), &status1, sizeof(GameStatus));
                    memcpy(msg2.data(), &status2, sizeof(GameStatus));

                    zmqPublisher.send(msg1, zmq::send_flags::none);
                    zmqPublisher.send(msg2, zmq::send_flags::none);

                    std::cout << "[Broadcasting] Runde " << roundNumber << " - Venter på input..." << std::endl;
                    lastBroadcast = now;
                }

                // Try to get input from player 1
                if (!p1HasInput)
                {
                    SpellMessage msg = inputProvider->getSpellInput(player1.getName());
                    if (!msg.spellName.empty())
                    {
                        p1Input = msg;
                        p1HasInput = true;
                        std::cout << "[Input] " << player1.getName() << " valgte: " << p1Input.spellName << std::endl;
                    }
                }

                // Try to get input from player 2
                if (!p2HasInput)
                {
                    SpellMessage msg = inputProvider->getSpellInput(player2.getName());
                    if (!msg.spellName.empty())
                    {
                        p2Input = msg;
                        p2HasInput = true;
                        std::cout << "[Input] " << player2.getName() << " valgte: " << p2Input.spellName << std::endl;
                    }
                }
            }

            p1Spell = spellMessageToSpell(p1Input);
            p2Spell = spellMessageToSpell(p2Input);

            // Reset inputs for next round
            p1Input = SpellMessage{"", ""};
            p2Input = SpellMessage{"", ""};

            currentState = GameState::CALCULATING;
            break;
        }

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
        }
        break;

        case GameState::BROADCASTING:
        {
            // Send game status for both players via ZeroMQ
            GameStatus status1 = createGameStatus(true);
            GameStatus status2 = createGameStatus(false);

            // Send both structs as binary data
            zmq::message_t msg1(sizeof(GameStatus));
            memcpy(msg1.data(), &status1, sizeof(GameStatus));
            zmqPublisher.send(msg1, zmq::send_flags::none);

            zmq::message_t msg2(sizeof(GameStatus));
            memcpy(msg2.data(), &status2, sizeof(GameStatus));
            zmqPublisher.send(msg2, zmq::send_flags::none);

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

            inputProvider->shutdown(); // Ryd op efter input provider (lukker netværksforbindelser)
            isRunning = false;
            break;
        }
    }
}

GameStatus GameMaster::createGameStatus(bool isPlayer1) const
{
    GameStatus status;
    status.round = roundNumber;
    status.phase = GamePhase::CASTING; // For nu, kan udvides senere

    // Selv (self) er den aktuelle spiller
    if (isPlayer1)
    {
        status.self.name = player1.getName();
        status.self.hp = player1.getHp();
        status.self.maxHp = 100; // TODO: Get from Player object
        status.self.mana = player1.getMana();
        status.self.maxMana = 50; // TODO: Get from Player object
        status.self.spellCast = p1Spell.name;
        status.self.spellDescription = p1Spell.damage > 0
                                           ? std::to_string(p1Spell.damage) + " dmg"
                                       : p1Spell.healAmount > 0
                                           ? "+" + std::to_string(p1Spell.healAmount) + " HP"
                                           : "Shield";

        // Modstander (opponent) er den anden spiller
        status.opponent.name = player2.getName();
        status.opponent.hp = player2.getHp();
        status.opponent.maxHp = 100;
        status.opponent.mana = player2.getMana();
        status.opponent.maxMana = 50;
        status.opponent.spellCast = p2Spell.name;
    }
    else
    {
        // Omvendt for Player 2
        status.self.name = player2.getName();
        status.self.hp = player2.getHp();
        status.self.maxHp = 100;
        status.self.mana = player2.getMana();
        status.self.maxMana = 50;
        status.self.spellCast = p2Spell.name;
        status.self.spellDescription = p2Spell.damage > 0
                                           ? std::to_string(p2Spell.damage) + " dmg"
                                       : p2Spell.healAmount > 0
                                           ? "+" + std::to_string(p2Spell.healAmount) + " HP"
                                           : "Shield";

        status.opponent.name = player1.getName();
        status.opponent.hp = player1.getHp();
        status.opponent.maxHp = 100;
        status.opponent.mana = player1.getMana();
        status.opponent.maxMana = 50;
        status.opponent.spellCast = p1Spell.name;
    }

    return status;
}