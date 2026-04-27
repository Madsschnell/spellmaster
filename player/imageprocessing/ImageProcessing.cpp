#include "ImageProcessing.h"
#include <iostream>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

ImageProcessing::ImageProcessing(const std::string& id, const std::string& ip)
    : playerId(id), serverIp(ip)
{
}

ImageProcessing::~ImageProcessing()
{
    running = false;
    if (workerThread.joinable()) workerThread.join();
    if (debugClient >= 0) close(debugClient);
    if (debugSocket >= 0) close(debugSocket);
}

void ImageProcessing::start()
{
    workerThread = std::thread(&ImageProcessing::run, this);
}

void ImageProcessing::setDebugStream(bool enable)
{
    debugEnabled = enable;
}

cv::Point ImageProcessing::detectIR(const cv::Mat& gray)
{
    cv::Mat thresh;
    cv::threshold(gray, thresh, IR_THRESHOLD, 255, cv::THRESH_BINARY);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, {5, 5});
    cv::morphologyEx(thresh, thresh, cv::MORPH_OPEN, kernel);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    cv::Point tip(-1, -1);
    double bestArea = 0.0;
    for (auto& c : contours) {
        double a = cv::contourArea(c);
        if (a < MIN_AREA || a > MAX_AREA) continue;
        if (a > bestArea) {
            bestArea = a;
            cv::Moments m = cv::moments(c);
            if (m.m00 > 0)
                tip = {(int)(m.m10 / m.m00), (int)(m.m01 / m.m00)};
        }
    }
    return tip;
}

std::string ImageProcessing::mapToGameSpell(Spell gesture)
{
    switch (gesture) {
    case Spell::FIREBALL:  return "Fireball";
    case Spell::LIGHTNING: return "Lightning";
    case Spell::SHIELD:    return "Shield";
    case Spell::HEAL:      return "Heal";
    default:               return "";
    }
}

