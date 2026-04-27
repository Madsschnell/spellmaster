// Player application: UI + ZeroMQ + ImageProcessing stub
//
// Usage:
//   ./spellmaster_player <player1|player2> <gamemaster-ip> [--windowed] [--auto]
//
// RPi (under Cage):
//   sudo XDG_RUNTIME_DIR=/run/user/1000 cage -- ./spellmaster_player player1 192.168.8.100
//
// Desktop test (3 terminals):
//   ./gamemaster                                          # Terminal 1
//   ./spellmaster_player player1 localhost --windowed      # Terminal 2
//   ./spellmaster_player player2 localhost --windowed      # Terminal 3
//
// Keyboard (ImageProcessing stub):
//   1 = Fireball, 2 = Lightning, 3 = Shield, 4 = Heal
//   5 = Unknown gesture (error test)
//   Space = READY, Esc = Quit

#include "ui/SpellMasterUI.h"
#include "imageprocessing/ImageProcessingStub.h"
#include "gamestatus.h"
#include "playeraction.h"
#include "serialization.h"
#include "transport.h"
#include <QApplication>
#include <QTimer>
#include <QKeyEvent>
#include <QMetaObject>
#include <zmq.hpp>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>

class PlayerApp : public QObject {
    std::string playerId;
    SpellMasterUI *ui;
    ImageProcessingStub imageProc;
    zmq::context_t context;
    zmq::socket_t subscriber;
    zmq::socket_t sender;
    QTimer *pollTimer;

public:
    PlayerApp(const std::string& id, const std::string& serverIp, bool autoMode)
        : playerId(id)
        , imageProc(id, serverIp, autoMode)
        , context(1)
        , subscriber(context, zmq::socket_type::sub)
        , sender(context, zmq::socket_type::push)
    {
        srand(time(nullptr));

        std::string subAddr = "tcp://" + serverIp + ":5555";
        subscriber.connect(subAddr);
        subscriber.set(zmq::sockopt::subscribe, playerId);
        std::cout << "UI SUB connected to " << subAddr << " (topic: " << playerId << ")" << std::endl;

        std::string pushAddr = "tcp://" + serverIp + ":5556";
        sender.connect(pushAddr);
        std::cout << "UI PUSH connected to " << pushAddr << std::endl;

        ui = new SpellMasterUI("SpellMaster - " + QString::fromStdString(playerId));
        ui->onReady = [this]() { sendReady(); };

        imageProc.onRecognitionResult = [this](bool success, const std::string& message) {
            QMetaObject::invokeMethod(ui, [this, success, msg = message]() {
                ui->showRecognitionResult(success, msg);
            }, Qt::QueuedConnection);
        };

        imageProc.start();

        pollTimer = new QTimer(this);
        connect(pollTimer, &QTimer::timeout, this, &PlayerApp::poll);
        pollTimer->start(100);

        ui->installEventFilter(this);
        ui->setFocusPolicy(Qt::StrongFocus);
    }

    SpellMasterUI* getUI() { return ui; }

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (event->type() == QEvent::KeyPress) {
            auto *keyEvent = static_cast<QKeyEvent*>(event);
            switch (keyEvent->key()) {
            case Qt::Key_1: imageProc.onKeyPress(1); return true;
            case Qt::Key_2: imageProc.onKeyPress(2); return true;
            case Qt::Key_3: imageProc.onKeyPress(3); return true;
            case Qt::Key_4: imageProc.onKeyPress(4); return true;
            case Qt::Key_5: imageProc.onKeyPress(5); return true;
            case Qt::Key_Space:
                ui->pressReady();
                return true;
            case Qt::Key_Escape: QApplication::quit(); return true;
            }
        }
        return QObject::eventFilter(obj, event);
    }

private:
    void poll()
    {
        std::string topic;
        GameStatus gs;
        while (zmq_subscribe_recv(subscriber, topic, gs)) {
            ui->updateDisplay(gs);
        }
    }

    void sendReady()
    {
        PlayerAction pa{playerId, "ready"};
        zmq_push(sender, pa);
        std::cout << "READY sent" << std::endl;
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0]
                  << " <player1|player2> <gamemaster-ip> [--windowed] [--auto]"
                  << std::endl;
        std::cerr << "  Keys: 1-4 = cast spell, 5 = error test, Space = ready, Esc = quit"
                  << std::endl;
        return 1;
    }

    std::string playerId = argv[1];
    std::string serverIp = "";
    bool windowed = false;
    bool autoMode = false;

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--windowed") windowed = true;
        else if (arg == "--auto") autoMode = true;
        else if (arg[0] != '-') serverIp = arg;
    }

    if (serverIp.empty()) {
        std::cerr << "Error: Missing GameMaster IP." << std::endl;
        std::cerr << "Usage: " << argv[0]
                  << " <player1|player2> <gamemaster-ip> [--windowed] [--auto]"
                  << std::endl;
        return 1;
    }

    PlayerApp player(playerId, serverIp, autoMode);

    if (!windowed) {
        player.getUI()->setWindowFlags(Qt::FramelessWindowHint);
        QApplication::setOverrideCursor(Qt::BlankCursor);
    }

    player.getUI()->show();
    std::cout << "Player " << playerId << " connected to " << serverIp << std::endl;
    if (autoMode) std::cout << "Auto-cast enabled" << std::endl;

    return app.exec();
}
