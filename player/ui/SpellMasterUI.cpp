#include "SpellMasterUI.h"
#include <QApplication>

// StatBar

QWidget* StatBar::create(const QString &name, const QString &barColor)
{
    auto *container = new QWidget;
    container->setFixedHeight(36);
    container->setStyleSheet("background: #111122; border-radius: 4px; border: none;");
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(8, 0, 8, 0);
    layout->setSpacing(6);

    auto *label = new QLabel(name);
    label->setFixedWidth(36);
    label->setStyleSheet("color: #cccccc; font-size: 18px; font-weight: bold; background: transparent; border: none;");
    label->setAlignment(Qt::AlignVCenter);

    bar = new QProgressBar;
    bar->setRange(0, 100);
    bar->setValue(100);
    bar->setTextVisible(false);
    bar->setFixedHeight(18);
    bar->setStyleSheet(
        "QProgressBar { background: #333333; border: none; border-radius: 3px; }"
        "QProgressBar::chunk { background: " + barColor + "; border-radius: 2px; }"
    );

    text = new QLabel("100/100");
    text->setFixedWidth(70);
    text->setStyleSheet("color: #cccccc; font-size: 18px; font-weight: bold; background: transparent; border: none;");
    text->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    layout->addWidget(label);
    layout->addWidget(bar);
    layout->addWidget(text);
    return container;
}

void StatBar::update(int value, int max)
{
    bar->setRange(0, max);
    bar->setValue(value);
    text->setText(QString("%1/%2").arg(value).arg(max));
}

void PlayerPanel::setEffectStyle(const QString &color, const QString &bg)
{
    effectLabel->setStyleSheet(
        QString("color: %1; font-size: 42px; font-weight: bold; "
                "background: %2; border-radius: 4px; border: none;").arg(color, bg));
}

// PlayerPanel
// All elements get matching boxes (background: #111122, border-radius: 4px)
// Panel is the outer frame (background: #1a1a2e, border: 1px solid #333355)

QWidget* PlayerPanel::create(const QString &title)
{
    auto *panel = new QWidget;
    auto *layout = new QVBoxLayout(panel);
    layout->setSpacing(3);
    layout->setContentsMargins(4, 4, 4, 4);

    nameLabel = new QLabel(title);
    nameLabel->setStyleSheet(
        "color: white; font-size: 32px; font-weight: bold; "
        "background: #111122; border-radius: 4px; border: none;"
    );
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setFixedHeight(38);

    effectLabel = new QLabel("");
    setEffectStyle("#666666", "#111122");
    effectLabel->setAlignment(Qt::AlignCenter);
    effectLabel->setFixedHeight(75);

    spellLabel = new QLabel("Spell: ---");
    spellLabel->setStyleSheet(
        "color: #aaaaaa; font-size: 24px; font-weight: bold; "
        "background: #111122; border-radius: 4px; border: none;"
    );
    spellLabel->setAlignment(Qt::AlignCenter);
    spellLabel->setFixedHeight(75);

    layout->addWidget(nameLabel);
    layout->addWidget(hpBar.create("HP", "#44bb44"));
    layout->addWidget(manaBar.create("MP", "#4488cc"));
    layout->addWidget(effectLabel);
    layout->addWidget(spellLabel);

    panel->setStyleSheet(
        "background: #1a1a2e; border: 1px solid #333355; "
        "border-radius: 6px;"
    );

    return panel;
}

void PlayerPanel::update(const PlayerInfo &info, const QString &roleLabel,
                         bool hideSpell, const EffectInfo &effect)
{
    nameLabel->setText(roleLabel);
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
        setEffectStyle("#666666", "#111122");
    } else {
        switch (effect.type) {
        case EffectType::DAMAGE:
            setEffectStyle("#ff4444", "#1a0000");
            break;
        case EffectType::HEAL:
            setEffectStyle("#44ff44", "#001a00");
            break;
        case EffectType::SHIELD:
            setEffectStyle("#44aaff", "#000a1a");
            break;
        default:
            setEffectStyle("#666666", "#111122");
            break;
        }
        effectLabel->setText(QString::fromStdString(effect.text));
    }
}

// SpellMasterUI

