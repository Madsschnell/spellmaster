// SpellMasterUI.h
// Player UI with three screens: Lobby, Battle, Result
// Used by both standalone render test and networked player application

#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include "gamestatus.h"
#include <functional>
#include <string>

static const char* UI_VERSION = "v0.2.0";

struct StatBar {
    QProgressBar *bar;
    QLabel *text;

    QWidget* create(const QString &name, const QString &barColor);
    void update(int value, int max);
};

struct PlayerPanel {
    QLabel *nameLabel;
    QLabel *playerIdLabel;
    StatBar hpBar;
    StatBar manaBar;
    QLabel *effectLabel;
    QLabel *spellLabel;

    QWidget* create(const QString &title);
    void update(const PlayerInfo &info, const QString &roleLabel,
                bool hideSpell = false, const EffectInfo &effect = {});
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
    QPushButton *resultReadyBtn;
    QLabel *recognitionLabel;
    QTimer *recognitionTimer;
    bool readySent = false;
    enum Screen { LOBBY = 0, BATTLE = 1, RESULT = 2 };

public:
    std::function<void()> onReady;

    SpellMasterUI(const QString &windowTitle = "SpellMaster");

    void pressReady();
    void showRecognitionResult(bool success, const std::string& message);
    void updateDisplay(const GameStatus &gs);

private:
    void handleReady();
    QWidget* createLobbyScreen();
    QWidget* createBattleScreen();
    QWidget* createResultScreen();
};
