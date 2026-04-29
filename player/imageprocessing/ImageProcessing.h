// ImageProcessing.h
// Camera-based gesture recognition using rpicam-vid and OpenCV.
// Runs in its own thread with own ZeroMQ sockets (SUB + PUSH).
// Communicates with UI via onRecognitionResult callback.

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

/// IR wand tracking and gesture recognition via rpicam-vid.
/// Captures frames, detects IR point, accumulates drawing path,
/// classifies gesture, and sends SpellMessage to GameMaster.
class ImageProcessing {
    // Camera settings
    static constexpr int CAP_W = 640;
    static constexpr int CAP_H = 480;
    static constexpr int FRAMERATE = 30;

    // IR detection thresholds
    static constexpr int IR_THRESHOLD = 240;
    static constexpr double MIN_AREA = 20.0;
    static constexpr double MAX_AREA = 8000.0;

    // Debounce: minimum gap before pen-up is registered
    static constexpr double DRAW_END_GAP_SEC = 0.25;

    std::string playerId;
    std::string serverIp;
    std::atomic<bool> running{true};
    std::atomic<bool> spellCastThisRound{false};
    std::atomic<GamePhase> currentPhase{GamePhase::LOBBY};
    std::thread workerThread;

public:
    /// Called when a gesture is recognized or rejected.
    /// @param success true if a valid spell was cast, false for error/unsupported
    /// @param message Description for UI display (e.g. "Spell: Fireball")
    std::function<void(bool success, const std::string& message)> onRecognitionResult;

    /// @param id Player ID ("player1" or "player2")
    /// @param ip GameMaster IP address for ZeroMQ connections
    ImageProcessing(const std::string& id, const std::string& ip);
    ~ImageProcessing();

    /// Start camera capture and tracking in a background thread
    void start();

    /// Enable MJPEG debug stream on port 8080.
    /// When enabled, every 3rd frame is sent as JPEG with IR overlay.
    /// @param enable true to start debug server
    void setDebugStream(bool enable);

private:
    bool debugEnabled = false;
    int debugSocket = -1;
    int debugClient = -1;

    void run();

    /// Detect brightest IR point in a grayscale frame (Y channel)
    /// @param gray Grayscale image (direct Y channel from YUV420)
    /// @return Centroid of brightest IR blob, or (-1,-1) if none found
    cv::Point detectIR(const cv::Mat& gray);

    /// Map gesture classifier output to game spell name
    /// @param gesture Classified gesture type
    /// @return Spell name for GameMaster (e.g. "Fireball"), or empty if unsupported
    std::string mapToGameSpell(Spell gesture);

    /// Draw debug overlay with IR crosshair, gesture path, and phase indicator
    /// @param bgr BGR frame for drawing on
    /// @param tip Current IR point position
    /// @param gesture Accumulated gesture path
    /// @return BGR frame with overlay drawn
    cv::Mat drawDebugOverlay(const cv::Mat& bgr, const cv::Point& tip,
                             const std::vector<cv::Point>& gesture);

    void startDebugServer();

    /// Encode frame as JPEG and send to connected debug viewer
    /// @param frame BGR frame to send
    void sendDebugFrame(const cv::Mat& frame);
};
