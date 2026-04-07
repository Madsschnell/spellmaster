// spellmaster_ui.cpp — SpellMaster Player UI (Sprint 1)
//
// Compile (desktop):
//   g++ spellmaster_ui.cpp -o spellmaster_ui $(pkg-config --cflags --libs Qt6Widgets) -fPIC
//
// Run modes:
//   ./spellmaster_ui              Normal mode (single screen, waits for GameStatus)
//   ./spellmaster_ui --test       Test mode (two player windows + controller)
//   RPi: sudo XDG_RUNTIME_DIR=/run/user/1000 cage -- ./spellmaster_ui
//
// Normal mode: frameless window, hidden cursor (for RPi kiosk).
// Test mode: windowed with cursor (for desktop development).

#include <QApplication>
#include <QWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QKeyEvent>
#include <QTimer>
#include "../../common/gamestatus.h"
#include <algorithm>
#include <functional>
#include <string>
#include <cstdlib>
#include <ctime>

static const char* UI_VERSION = "v0.1.0";

// ================================================================
//  PRODUCTION CODE — runs on both RPi and desktop
// ================================================================

struct StatBar {
    QProgressBar *bar;
    QLabel *text;

    QWidget* create(const QString &name, const QString &barColor) {
        auto *container = new QWidget;
        auto *layout = new QHBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        auto *label = new QLabel(name);
        label->setFixedWidth(50);
        label->setStyleSheet("color: #cccccc; font-size: 14px;");
        bar = new QProgressBar;
        bar->setRange(0, 100);
        bar->setValue(100);
        bar->setTextVisible(false);
        bar->setFixedHeight(20);
        bar->setStyleSheet(
            "QProgressBar { background: #333333; border: 1px solid #555555; border-radius: 3px; }"
            "QProgressBar::chunk { background: " + barColor + "; border-radius: 2px; }"
        );
        text = new QLabel("100/100");
        text->setFixedWidth(80);
        text->setStyleSheet("color: #cccccc; font-size: 13px;");
        text->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(label);
        layout->addWidget(bar);
        layout->addWidget(text);
        return container;
    }

    void update(int value, int max) {
        bar->setRange(0, max);
        bar->setValue(value);
        text->setText(QString("%1/%2").arg(value).arg(max));
    }
};

struct PlayerPanel {
    QLabel *nameLabel;
    QLabel *playerIdLabel;
    StatBar hpBar;
    StatBar manaBar;
    QLabel *effectLabel;
    QLabel *spellLabel;

    QWidget* create(const QString &title) {
        auto *panel = new QWidget;
        auto *layout = new QVBoxLayout(panel);
        layout->setSpacing(8);
        nameLabel = new QLabel(title);
        nameLabel->setStyleSheet("color: white; font-size: 20px; font-weight: bold;");
        nameLabel->setAlignment(Qt::AlignCenter);
        playerIdLabel = new QLabel("");
        playerIdLabel->setStyleSheet("color: #555555; font-size: 11px;");
        playerIdLabel->setAlignment(Qt::AlignCenter);
        effectLabel = new QLabel("");
        effectLabel->setStyleSheet(
            "color: #666666; font-size: 22px; font-weight: bold; "
            "background: #111122; border: 1px solid #222244; border-radius: 6px; padding: 10px;"
        );
        effectLabel->setAlignment(Qt::AlignCenter);
        spellLabel = new QLabel("Spell: ---");
        spellLabel->setStyleSheet("color: #aaaaaa; font-size: 14px;");
        spellLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(nameLabel);
        layout->addWidget(playerIdLabel);
        layout->addWidget(hpBar.create("HP", "#44bb44"));
        layout->addWidget(manaBar.create("MP", "#4488cc"));
        layout->addStretch();
        layout->addWidget(effectLabel);
        layout->addStretch();
        layout->addWidget(spellLabel);
        panel->setStyleSheet(
            "background: #1a1a2e; border: 1px solid #333355; "
            "border-radius: 6px; padding: 10px;"
        );
        return panel;
    }

