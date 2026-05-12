// SpellMasterUI.h
// Player UI with three screens: Lobby, Battle, Result.
// Used by both standalone render test and networked player application.

#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include "GameStatus.h"
#include <functional>
#include <string>

static const char* UI_VERSION = "v0.4.0";

/// HP or Mana bar with label and numeric text
struct StatBar {
    QProgressBar *bar;
    QLabel *text;

    /// Create the bar widget with a colored progress indicator
    /// @param name Label text (e.g. "HP", "MP")
    /// @param barColor CSS color for the progress chunk
    /// @return Owning QWidget containing the bar layout
    QWidget* create(const QString &name, const QString &barColor);

    /// Update bar value and numeric display
    /// @param value Current value
    /// @param max Maximum value (sets bar range)
    void update(int value, int max);
};

/// One player's info panel (name, HP, mana, effect, spell)
struct PlayerPanel {
    QLabel *nameLabel;
    StatBar hpBar;
    StatBar manaBar;
    QLabel *effectLabel;
    QLabel *spellLabel;

    /// Create the panel widget with all sub-elements
    /// @param title Panel header (e.g. "YOU", "OPPONENT")
    /// @return Owning QWidget containing the panel layout
    QWidget* create(const QString &title);

    /// Update all panel elements from game state
    /// @param info Player stats from GameStatus
    /// @param roleLabel Display label ("YOU" or "OPPONENT")
    /// @param hideSpell If true, show "---" instead of spell name
    /// @param effect Effect info for coloring and text
    void update(const PlayerInfo &info, const QString &roleLabel,
                bool hideSpell = false, const EffectInfo &effect = {});

    /// Set effect label color and background
    void setEffectStyle(const QString &color, const QString &bg);
};

/// Main game UI with lobby, battle, and result screens.
/// Renders at 800x480 for the RPi touchscreen.
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
    QWidget *connectionOverlay;
    QLabel *connectionLabel;
    QLabel *playerDebugLabel;
    QTimer *pulseTimer;
    bool pulseBright = false;
    bool readySent = false;
    GamePhase prevPhase = GamePhase::LOBBY;
    enum Screen { LOBBY = 0, BATTLE = 1, RESULT = 2 };

public:
    /// Callback invoked when player presses READY (lobby or result screen)
    std::function<void()> onReady;

    /// @param windowTitle Window title (includes player ID in networked mode)
    SpellMasterUI(const QString &windowTitle = "SpellMaster");

    /// Trigger READY action programmatically (used by keyboard Space key)
    void pressReady();

    /// Show gesture recognition feedback temporarily
    /// @param success true for green text (spell recognized), false for red (error)
    /// @param message Text to display (e.g. "Spell: Fireball" or "Unknown gesture")
    void showRecognitionResult(bool success, const std::string& message);

    /// Update all UI elements from GameStatus received via ZeroMQ
    /// @param gs Current game status for this player
    void updateDisplay(const GameStatus &gs);

    /// Show or hide the connection lost overlay
    /// Resets readySent when connection is lost so READY works on reconnect
    /// @param lost true to show overlay, false to hide
    void setConnectionLost(bool lost);

private:
    void handleReady();
    QWidget* createLobbyScreen();
    QWidget* createBattleScreen();
    QWidget* createResultScreen();
};