SpellMasterUI::SpellMasterUI(const QString &windowTitle)
{
    setFixedSize(800, 480);
    setWindowTitle(windowTitle);
    setStyleSheet("background: #0f0f1a; color: white;");

    screens = new QStackedWidget(this);
    screens->setGeometry(0, 0, 800, 480);
    screens->addWidget(createLobbyScreen());
    screens->addWidget(createBattleScreen());
    screens->addWidget(createResultScreen());
    screens->setCurrentIndex(LOBBY);

    auto *versionLabel = new QLabel(UI_VERSION, this);
    versionLabel->setStyleSheet("color: #333333; font-size: 9px;");
    versionLabel->setGeometry(5, 2, 50, 12);
    versionLabel->setAlignment(Qt::AlignLeft);
    versionLabel->raise();

    playerDebugLabel = new QLabel("", this);
    playerDebugLabel->setStyleSheet("color: #333333; font-size: 9px;");
    playerDebugLabel->setGeometry(55, 2, 120, 12);
    playerDebugLabel->setAlignment(Qt::AlignLeft);
    playerDebugLabel->raise();

    connectionOverlay = new QWidget(this);
    connectionOverlay->setGeometry(0, 0, 800, 480);
    connectionOverlay->setStyleSheet("background: rgba(0, 0, 0, 180);");
    connectionLabel = new QLabel("Connection lost", connectionOverlay);
    connectionLabel->setStyleSheet(
        "color: #ff4444; font-size: 28px; font-weight: bold;"
    );
    connectionLabel->setAlignment(Qt::AlignCenter);
    connectionLabel->setGeometry(0, 200, 800, 80);
    connectionOverlay->hide();
    connectionOverlay->raise();

    recognitionTimer = new QTimer(this);
    recognitionTimer->setSingleShot(true);
    QObject::connect(recognitionTimer, &QTimer::timeout, [this]() {
        recognitionLabel->setText("");
        recognitionLabel->setStyleSheet("font-size: 28px;");
    });

    pulseTimer = new QTimer(this);
    QObject::connect(pulseTimer, &QTimer::timeout, [this]() {
        pulseBright = !pulseBright;
        if (pulseBright) {
            messageLabel->setStyleSheet(
                "color: #ffffff; font-size: 28px; font-weight: bold; background: #1a3a6e; "
                "border-radius: 4px; padding: 4px;"
            );
        } else {
            messageLabel->setStyleSheet(
                "color: #cccccc; font-size: 28px; font-weight: bold; background: #16213e; "
                "border-radius: 4px; padding: 4px;"
            );
        }
    });
}

void SpellMasterUI::pressReady()
{
    handleReady();
}

void SpellMasterUI::showRecognitionResult(bool success, const std::string& message)
{
    if (success) {
        recognitionLabel->setText(QString::fromStdString(message));
        recognitionLabel->setStyleSheet(
            "color: #44bb44; font-size: 28px; font-weight: bold;"
        );
    } else {
        recognitionLabel->setText(QString::fromStdString(message));
        recognitionLabel->setStyleSheet(
            "color: #ff4444; font-size: 28px; font-weight: bold;"
        );
    }
    recognitionTimer->start(2000);
}

void SpellMasterUI::setConnectionLost(bool lost)
{
    if (lost) {
        readySent = false;
        connectionOverlay->raise();
        connectionOverlay->show();
    } else {
        connectionOverlay->hide();
    }
}