    void update(const PlayerInfo &info, const QString &roleLabel, bool hideSpell = false, const EffectInfo &effect = {}) {
        nameLabel->setText(roleLabel);
        playerIdLabel->setText(QString::fromStdString(info.name));
        hpBar.update(info.hp, info.maxHp);
        manaBar.update(info.mana, info.maxMana);
        if (hideSpell || info.spellCast.empty()) {
            spellLabel->setText("Spell: ---");
        } else {
            QString text = "Spell: " + QString::fromStdString(info.spellCast);
            if (!info.spellDescription.empty())
                text += " (" + QString::fromStdString(info.spellDescription) + ")";
            spellLabel->setText(text);
        }
        if (effect.type == EffectType::NONE) {
            effectLabel->setText("");
            effectLabel->setStyleSheet(
                "color: #666666; font-size: 22px; font-weight: bold; "
                "background: #111122; border: 1px solid #222244; border-radius: 6px; padding: 10px;"
            );
        } else {
            switch (effect.type) {
            case EffectType::DAMAGE:
                effectLabel->setStyleSheet(
                    "color: #ff4444; font-size: 22px; font-weight: bold; "
                    "background: #1a0000; border: 1px solid #442222; border-radius: 6px; padding: 10px;"
                );
                break;
            case EffectType::HEAL:
                effectLabel->setStyleSheet(
                    "color: #44ff44; font-size: 22px; font-weight: bold; "
                    "background: #001a00; border: 1px solid #224422; border-radius: 6px; padding: 10px;"
                );
                break;
            case EffectType::SHIELD:
                effectLabel->setStyleSheet(
                    "color: #44aaff; font-size: 22px; font-weight: bold; "
                    "background: #000a1a; border: 1px solid #224466; border-radius: 6px; padding: 10px;"
                );
                break;
            default:
                effectLabel->setStyleSheet(
                    "color: #666666; font-size: 22px; font-weight: bold; "
                    "background: #111122; border: 1px solid #222244; border-radius: 6px; padding: 10px;"
                );
                break;
            }
            effectLabel->setText(QString::fromStdString(effect.text));
        }
    }
};

class SpellMasterUI : public QWidget {
    QStackedWidget *screens;
    GameStatus status;
    QPushButton *readyBtn;
    QLabel *lobbyStatus;
    QLabel *roundLabel;
    QLabel *timerLabel;
    QLabel *phaseLabel;
    PlayerPanel selfPanel;
    PlayerPanel opponentPanel;
    QLabel *messageLabel;
    QLabel *winnerLabel;
    QLabel *resultDetails;
    enum Screen { LOBBY = 0, BATTLE = 1, RESULT = 2 };

public:
    std::function<void()> onReady;

    SpellMasterUI(const QString &windowTitle = "SpellMaster") {
        setFixedSize(800, 480);
        setWindowTitle(windowTitle);
        setStyleSheet("background: #0f0f1a; color: white;");
        screens = new QStackedWidget(this);
        screens->setGeometry(0, 0, 800, 480);
        screens->addWidget(createLobbyScreen());
        screens->addWidget(createBattleScreen());
        screens->addWidget(createResultScreen());
        screens->setCurrentIndex(LOBBY);
    }

