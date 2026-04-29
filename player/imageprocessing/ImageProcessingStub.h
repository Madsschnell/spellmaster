// ImageProcessingStub.h
// Keyboard-based stub for testing without camera
//
// Runs in its own thread with own ZeroMQ sockets (SUB + PUSH).
// Communicates with UI via onRecognitionResult callback (QueuedConnection).
//
// The real ImageProcessing will have the same threading model:
//   - Own thread for camera capture and pattern recognition
//   - Own ZeroMQ sockets (SUB for GameStatus, PUSH for spells)
//   - onRecognitionResult callback marshalled to Qt thread

#pragma once

#include "GameStatus.h"
#include "SpellMessage.h"
#include "Serialization.h"
#include "Transport.h"
#include <zmq.hpp>
#include <iostream>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <cstdlib>

class ImageProcessingStub {
    std::string playerId;
    bool autoMode;
    std::atomic<bool> spellCastThisRound{false};
    std::atomic<GamePhase> currentPhase{GamePhase::LOBBY};
    std::atomic<int> pendingKeyPress{0};
    std::atomic<bool> running{true};

    std::string serverIp;
    std::thread workerThread;

public:
    std::function<void(bool success, const std::string& message)> onRecognitionResult;

    ImageProcessingStub(const std::string& id, const std::string& ip, bool autocast)
        : playerId(id), autoMode(autocast), serverIp(ip) {}

    ~ImageProcessingStub()
    {
        running = false;
        if (workerThread.joinable()) workerThread.join();
    }

    void start()
    {
        workerThread = std::thread(&ImageProcessingStub::run, this);
    }

    void onKeyPress(int key)
    {
        pendingKeyPress.store(key);
    }

private:
    void run()
    {
        zmq::context_t context(1);

        zmq::socket_t subscriber(context, zmq::socket_type::sub);
        std::string subAddr = "tcp://" + serverIp + ":5555";
        subscriber.connect(subAddr);
        subscriber.set(zmq::sockopt::subscribe, playerId);
        std::cout << "ImageProc SUB connected to " << subAddr << std::endl;

        zmq::socket_t sender(context, zmq::socket_type::push);
        std::string pushAddr = "tcp://" + serverIp + ":5556";
        sender.connect(pushAddr);
        std::cout << "ImageProc PUSH connected to " << pushAddr << std::endl;

        while (running) {
            std::string topic;
            GameStatus gs;
            while (zmq_subscribe_recv(subscriber, topic, gs)) {
                GamePhase prevPhase = currentPhase.load();
                currentPhase.store(gs.phase);

                if (gs.phase == GamePhase::CASTING && prevPhase != GamePhase::CASTING) {
                    spellCastThisRound.store(false);
                }

                if (autoMode && gs.phase == GamePhase::CASTING && !spellCastThisRound.load()) {
                    if (gs.castTimeLeft <= 7) {
                        static const char* spells[] = {"Fireball", "Lightning", "Shield", "Heal"};
                        std::string spellName = spells[rand() % 4];
                        castSpell(sender, spellName);
                        if (onRecognitionResult) onRecognitionResult(true, "Auto-cast: " + spellName);
                    }
                }
            }

            int key = pendingKeyPress.exchange(0);
            if (key != 0) {
                processKeyPress(sender, key);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    void processKeyPress(zmq::socket_t& sender, int key)
    {
        if (currentPhase.load() != GamePhase::CASTING) return;

        std::string spellName;
        switch (key) {
        case 1: spellName = "Fireball"; break;
        case 2: spellName = "Lightning"; break;
        case 3: spellName = "Shield"; break;
        case 4: spellName = "Heal"; break;
        case 5:
            if (onRecognitionResult) onRecognitionResult(false, "Unknown gesture");
            return;
        default: return;
        }

        if (spellCastThisRound.load()) {
            if (onRecognitionResult) onRecognitionResult(false, "Already cast this round");
            return;
        }

        castSpell(sender, spellName);
        if (onRecognitionResult) onRecognitionResult(true, "Spell recognized: " + spellName);
    }

    void castSpell(zmq::socket_t& sender, const std::string& spellName)
    {
        SpellMessage sm{playerId, spellName};
        zmq_push(sender, sm);
        spellCastThisRound.store(true);
        std::cout << "Cast: " << spellName << std::endl;
    }
};