void SpellMasterUI::updateDisplay(const GameStatus &gs)
{
    status = gs;

    if (!status.self.name.empty())
        playerDebugLabel->setText(QString::fromStdString(status.self.name));

    switch (status.phase) {
    case GamePhase::LOBBY:
        if (prevPhase != GamePhase::LOBBY)
            readySent = false;
        if (!readySent) {
            readyBtn->setEnabled(true);
            readyBtn->setText("READY");
            lobbyStatus->setText(QString::fromStdString(status.statusMessage));
        }
        screens->setCurrentIndex(LOBBY);
        break;

    case GamePhase::GET_READY:
        readySent = false;
        [[fallthrough]];
    case GamePhase::CASTING:
        [[fallthrough]];
    case GamePhase::RESOLVING:
        roundLabel->setText(status.round > 0 ? QString("Round %1").arg(status.round) : "---");

        if (status.phase == GamePhase::GET_READY) {
            phaseLabel->setText("GET READY");
            phaseLabel->setStyleSheet("color: #ffcc00; font-size: 24px; font-weight: bold;");
            timerLabel->setText(QString("Time: %1s").arg(status.castTimeLeft));
            timerLabel->setStyleSheet("color: #ffcc00; font-size: 32px; font-weight: bold;");
        } else if (status.phase == GamePhase::CASTING) {
            phaseLabel->setText("CASTING");
            phaseLabel->setStyleSheet("color: #44bb44; font-size: 24px; font-weight: bold;");
            if (status.castTimeLeft <= 3) {
                timerLabel->setText(QString("Time: %1s").arg(status.castTimeLeft));
                timerLabel->setStyleSheet("color: #ff4444; font-size: 32px; font-weight: bold;");
            } else {
                timerLabel->setText(QString("Time: %1s").arg(status.castTimeLeft));
                timerLabel->setStyleSheet("color: #44bb44; font-size: 32px; font-weight: bold;");
            }
        } else {
            phaseLabel->setText("RESOLVING");
            phaseLabel->setStyleSheet("color: #44aaff; font-size: 24px; font-weight: bold;");
            timerLabel->setText(QString("Time: %1s").arg(status.castTimeLeft));
            timerLabel->setStyleSheet("color: #44aaff; font-size: 32px; font-weight: bold;");
        }

        selfPanel.update(status.self, "YOU",
            status.phase == GamePhase::GET_READY,
            status.phase == GamePhase::RESOLVING ? status.selfEffect : EffectInfo{});
        opponentPanel.update(status.opponent, "OPPONENT",
            status.phase != GamePhase::RESOLVING,
            status.phase == GamePhase::RESOLVING ? status.opponentEffect : EffectInfo{});
        messageLabel->setText(QString::fromStdString(status.statusMessage));

        if (status.phase == GamePhase::CASTING && !status.spellRejected.empty())
            showRecognitionResult(false, status.spellRejected);

        screens->setCurrentIndex(BATTLE);
        break;

    case GamePhase::GAME_OVER:
        if (!readySent) {
            resultReadyBtn->setEnabled(true);
            resultReadyBtn->setText("PLAY AGAIN");
        }
        if (status.winner == "Nobody") {
            winnerLabel->setText("DRAW!");
            winnerLabel->setStyleSheet("color: #ffcc00; font-size: 56px; font-weight: bold;");
        } else if (status.winner == status.self.name) {
            winnerLabel->setText("YOU WON!");
            winnerLabel->setStyleSheet("color: #44ff44; font-size: 56px; font-weight: bold;");
        } else {
            winnerLabel->setText("YOU LOST!");
            winnerLabel->setStyleSheet("color: #ff4444; font-size: 56px; font-weight: bold;");
        }
        resultDetails->setText(QString("You: %1 HP  |  Opponent: %2 HP")
            .arg(status.self.hp).arg(status.opponent.hp));
        screens->setCurrentIndex(RESULT);
        break;
    }

    prevPhase = status.phase;

    if (status.phase == GamePhase::CASTING &&
        status.statusMessage.find("Waiting") == std::string::npos) {
        if (!pulseTimer->isActive())
            pulseTimer->start(500);
    } else {
        if (pulseTimer->isActive()) {
            pulseTimer->stop();
            messageLabel->setStyleSheet(
                "color: #cccccc; font-size: 28px; font-weight: bold; background: #16213e; "
                "border-radius: 4px; padding: 4px;"
            );
        }
    }
}

void SpellMasterUI::handleReady()
{
    if (readySent) return;
    readySent = true;
    readyBtn->setEnabled(false);
    readyBtn->setText("WAITING...");
    lobbyStatus->setText("Waiting for opponent...");
    resultReadyBtn->setEnabled(false);
    resultReadyBtn->setText("WAITING...");
    if (onReady) onReady();
}