    void updateDisplay(const GameStatus &gs) {
        status = gs;
        switch (status.phase) {
        case GamePhase::LOBBY:
            lobbyStatus->setText(QString::fromStdString(status.statusMessage));
            screens->setCurrentIndex(LOBBY);
            break;
        case GamePhase::CASTING:
            [[fallthrough]];
        case GamePhase::GET_READY:
            [[fallthrough]];
        case GamePhase::RESOLVING:
            roundLabel->setText(status.round > 0 ? QString("Round %1").arg(status.round) : "---");
            if (status.phase == GamePhase::GET_READY) {
                phaseLabel->setText("GET READY");
                phaseLabel->setStyleSheet("color: #ffcc00; font-size: 16px; font-weight: bold;");
                timerLabel->setText(QString("Time: %1s").arg(status.castTimeLeft));
                timerLabel->setStyleSheet("color: #ffcc00; font-size: 16px;");
            } else if (status.phase == GamePhase::CASTING) {
                phaseLabel->setText("CASTING");
                phaseLabel->setStyleSheet("color: #44bb44; font-size: 16px; font-weight: bold;");
                if (status.castTimeLeft <= 3) {
                    timerLabel->setText(QString("Time: %1s").arg(status.castTimeLeft));
                    timerLabel->setStyleSheet("color: #ff4444; font-size: 16px; font-weight: bold;");
                } else {
                    timerLabel->setText(QString("Time: %1s").arg(status.castTimeLeft));
                    timerLabel->setStyleSheet("color: #44bb44; font-size: 16px;");
                }
            } else {
                phaseLabel->setText("RESOLVING");
                phaseLabel->setStyleSheet("color: #44aaff; font-size: 16px; font-weight: bold;");
                timerLabel->setText(QString("Time: %1s").arg(status.castTimeLeft));
                timerLabel->setStyleSheet("color: #44aaff; font-size: 16px;");
            }
            selfPanel.update(status.self, "YOU",
                status.phase == GamePhase::GET_READY,
                status.phase == GamePhase::RESOLVING ? status.selfEffect : EffectInfo{});
            opponentPanel.update(status.opponent, "OPPONENT",
                status.phase != GamePhase::RESOLVING,
                status.phase == GamePhase::RESOLVING ? status.opponentEffect : EffectInfo{});
            messageLabel->setText(QString::fromStdString(status.statusMessage));
            screens->setCurrentIndex(BATTLE);
            break;
        case GamePhase::GAME_OVER: {
            bool youWon = (status.winner == status.self.name);
            if (status.winner == "Nobody") {
                winnerLabel->setText("DRAW!");
                winnerLabel->setStyleSheet("color: #ffcc00; font-size: 36px; font-weight: bold;");
            } else if (youWon) {
                winnerLabel->setText("YOU WON!");
                winnerLabel->setStyleSheet("color: #44ff44; font-size: 36px; font-weight: bold;");
            } else {
                winnerLabel->setText("YOU LOST!");
                winnerLabel->setStyleSheet("color: #ff4444; font-size: 36px; font-weight: bold;");
            }
            resultDetails->setText(QString("You: %1 HP  |  Opponent: %2 HP")
                .arg(status.self.hp).arg(status.opponent.hp));
            screens->setCurrentIndex(RESULT);
            break;
        }
        }
    }

private:
    QWidget* createLobbyScreen() {
        auto *screen = new QWidget;
        auto *layout = new QVBoxLayout(screen);
        layout->setAlignment(Qt::AlignCenter);
        auto *title = new QLabel("SpellMaster");
        title->setStyleSheet("color: #ffcc00; font-size: 42px; font-weight: bold;");
        title->setAlignment(Qt::AlignCenter);
        auto *subtitle = new QLabel("A Wizard Duel");
        subtitle->setStyleSheet("color: #888888; font-size: 16px;");
        subtitle->setAlignment(Qt::AlignCenter);
        readyBtn = new QPushButton("READY");
        readyBtn->setFixedSize(200, 60);
        readyBtn->setStyleSheet(
            "QPushButton { background: #2d6a2d; color: white; font-size: 20px; "
            "font-weight: bold; border-radius: 8px; border: 2px solid #44aa44; }"
            "QPushButton:pressed { background: #44aa44; }"
        );
        QObject::connect(readyBtn, &QPushButton::clicked, [this]() { if (onReady) onReady(); });
        lobbyStatus = new QLabel("Press READY to start");
        lobbyStatus->setStyleSheet("color: #aaaaaa; font-size: 14px;");
        lobbyStatus->setAlignment(Qt::AlignCenter);
        auto *btnLayout = new QHBoxLayout;
        btnLayout->setAlignment(Qt::AlignCenter);
        btnLayout->addWidget(readyBtn);
        layout->addStretch();
        layout->addWidget(title);
        layout->addWidget(subtitle);
        layout->addSpacing(30);
        layout->addLayout(btnLayout);
        layout->addSpacing(10);
        layout->addWidget(lobbyStatus);
        layout->addStretch();
        return screen;
    }

