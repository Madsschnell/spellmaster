// ImageProcessing.h
// Camera-based gesture recognition using rpicam-vid and OpenCV.
//
// Runs in its own thread with own ZeroMQ sockets (SUB + PUSH).
// Captures IR wand input, accumulates drawing path, classifies gesture,
// and sends SpellMessage to GameMaster when a valid spell is recognized.
// Communicates with UI via onRecognitionResult callback (QueuedConnection).
//
// Based on IR detection and gesture tracking from Test3 branch,
// adapted to SpellMaster's threaded component architecture.

#pragma once

#include "Spells.h"
#include "GestureClassifier.h"
#include "GameStatus.h"
#include "SpellMessage.h"
#include "Serialization.h"
#include "Transport.h"
#include <opencv2/opencv.hpp>
#include <zmq.hpp>
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>

class ImageProcessing {
    // Camera settings
    static constexpr int CAP_W = 640;
    static constexpr int CAP_H = 480;
    static constexpr int FRAMERATE = 30;

    // IR detection thresholds
    static constexpr int IR_THRESHOLD = 240;
    static constexpr double MIN_AREA = 20.0;
    static constexpr double MAX_AREA = 8000.0;

    // Timing
    static constexpr double DRAW_END_GAP_SEC = 0.25;

    std::string playerId;
    std::string serverIp;
    std::atomic<bool> running{true};
    std::atomic<bool> spellCastThisRound{false};
    std::atomic<GamePhase> currentPhase{GamePhase::LOBBY};
    std::thread workerThread;

public:
    std::function<void(bool success, const std::string& message)> onRecognitionResult;

    ImageProcessing(const std::string& id, const std::string& ip);
    ~ImageProcessing();

    void start();
    void setDebugStream(bool enable);

private:
    bool debugEnabled = false;
    int debugSocket = -1;
    int debugClient = -1;

    void run();
    cv::Point detectIR(const cv::Mat& gray);
    std::string mapToGameSpell(Spell gesture);
    cv::Mat drawDebugOverlay(const cv::Mat& bgr, const cv::Point& tip,
                             const std::vector<cv::Point>& gesture);
    void startDebugServer();
    void sendDebugFrame(const cv::Mat& frame);
};