QWidget* SpellMasterUI::createLobbyScreen()
{
    auto *screen = new QWidget;
    auto *layout = new QVBoxLayout(screen);
    layout->setAlignment(Qt::AlignCenter);

    auto *title = new QLabel("SpellMaster");
    title->setStyleSheet("color: #ffcc00; font-size: 52px; font-weight: bold;");
    title->setAlignment(Qt::AlignCenter);

    auto *subtitle = new QLabel("A Wizard Duel");
    subtitle->setStyleSheet("color: #888888; font-size: 20px;");
    subtitle->setAlignment(Qt::AlignCenter);

    readyBtn = new QPushButton("READY");
    readyBtn->setFixedSize(280, 70);
    readyBtn->setStyleSheet(
        "QPushButton { background: #2d6a2d; color: white; font-size: 26px; "
        "font-weight: bold; border-radius: 8px; border: 2px solid #44aa44; }"
        "QPushButton:pressed { background: #44aa44; }"
        "QPushButton:disabled { background: #333333; border: 2px solid #555555; color: #888888; }"
    );
    QObject::connect(readyBtn, &QPushButton::clicked, [this]() { handleReady(); });

    lobbyStatus = new QLabel("Press READY to start");
    lobbyStatus->setStyleSheet("color: #aaaaaa; font-size: 18px;");
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

QWidget* SpellMasterUI::createBattleScreen()
{
    auto *screen = new QWidget;
    auto *layout = new QVBoxLayout(screen);
    layout->setContentsMargins(15, 8, 15, 8);
    layout->setSpacing(4);

    auto *topBar = new QHBoxLayout;
    roundLabel = new QLabel("Round 1");
    roundLabel->setStyleSheet("color: white; font-size: 24px; font-weight: bold;");
    roundLabel->setFixedHeight(34);
    timerLabel = new QLabel("Time: 10s");
    timerLabel->setStyleSheet("color: #ffaa00; font-size: 32px; font-weight: bold;");
    timerLabel->setAlignment(Qt::AlignCenter);
    timerLabel->setFixedHeight(34);
    phaseLabel = new QLabel("CASTING");
    phaseLabel->setStyleSheet("color: #ffaa00; font-size: 24px; font-weight: bold;");
    phaseLabel->setAlignment(Qt::AlignRight);
    phaseLabel->setFixedHeight(34);
    topBar->addWidget(roundLabel);
    topBar->addWidget(timerLabel);
    topBar->addWidget(phaseLabel);
    layout->addLayout(topBar);

    auto *panels = new QHBoxLayout;
    panels->setSpacing(15);
    panels->addWidget(selfPanel.create("YOU"));
    panels->addWidget(opponentPanel.create("OPPONENT"));
    layout->addLayout(panels);

    recognitionLabel = new QLabel("");
    recognitionLabel->setStyleSheet("font-size: 28px;");
    recognitionLabel->setAlignment(Qt::AlignCenter);
    recognitionLabel->setFixedHeight(40);
    layout->addWidget(recognitionLabel);

    messageLabel = new QLabel("Cast your spell!");
    messageLabel->setStyleSheet(
        "color: #cccccc; font-size: 28px; font-weight: bold; background: #16213e; "
        "border-radius: 4px; padding: 4px;"
    );
    messageLabel->setAlignment(Qt::AlignCenter);
    messageLabel->setFixedHeight(48);
    layout->addWidget(messageLabel);

    return screen;
}

QWidget* SpellMasterUI::createResultScreen()
{
    auto *screen = new QWidget;
    auto *layout = new QVBoxLayout(screen);
    layout->setAlignment(Qt::AlignCenter);

    winnerLabel = new QLabel("");
    winnerLabel->setStyleSheet("color: #ffcc00; font-size: 56px; font-weight: bold;");
    winnerLabel->setAlignment(Qt::AlignCenter);

    resultDetails = new QLabel("");
    resultDetails->setStyleSheet("color: #aaaaaa; font-size: 24px;");
    resultDetails->setAlignment(Qt::AlignCenter);

    resultReadyBtn = new QPushButton("PLAY AGAIN");
    resultReadyBtn->setFixedSize(280, 70);
    resultReadyBtn->setStyleSheet(
        "QPushButton { background: #2d6a2d; color: white; font-size: 26px; "
        "font-weight: bold; border-radius: 8px; border: 2px solid #44aa44; }"
        "QPushButton:pressed { background: #44aa44; }"
        "QPushButton:disabled { background: #333333; border: 2px solid #555555; color: #888888; }"
    );
    QObject::connect(resultReadyBtn, &QPushButton::clicked, [this]() { handleReady(); });

    auto *btnLayout = new QHBoxLayout;
    btnLayout->setAlignment(Qt::AlignCenter);
    btnLayout->addWidget(resultReadyBtn);

    layout->addStretch();
    layout->addWidget(winnerLabel);
    layout->addSpacing(15);
    layout->addWidget(resultDetails);
    layout->addSpacing(30);
    layout->addLayout(btnLayout);
    layout->addStretch();
    return screen;
}