    QWidget* createBattleScreen() {
        auto *screen = new QWidget;
        auto *layout = new QVBoxLayout(screen);
        layout->setContentsMargins(15, 10, 15, 10);
        auto *topBar = new QHBoxLayout;
        roundLabel = new QLabel("Round 1");
        roundLabel->setStyleSheet("color: white; font-size: 16px; font-weight: bold;");
        timerLabel = new QLabel("Time: 10s");
        timerLabel->setStyleSheet("color: #ffaa00; font-size: 16px;");
        timerLabel->setAlignment(Qt::AlignCenter);
        phaseLabel = new QLabel("CASTING");
        phaseLabel->setStyleSheet("color: #ffaa00; font-size: 16px; font-weight: bold;");
        phaseLabel->setAlignment(Qt::AlignRight);
        topBar->addWidget(roundLabel);
        topBar->addWidget(timerLabel);
        topBar->addWidget(phaseLabel);
        layout->addLayout(topBar);
        auto *panels = new QHBoxLayout;
        panels->setSpacing(15);
        panels->addWidget(selfPanel.create("YOU"));
        panels->addWidget(opponentPanel.create("OPPONENT"));
        layout->addLayout(panels, 1);
        messageLabel = new QLabel("Cast your spell!");
        messageLabel->setStyleSheet(
            "color: #cccccc; font-size: 14px; background: #16213e; "
            "border-radius: 4px; padding: 8px;"
        );
        messageLabel->setAlignment(Qt::AlignCenter);
        messageLabel->setFixedHeight(40);
        layout->addWidget(messageLabel);
        return screen;
    }

    QWidget* createResultScreen() {
        auto *screen = new QWidget;
        auto *layout = new QVBoxLayout(screen);
        layout->setAlignment(Qt::AlignCenter);
        winnerLabel = new QLabel("");
        winnerLabel->setStyleSheet("color: #ffcc00; font-size: 36px; font-weight: bold;");
        winnerLabel->setAlignment(Qt::AlignCenter);
        resultDetails = new QLabel("");
        resultDetails->setStyleSheet("color: #aaaaaa; font-size: 16px;");
        resultDetails->setAlignment(Qt::AlignCenter);
        auto *hint = new QLabel("Press READY for new game");
        hint->setStyleSheet("color: #555555; font-size: 13px;");
        hint->setAlignment(Qt::AlignCenter);
        layout->addStretch();
        layout->addWidget(winnerLabel);
        layout->addSpacing(15);
        layout->addWidget(resultDetails);
        layout->addSpacing(30);
        layout->addWidget(hint);
        layout->addStretch();
        return screen;
    }
};

// ================================================================
//  TEST CODE — only used with --test flag, never runs on RPi
// ================================================================