cv::Mat ImageProcessing::drawDebugOverlay(const cv::Mat& bgr, const cv::Point& tip,
                                          const std::vector<cv::Point>& gesture)
{
    cv::Mat display = bgr.clone();

    // Draw accumulated gesture path
    for (size_t i = 1; i < gesture.size(); i++) {
        cv::line(display, gesture[i-1], gesture[i],
                 cv::Scalar(50, 220, 255), 3, cv::LINE_AA);
    }

    // Draw crosshair at current IR position
    if (tip.x >= 0) {
        int cs = 12;
        cv::line(display, {tip.x - cs, tip.y}, {tip.x + cs, tip.y},
                 cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
        cv::line(display, {tip.x, tip.y - cs}, {tip.x, tip.y + cs},
                 cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
        cv::circle(display, tip, 6, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
    }

    // Phase indicator
    std::string phaseText;
    switch (currentPhase.load()) {
    case GamePhase::LOBBY:     phaseText = "LOBBY"; break;
    case GamePhase::GET_READY: phaseText = "GET READY"; break;
    case GamePhase::CASTING:   phaseText = "CASTING"; break;
    case GamePhase::RESOLVING: phaseText = "RESOLVING"; break;
    case GamePhase::GAME_OVER: phaseText = "GAME OVER"; break;
    }
    cv::putText(display, phaseText, {10, 30},
                cv::FONT_HERSHEY_SIMPLEX, 0.7, {0, 255, 255}, 2, cv::LINE_AA);

    return display;
}

void ImageProcessing::startDebugServer()
{
    debugSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (debugSocket < 0) {
        std::cerr << "Debug stream: Failed to create socket" << std::endl;
        debugEnabled = false;
        return;
    }

    int opt = 1;
    setsockopt(debugSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    if (bind(debugSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Debug stream: Failed to bind port 8080" << std::endl;
        close(debugSocket);
        debugSocket = -1;
        debugEnabled = false;
        return;
    }

    listen(debugSocket, 1);
    std::cout << "Debug stream: Waiting for viewer on port 8080..." << std::endl;

    // Non-blocking accept will be checked in the main loop
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    setsockopt(debugSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

void ImageProcessing::sendDebugFrame(const cv::Mat& frame)
{
    if (!debugEnabled) return;

    // Try to accept new client if none connected
    if (debugClient < 0 && debugSocket >= 0) {
        struct sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);
        int fd = accept(debugSocket, (struct sockaddr*)&clientAddr, &len);
        if (fd >= 0) {
            debugClient = fd;
            // Send HTTP MJPEG header
            std::string header =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
                "\r\n";
            send(debugClient, header.c_str(), header.size(), MSG_NOSIGNAL);
            std::cout << "Debug stream: Viewer connected" << std::endl;
        }
    }

    if (debugClient < 0) return;

    // Encode frame as JPEG
    std::vector<uchar> jpegBuf;
    cv::imencode(".jpg", frame, jpegBuf, {cv::IMWRITE_JPEG_QUALITY, 60});

    // Send MJPEG frame
    std::string boundary =
        "--frame\r\n"
        "Content-Type: image/jpeg\r\n"
        "Content-Length: " + std::to_string(jpegBuf.size()) + "\r\n"
        "\r\n";

    ssize_t sent = send(debugClient, boundary.c_str(), boundary.size(), MSG_NOSIGNAL);
    if (sent < 0) {
        close(debugClient);
        debugClient = -1;
        return;
    }

    sent = send(debugClient, jpegBuf.data(), jpegBuf.size(), MSG_NOSIGNAL);
    if (sent < 0) {
        close(debugClient);
        debugClient = -1;
        return;
    }

    std::string end = "\r\n";
    send(debugClient, end.c_str(), end.size(), MSG_NOSIGNAL);
}

void ImageProcessing::run()
{
    // ZeroMQ setup
    zmq::context_t context(1);

    zmq::socket_t subscriber(context, zmq::socket_type::sub);
    subscriber.connect("tcp://" + serverIp + ":5555");
    subscriber.set(zmq::sockopt::subscribe, playerId);
    std::cout << "ImageProc SUB connected to tcp://" << serverIp << ":5555" << std::endl;

    zmq::socket_t sender(context, zmq::socket_type::push);
    sender.connect("tcp://" + serverIp + ":5556");
    std::cout << "ImageProc PUSH connected to tcp://" << serverIp << ":5556" << std::endl;

    // Debug stream server
    if (debugEnabled) startDebugServer();

    // Camera setup via rpicam-vid pipe
    std::string cmd =
        "rpicam-vid"
        " --width " + std::to_string(CAP_W) +
        " --height " + std::to_string(CAP_H) +
        " --framerate " + std::to_string(FRAMERATE) +
        " --codec yuv420"
        " --output -"
        " --timeout 0"
        " --nopreview"
        " 2>/dev/null";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "ImageProc: Failed to open camera pipe" << std::endl;
        if (onRecognitionResult)
            onRecognitionResult(false, "Camera not available");
        return;
    }

    std::cout << "ImageProc: Camera started (" << CAP_W << "x" << CAP_H
              << " @ " << FRAMERATE << " fps)" << std::endl;

    const int frameSize = CAP_W * CAP_H * 3 / 2;
    std::vector<uint8_t> buf(frameSize);

    // Gesture tracking state
    std::vector<cv::Point> gesture;
    cv::Point prevPt(-1, -1);
    cv::Point lastTip(-1, -1);
    double lastDrawActiveTime = 0.0;

    auto now = []() {
        return (double)cv::getTickCount() / cv::getTickFrequency();
    };

    int frameCount = 0;

    while (running) {
        // Poll ZeroMQ for phase updates
        std::string topic;
        GameStatus gs;
        while (zmq_subscribe_recv(subscriber, topic, gs)) {
            GamePhase prevPhase = currentPhase.load();
            currentPhase.store(gs.phase);

            // Reset tracking state when entering CASTING
            if (gs.phase == GamePhase::CASTING && prevPhase != GamePhase::CASTING) {
                spellCastThisRound.store(false);
                gesture.clear();
                prevPt = {-1, -1};
            }
        }

        // Read frame (always, to keep pipe drained)
        {
            size_t total = 0;
            while (total < (size_t)frameSize) {
                size_t got = fread(buf.data() + total, 1, frameSize - total, pipe);
                if (got == 0) goto done;
                total += got;
            }
        }

        // Use Y channel directly as grayscale (first W*H bytes of YUV420)
        {
            cv::Mat gray(CAP_H, CAP_W, CV_8UC1, buf.data());

            cv::Point tip = detectIR(gray);
            lastTip = tip;
            bool irSeen = (tip.x >= 0);
            double t = now();

            // Only process gestures during CASTING and if not yet cast
            bool tracking = (currentPhase.load() == GamePhase::CASTING)
                         && !spellCastThisRound.load();

            if (tracking) {
                if (irSeen) {
                    gesture.push_back(tip);
                    prevPt = tip;
                    lastDrawActiveTime = t;
                } else {
                    // Gesture complete (pen up with debounce)
                    if (!gesture.empty() && (t - lastDrawActiveTime) > DRAW_END_GAP_SEC) {
                        Spell s = classifyGesture(gesture, cv::Size(CAP_W, CAP_H));
                        std::string spellName = mapToGameSpell(s);

                        if (!spellName.empty()) {
                            SpellMessage sm{playerId, spellName};
                            zmq_push(sender, sm);
                            spellCastThisRound.store(true);
                            std::cout << "ImageProc: Cast " << spellName << std::endl;
                            if (onRecognitionResult)
                                onRecognitionResult(true, "Spell: " + spellName);
                        } else {
                            std::string msg = spellToString(s);
                            std::cout << "ImageProc: " << msg << std::endl;
                            if (onRecognitionResult)
                                onRecognitionResult(false, msg);
                        }
                        gesture.clear();
                    }
                    prevPt = {-1, -1};
                }
            }

            // Send debug frame (every 3rd frame, BGR conversion only here)
            if (debugEnabled && (frameCount % 3 == 0)) {
                cv::Mat yuv(CAP_H * 3 / 2, CAP_W, CV_8UC1, buf.data());
                cv::Mat bgr;
                cv::cvtColor(yuv, bgr, cv::COLOR_YUV2BGR_I420);
                cv::Mat debugFrame = drawDebugOverlay(bgr, lastTip, gesture);
                sendDebugFrame(debugFrame);
            }
            frameCount++;
        }
    }

done:
    pclose(pipe);
    std::cout << "ImageProc: Camera stopped" << std::endl;
}
