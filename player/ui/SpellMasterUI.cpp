#include "SpellMasterUI.h"
#include <QApplication>

// StatBar

QWidget* StatBar::create(const QString &name, const QString &barColor)
{
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

void StatBar::update(int value, int max)
{
    bar->setRange(0, max);
    bar->setValue(value);
    text->setText(QString("%1/%2").arg(value).arg(max));
}

// PlayerPanel

QWidget* PlayerPanel::create(const QString &title)
{
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

void PlayerPanel::update(const PlayerInfo &info, const QString &roleLabel,
                         bool hideSpell, const EffectInfo &effect)
{
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
                "background: #1a0000; border: 1px solid #442222; border-radius: 6px; padding: 10px;");
            break;
        case EffectType::HEAL:
            effectLabel->setStyleSheet(
                "color: #44ff44; font-size: 22px; font-weight: bold; "
                "background: #001a00; border: 1px solid #224422; border-radius: 6px; padding: 10px;");
            break;
        case EffectType::SHIELD:
            effectLabel->setStyleSheet(
                "color: #44aaff; font-size: 22px; font-weight: bold; "
                "background: #000a1a; border: 1px solid #224466; border-radius: 6px; padding: 10px;");
            break;
        default:
            effectLabel->setStyleSheet(
                "color: #666666; font-size: 22px; font-weight: bold; "
                "background: #111122; border: 1px solid #222244; border-radius: 6px; padding: 10px;");
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
    versionLabel->setStyleSheet("color: #444444; font-size: 10px;");
    versionLabel->setGeometry(700, 460, 90, 15);
    versionLabel->setAlignment(Qt::AlignRight);
    versionLabel->raise();

    recognitionTimer = new QTimer(this);
    recognitionTimer->setSingleShot(true);
    QObject::connect(recognitionTimer, &QTimer::timeout, [this]() {
        recognitionLabel->setText("");
        recognitionLabel->setStyleSheet("font-size: 11px;");
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
        recognitionLabel->setStyleSheet("color: #44bb44; font-size: 11px;");
    } else {
        recognitionLabel->setText(QString::fromStdString(message));
        recognitionLabel->setStyleSheet("color: #ff4444; font-size: 11px;");
    }
    recognitionTimer->start(2000);
}

void SpellMasterUI::updateDisplay(const GameStatus &gs)
{
    status = gs;

    switch (status.phase) {
    case GamePhase::LOBBY:
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
            winnerLabel->setStyleSheet("color: #ffcc00; font-size: 36px; font-weight: bold;");
        } else if (status.winner == status.self.name) {
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
        "QPushButton:disabled { background: #333333; border: 2px solid #555555; color: #888888; }"
    );
    QObject::connect(readyBtn, &QPushButton::clicked, [this]() { handleReady(); });

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

QWidget* SpellMasterUI::createBattleScreen()
{
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

    recognitionLabel = new QLabel("");
    recognitionLabel->setStyleSheet("font-size: 11px;");
    recognitionLabel->setAlignment(Qt::AlignRight);
    recognitionLabel->setFixedHeight(20);
    layout->addWidget(recognitionLabel);

    return screen;
}

QWidget* SpellMasterUI::createResultScreen()
{
    auto *screen = new QWidget;
    auto *layout = new QVBoxLayout(screen);
    layout->setAlignment(Qt::AlignCenter);

    winnerLabel = new QLabel("");
    winnerLabel->setStyleSheet("color: #ffcc00; font-size: 36px; font-weight: bold;");
    winnerLabel->setAlignment(Qt::AlignCenter);

    resultDetails = new QLabel("");
    resultDetails->setStyleSheet("color: #aaaaaa; font-size: 16px;");
    resultDetails->setAlignment(Qt::AlignCenter);

    resultReadyBtn = new QPushButton("PLAY AGAIN");
    resultReadyBtn->setFixedSize(200, 60);
    resultReadyBtn->setStyleSheet(
        "QPushButton { background: #2d6a2d; color: white; font-size: 20px; "
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