class TestSimulator : public QWidget {
    SpellMasterUI *p1Window;
    SpellMasterUI *p2Window;
    int round = 0;
    int castTimeLeft = 10;
    PlayerInfo p1 = {"Player 1", 100, 100, 100, 100, ""};
    PlayerInfo p2 = {"Player 2", 100, 100, 100, 100, ""};
    GamePhase phase = GamePhase::LOBBY;
    std::string winner = "";
    int p1HpBefore = 100;
    int p2HpBefore = 100;
    EffectInfo p1Effect;
    EffectInfo p2Effect;
    bool autoMode = false;
    QTimer *castTimer;
    QLabel *statusLabel;

public:
    TestSimulator() {
        srand(time(nullptr));
        setWindowTitle("SpellMaster Test Controller");
        setFixedSize(320, 420);
        setStyleSheet("background: #1a1a1a; color: white;");
        auto *layout = new QVBoxLayout(this);
        layout->setSpacing(4);
        auto *title = new QLabel("Test Controller");
        title->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffcc00;");
        title->setAlignment(Qt::AlignCenter);
        layout->addWidget(title);
        auto *sep1 = new QFrame;
        sep1->setFrameShape(QFrame::HLine);
        sep1->setStyleSheet("color: #333333;");
        layout->addWidget(sep1);
        auto *spellTitle = new QLabel("Spells:");
        spellTitle->setStyleSheet("color: white; font-size: 13px; font-weight: bold;");
        layout->addWidget(spellTitle);
        auto *grid = new QGridLayout;
        grid->setSpacing(2);
        grid->addWidget(makeLabel("Key", true), 0, 0);
        grid->addWidget(makeLabel("Spell", true), 0, 1);
        grid->addWidget(makeLabel("Effect", true), 0, 2);
        grid->addWidget(makeLabel("Cost", true), 0, 3);
        grid->addWidget(makeLabel("Player 1", true, "#44aaff"), 1, 0);
        addSpellRow(grid, 2, "1", "Fireball",  "20 dmg", "10 mp");
        addSpellRow(grid, 3, "2", "Lightning", "30 dmg", "20 mp");
        addSpellRow(grid, 4, "3", "Shield",    "1/2 dmg",  "15 mp");
        addSpellRow(grid, 5, "4", "Heal",      "+30 hp", "25 mp");
        grid->addWidget(makeLabel("Player 2", true, "#ff8844"), 6, 0);
        addSpellRow(grid, 7, "5", "Fireball",  "20 dmg", "10 mp");
        addSpellRow(grid, 8, "6", "Lightning", "30 dmg", "20 mp");
        addSpellRow(grid, 9, "7", "Shield",    "1/2 dmg",  "15 mp");
        addSpellRow(grid, 10,"8", "Heal",      "+30 hp", "25 mp");
        grid->addWidget(makeLabel("Controls:", true, "white"), 11, 0);
        grid->addWidget(makeLabel("Space", false, "#ffcc00"), 12, 0);
        grid->addWidget(makeLabel("Start / New game"), 12, 1);
        grid->addWidget(makeLabel("R", false, "#ffcc00"), 13, 0);
        grid->addWidget(makeLabel("Resolve round"), 13, 1);
        grid->addWidget(makeLabel("N", false, "#ffcc00"), 14, 0);
        grid->addWidget(makeLabel("Skip phase"), 14, 1);
        grid->addWidget(makeLabel("A", false, "#ffcc00"), 15, 0);
        grid->addWidget(makeLabel("Auto mode"), 15, 1);
        grid->addWidget(makeLabel("0", false, "#ffcc00"), 16, 0);
        grid->addWidget(makeLabel("Reset game"), 16, 1);
        grid->addWidget(makeLabel("Esc", false, "#ffcc00"), 17, 0);
        grid->addWidget(makeLabel("Quit"), 17, 1);
        layout->addLayout(grid);
        statusLabel = new QLabel("Ready");
        statusLabel->setStyleSheet(
            "color: #cccccc; font-size: 11px; background: #222222; border-radius: 3px; padding: 4px;"
        );
        statusLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(statusLabel);
        layout->addStretch();
        auto *version = new QLabel(QString("SpellMaster UI %1").arg(UI_VERSION));
        version->setStyleSheet("color: #444444; font-size: 10px;");
        version->setAlignment(Qt::AlignCenter);
        layout->addWidget(version);
        p1Window = new SpellMasterUI("Player 1");
        p2Window = new SpellMasterUI("Player 2");
        this->move(10, 100);
        p1Window->move(350, 100);
        p2Window->move(1170, 100);
        p1Window->onReady = [this]() { startGame(); };
        p2Window->onReady = [this]() { startGame(); };
        p1Window->show();
        p2Window->show();
        castTimer = new QTimer(this);
        QObject::connect(castTimer, &QTimer::timeout, [this]() { onTimerTick(); });
        setFocusPolicy(Qt::StrongFocus);
        broadcastStatus("Waiting for players...");
    }

