// Player application: UI + ZeroMQ + ImageProcessing
//
// Usage:
//   ./spellmaster_player <player1|player2> <gamemaster-ip> [options]
//
// RPi production (under Cage):
//   sudo XDG_RUNTIME_DIR=/run/user/1000 cage -- ./spellmaster_player player1 192.168.8.100
//
// RPi with debug stream (viewable at http://<pi-ip>:8080):
//   ./spellmaster_player player1 192.168.8.100 --debug
//
// Desktop test (3 terminals, keyboard):
//   ./gamemaster                                              # Terminal 1
//   ./spellmaster_player player1 localhost --stub --windowed   # Terminal 2
//   ./spellmaster_player player2 localhost --stub --windowed   # Terminal 3
//
// Options:
//   --stub       Use keyboard input instead of camera (for desktop testing)
//   --auto       Auto-cast random spells (stub mode only)
//   --windowed   Run in a window instead of fullscreen kiosk
//   --debug      Enable MJPEG debug stream of camera feed on port 8080
//
// Keyboard (stub mode):
//   1 = Fireball, 2 = Lightning, 3 = Shield, 4 = Heal
//   5 = Unknown gesture (error test)
//   Space = READY, Esc = Quit

#include "ui/SpellMasterUI.h"
#include "imageprocessing/ImageProcessingStub.h"
#include "imageprocessing/ImageProcessing.h"
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
#include <memory>
#include <functional>

class PlayerApp : public QObject {
    std::string playerId;
    SpellMasterUI *ui;

    // ImageProcessing: either camera (default) or stub (keyboard)
    std::unique_ptr<ImageProcessingStub> stub;
    std::unique_ptr<ImageProcessing> camera;
    bool useStub;

    zmq::context_t context;
    zmq::socket_t subscriber;
    zmq::socket_t sender;
    QTimer *pollTimer;

    void setupRecognitionCallback(std::function<void(bool, const std::string&)>& callback)
    {
        callback = [this](bool success, const std::string& message) {
            QMetaObject::invokeMethod(ui, [this, success, msg = message]() {
                ui->showRecognitionResult(success, msg);
            }, Qt::QueuedConnection);
        };
    }

public:
    PlayerApp(const std::string& id, const std::string& serverIp,
              bool stubMode, bool autoMode, bool debugStream)
        : playerId(id)
        , useStub(stubMode)
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

        // Start ImageProcessing
        if (!useStub) {
            camera = std::make_unique<ImageProcessing>(id, serverIp);
            camera->setDebugStream(debugStream);
            setupRecognitionCallback(camera->onRecognitionResult);
            camera->start();
            std::cout << "ImageProcessing: camera mode" << std::endl;
            if (debugStream)
                std::cout << "Debug stream: http://localhost:8080" << std::endl;
        }

        if (useStub) {
            stub = std::make_unique<ImageProcessingStub>(id, serverIp, autoMode);
            setupRecognitionCallback(stub->onRecognitionResult);
            stub->start();
            std::cout << "ImageProcessing: keyboard stub" << std::endl;
        }

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
            case Qt::Key_1: if (stub) stub->onKeyPress(1); return true;
            case Qt::Key_2: if (stub) stub->onKeyPress(2); return true;
            case Qt::Key_3: if (stub) stub->onKeyPress(3); return true;
            case Qt::Key_4: if (stub) stub->onKeyPress(4); return true;
            case Qt::Key_5: if (stub) stub->onKeyPress(5); return true;
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
                  << " <player1|player2> <gamemaster-ip> [--stub] [--windowed] [--auto] [--debug]"
                  << std::endl;
        std::cerr << "  --stub     Keyboard input instead of camera (desktop testing)"
                  << std::endl;
        std::cerr << "  --auto     Auto-cast random spells (stub mode only)"
                  << std::endl;
        std::cerr << "  --windowed Run in window instead of fullscreen"
                  << std::endl;
        std::cerr << "  --debug    Enable MJPEG debug stream on port 8080"
                  << std::endl;
        std::cerr << "  Keys (stub): 1-4 = cast spell, 5 = error test, Space = ready, Esc = quit"
                  << std::endl;
        return 1;
    }

    std::string playerId = argv[1];
    std::string serverIp = "";
    bool windowed = false;
    bool autoMode = false;
    bool stubMode = false;
    bool debugStream = false;

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--windowed") windowed = true;
        else if (arg == "--auto") { autoMode = true; stubMode = true; }
        else if (arg == "--stub") stubMode = true;
        else if (arg == "--debug") debugStream = true;
        else if (arg[0] != '-') serverIp = arg;
    }

    if (serverIp.empty()) {
        std::cerr << "Error: Missing GameMaster IP." << std::endl;
        std::cerr << "Usage: " << argv[0]
                  << " <player1|player2> <gamemaster-ip> [--stub] [--windowed] [--auto] [--debug]"
                  << std::endl;
        return 1;
    }

    PlayerApp player(playerId, serverIp, stubMode, autoMode, debugStream);

    if (!windowed) {
        player.getUI()->setWindowFlags(Qt::FramelessWindowHint);
        QApplication::setOverrideCursor(Qt::BlankCursor);
    }

    player.getUI()->show();
    std::cout << "Player " << playerId << " connected to " << serverIp << std::endl;

    return app.exec();
}