    ~TestSimulator() { delete p1Window; delete p2Window; }

protected:
    void keyPressEvent(QKeyEvent *event) override {
        switch (event->key()) {
        case Qt::Key_Space: startGame(); break;
        case Qt::Key_0: resetGame(); break;
        case Qt::Key_1: castSpell(p1, "Fireball", 20, 10, "20 dmg"); break;
        case Qt::Key_2: castSpell(p1, "Lightning", 30, 20, "30 dmg"); break;
        case Qt::Key_3: castSpell(p1, "Shield", 0, 15, "1/2 dmg"); break;
        case Qt::Key_4: castSpell(p1, "Heal", 0, 25, "+30 HP"); break;
        case Qt::Key_5: castSpell(p2, "Fireball", 20, 10, "20 dmg"); break;
        case Qt::Key_6: castSpell(p2, "Lightning", 30, 20, "30 dmg"); break;
        case Qt::Key_7: castSpell(p2, "Shield", 0, 15, "1/2 dmg"); break;
        case Qt::Key_8: castSpell(p2, "Heal", 0, 25, "+30 HP"); break;
        case Qt::Key_R: resolve(); break;
        case Qt::Key_N: skipPhase(); break;
        case Qt::Key_A: toggleAutoMode(); break;
        case Qt::Key_Escape: p1Window->close(); p2Window->close(); close(); break;
        default: break;
        }
    }

private:
    QLabel* makeLabel(const QString &text, bool bold = false, const QString &color = "#aaaaaa") {
        auto *l = new QLabel(text);
        l->setStyleSheet(QString("color: %1; font-size: 11px;%2").arg(color, bold ? " font-weight: bold;" : ""));
        return l;
    }

    void addSpellRow(QGridLayout *grid, int row, const QString &key, const QString &name,
                     const QString &effect, const QString &cost) {
        grid->addWidget(makeLabel(key, false, "#ffcc00"), row, 0);
        grid->addWidget(makeLabel(name), row, 1);
        grid->addWidget(makeLabel(effect), row, 2);
        grid->addWidget(makeLabel(cost), row, 3);
    }

    void setStatus(const std::string &msg) { statusLabel->setText(QString::fromStdString(msg)); }

    void resetGame() {
        castTimer->stop();
        phase = GamePhase::LOBBY;
        round = 0; winner = "";
        p1 = {"Player 1", 100, 100, 100, 100, ""};
        p2 = {"Player 2", 100, 100, 100, 100, ""};
        setStatus("Reset");
        broadcastStatus("Waiting for players...");
    }

    void startGame() {
        if (phase != GamePhase::LOBBY && phase != GamePhase::GAME_OVER) return;
        round = 1;
        p1 = {"Player 1", 100, 100, 100, 100, ""};
        p2 = {"Player 2", 100, 100, 100, 100, ""};
        winner = "";
        p1Effect = {}; p2Effect = {};
        enterGetReady("Get ready!");
    }

    void enterGetReady(const std::string &msg) {
        phase = GamePhase::GET_READY;
        castTimeLeft = 5;
        p1.spellCast = ""; p2.spellCast = "";
        p1.spellDescription = ""; p2.spellDescription = "";
        p1Effect = {}; p2Effect = {};
        setStatus("Round " + std::to_string(round) + " - Get ready");
        broadcastStatus(msg);
        castTimer->start(1000);
    }

    void enterCasting() {
        phase = GamePhase::CASTING;
        castTimeLeft = 10;
        setStatus("Round " + std::to_string(round) + " - Casting");
        broadcastStatus("Cast your spell!");
    }

    void castSpell(PlayerInfo &caster, const std::string &name, int damage, int manaCost, const std::string &desc) {
        if (phase != GamePhase::CASTING) return;
        if (!caster.spellCast.empty()) { setStatus(caster.name + " already cast " + caster.spellCast); return; }
        if (caster.mana < manaCost) { setStatus(caster.name + ": not enough mana!"); return; }
        caster.spellCast = name;
        caster.spellDescription = desc;
        caster.mana -= manaCost;
        bool p1Ready = !p1.spellCast.empty(), p2Ready = !p2.spellCast.empty();
        if (p1Ready && p2Ready) { setStatus("Both cast! Press R"); broadcastStatus("Both spells cast! Press R to resolve."); }
        else if (p1Ready) { setStatus("P1: " + p1.spellCast + " | P2: ..."); broadcastStatus("Waiting for Player 2..."); }
        else { setStatus("P1: ... | P2: " + p2.spellCast); broadcastStatus("Waiting for Player 1..."); }
    }

    int applySpell(const std::string &spell, PlayerInfo &caster, PlayerInfo &target, bool targetShielded) {
        int damage = 0;
        if (spell == "Fireball") damage = 20;
        else if (spell == "Lightning") damage = 30;
        else if (spell == "Heal") { caster.hp += 30; return 0; }
        if (damage > 0 && targetShielded) damage = damage / 2;
        target.hp -= damage;
        return damage;
    }

    void resolve() {
        if (phase != GamePhase::CASTING) return;
        if (p1.spellCast.empty()) p1.spellCast = "Miss";
        if (p2.spellCast.empty()) p2.spellCast = "Miss";
        p1HpBefore = p1.hp; p2HpBefore = p2.hp;
        bool p1Shielded = p1.spellCast == "Shield", p2Shielded = p2.spellCast == "Shield";
        applySpell(p1.spellCast, p1, p2, p2Shielded);
        applySpell(p2.spellCast, p2, p1, p1Shielded);
        p1.hp = std::max(0, std::min(p1.hp, p1.maxHp));
        p2.hp = std::max(0, std::min(p2.hp, p2.maxHp));
        phase = GamePhase::RESOLVING;
        castTimeLeft = 5;

        int p1Delta = p1.hp - p1HpBefore;
        int p2Delta = p2.hp - p2HpBefore;

        if (p1Shielded && p1Delta < 0)
            p1Effect = {EffectType::SHIELD, "Reduced! " + std::to_string(p1Delta) + " HP"};
        else if (p1Shielded && p1Delta == 0)
            p1Effect = {EffectType::SHIELD, "Shielded!"};
        else if (p1Delta < 0)
            p1Effect = {EffectType::DAMAGE, std::to_string(p1Delta) + " HP"};
        else if (p1Delta > 0)
            p1Effect = {EffectType::HEAL, "+" + std::to_string(p1Delta) + " HP"};
        else
            p1Effect = {};

        if (p2Shielded && p2Delta < 0)
            p2Effect = {EffectType::SHIELD, "Reduced! " + std::to_string(p2Delta) + " HP"};
        else if (p2Shielded && p2Delta == 0)
            p2Effect = {EffectType::SHIELD, "Shielded!"};
        else if (p2Delta < 0)
            p2Effect = {EffectType::DAMAGE, std::to_string(p2Delta) + " HP"};
        else if (p2Delta > 0)
            p2Effect = {EffectType::HEAL, "+" + std::to_string(p2Delta) + " HP"};
        else
            p2Effect = {};

        std::string msg = p1.spellCast + " vs " + p2.spellCast;
        setStatus(msg);
        if (p1.hp <= 0 || p2.hp <= 0) {
            if (p1.hp > p2.hp) winner = p1.name;
            else if (p2.hp > p1.hp) winner = p2.name;
            else winner = "Nobody";
        }
        broadcastStatus(msg);
    }

    void nextRound() {
        round++;
        enterGetReady("Next round!");
    }

    void toggleAutoMode() {
        autoMode = !autoMode;
        setStatus(autoMode ? "Auto mode ON" : "Auto mode OFF");
        if (autoMode && (phase == GamePhase::LOBBY || phase == GamePhase::GAME_OVER)) {
            startGame();
        }
    }

    void skipPhase() {
        castTimeLeft = 1;
    }

    void autoCastSpells() {
        if (!autoMode || phase != GamePhase::CASTING) return;
        static const char* spells[] = {"Fireball", "Lightning", "Shield", "Heal"};
        static const int damage[] = {20, 30, 0, 0};
        static const int cost[] = {10, 20, 15, 25};
        static const char* desc[] = {"20 dmg", "30 dmg", "1/2 dmg", "+30 HP"};
        if (p1.spellCast.empty() && castTimeLeft == 8) {
            int i = rand() % 4;
            castSpell(p1, spells[i], damage[i], cost[i], desc[i]);
        }
        if (p2.spellCast.empty() && castTimeLeft == 6) {
            int i = rand() % 4;
            castSpell(p2, spells[i], damage[i], cost[i], desc[i]);
        }
    }

    void onTimerTick() {
        castTimeLeft--;

        if (phase == GamePhase::GET_READY) {
            if (castTimeLeft <= 0) {
                enterCasting();
            } else {
                setStatus("Get ready: " + std::to_string(castTimeLeft) + "s");
                broadcastStatus("Get ready!");
            }
        } else if (phase == GamePhase::CASTING) {
            autoCastSpells();
            if (castTimeLeft <= 0) {
                resolve();
            } else {
                setStatus("Time: " + std::to_string(castTimeLeft) + "s");
                broadcastStatus("Cast your spell!");
            }
        } else if (phase == GamePhase::RESOLVING) {
            if (castTimeLeft <= 0) {
                if (!winner.empty()) {
                    phase = GamePhase::GAME_OVER;
                    broadcastStatus(winner + " wins!");
                    castTimer->stop();
                    if (autoMode) startGame();
                } else {
                    nextRound();
                }
            } else {
                setStatus("Resolving: " + std::to_string(castTimeLeft) + "s");
                broadcastStatus(p1.spellCast + " vs " + p2.spellCast);
            }
        } else {
            castTimer->stop();
        }
    }

    void broadcastStatus(const std::string &msg) {
        std::string p1Msg = msg, p2Msg = msg;
        if (phase == GamePhase::CASTING) {
            p1Msg = p1.spellCast.empty() ? "Cast your spell!" : "Waiting for opponent...";
            p2Msg = p2.spellCast.empty() ? "Cast your spell!" : "Waiting for opponent...";
        }
        GameStatus gs1;
        gs1.phase = phase; gs1.round = round; gs1.castTimeLeft = castTimeLeft;
        gs1.self = p1; gs1.opponent = p2;
        gs1.selfEffect = p1Effect; gs1.opponentEffect = p2Effect;
        gs1.winner = winner; gs1.statusMessage = p1Msg;
        p1Window->updateDisplay(gs1);
        GameStatus gs2;
        gs2.phase = phase; gs2.round = round; gs2.castTimeLeft = castTimeLeft;
        gs2.self = p2; gs2.opponent = p1;
        gs2.selfEffect = p2Effect; gs2.opponentEffect = p1Effect;
        gs2.winner = winner; gs2.statusMessage = p2Msg;
        p2Window->updateDisplay(gs2);
    }
};

// ================================================================
//  MAIN
// ================================================================

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    bool testMode = false;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--test") testMode = true;
    }
    if (testMode) {
        TestSimulator sim;
        sim.show();
        return app.exec();
    }
    SpellMasterUI ui;
    ui.setWindowFlags(Qt::FramelessWindowHint);
    QApplication::setOverrideCursor(Qt::BlankCursor);
    ui.show();
    return app.exec();
}
