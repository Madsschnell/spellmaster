#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdint>
#include <ctime>

#include <opencv2/opencv.hpp>

#include "spells.hpp"
#include "gesture_classifier.hpp"

// Camera
static constexpr int CAP_W = 640;
static constexpr int CAP_H = 480;
static constexpr int FRAMERATE = 30;

// IR detection
static constexpr int IR_THRESHOLD = 240;
static constexpr double MIN_AREA = 20.0;
static constexpr double MAX_AREA = 8000.0;

// Drawing
static constexpr int BRUSH_RADIUS = 8;
static constexpr float BRUSH_ALPHA = 0.85f;

// Colours
static const cv::Scalar BRUSH_COLOR = {50, 220, 255};
static const cv::Scalar CURSOR_COLOR = {255, 255, 255};

// Botched timeout (no gesture for this long)
static constexpr double BOTCHED_TIMEOUT = 20.0; // seconds

int main()
{
    const int frameSize = CAP_W * CAP_H * 3 / 2;

    std::string cmd =
        "rpicam-vid"
        " --width " +
        std::to_string(CAP_W) +
        " --height " + std::to_string(CAP_H) +
        " --framerate " + std::to_string(FRAMERATE) +
        " --codec yuv420"
        " --output -"
        " --timeout 0"
        " --nopreview"
        " 2>/dev/null";

    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
    {
        std::cerr << "Failed to open camera pipe\n";
        return 1;
    }

    std::vector<uint8_t> buf(frameSize);

    cv::Mat canvas = cv::Mat::zeros(CAP_H, CAP_W, CV_8UC3);
    cv::namedWindow("Spell Casting", cv::WINDOW_NORMAL);
    cv::resizeWindow("Spell Casting", CAP_W, CAP_H);

    cv::Point prevPt(-1, -1);
    std::vector<cv::Point> gesture;

    Spell lastSpell = Spell::NONE;
    double lastSpellTime = 0.0;

    double lastSeenTime = (double)cv::getTickCount() / cv::getTickFrequency();

    while (true)
    {
        size_t total = 0;
        while (total < (size_t)frameSize)
        {
            size_t got = fread(buf.data() + total, 1, frameSize - total, pipe);
            if (got == 0)
                goto done;
            total += got;
        }

        cv::Mat yuv(CAP_H * 3 / 2, CAP_W, CV_8UC1, buf.data());
        cv::Mat bgr;
        cv::cvtColor(yuv, bgr, cv::COLOR_YUV2BGR_I420);

        cv::Mat gray, thresh;
        cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);
        cv::threshold(gray, thresh, IR_THRESHOLD, 255, cv::THRESH_BINARY);

        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, {5, 5});
        cv::morphologyEx(thresh, thresh, cv::MORPH_OPEN, kernel);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        cv::Point tip(-1, -1);
        double bestArea = 0.0;
        for (auto &c : contours)
        {
            double a = cv::contourArea(c);
            if (a < MIN_AREA || a > MAX_AREA)
                continue;
            if (a > bestArea)
            {
                bestArea = a;
                cv::Moments m = cv::moments(c);
                if (m.m00 > 0)
                    tip = {(int)(m.m10 / m.m00), (int)(m.m01 / m.m00)};
            }
        }

        bool irSeen = (tip.x >= 0);
        bool drawActive = irSeen;

        double now = (double)cv::getTickCount() / cv::getTickFrequency();

        if (irSeen)
        {
            lastSeenTime = now;
        }

        if (drawActive)
        {
            gesture.push_back(tip);

            if (prevPt.x >= 0)
            {
                cv::line(canvas, prevPt, tip,
                         BRUSH_COLOR, BRUSH_RADIUS * 2,
                         cv::LINE_AA);
            }
            else
            {
                cv::circle(canvas, tip, BRUSH_RADIUS, BRUSH_COLOR, -1, cv::LINE_AA);
            }
            prevPt = tip;
        }
        else
        {
            if (!gesture.empty())
            {
                Spell s = classifyGesture(gesture, cv::Size(CAP_W, CAP_H));
                if (s == Spell::NONE)
                {
                    lastSpell = Spell::BOTCHED;
                    lastSpellTime = now;
                    std::cout << "Spell botched: unrecognised gesture\n";
                }
                else
                {
                    lastSpell = s;
                    lastSpellTime = now;
                    std::cout << "Spell cast: " << spellToString(s) << std::endl;
                }
                gesture.clear();
            }
            prevPt = cv::Point(-1, -1);
        }

        if (!irSeen && (now - lastSeenTime) > BOTCHED_TIMEOUT)
        {
            lastSpell = Spell::BOTCHED;
            lastSpellTime = now;
            std::cout << "Spell botched: timeout\n";
            lastSeenTime = now;
        }

        cv::Mat display;
        cv::addWeighted(bgr, 1.0f, canvas, BRUSH_ALPHA, 0.0, display);

        if (irSeen)
        {
            int cs = 12;
            cv::line(display, {tip.x - cs, tip.y}, {tip.x + cs, tip.y},
                     CURSOR_COLOR, 1, cv::LINE_AA);
            cv::line(display, {tip.x, tip.y - cs}, {tip.x, tip.y + cs},
                     CURSOR_COLOR, 1, cv::LINE_AA);
            cv::circle(display, tip, 6, CURSOR_COLOR, 1, cv::LINE_AA);
        }

        if (lastSpell != Spell::NONE)
        {
            if ((now - lastSpellTime) < 3.0)
            {
                std::string txt = "SPELL: " + spellToString(lastSpell);
                cv::rectangle(display, {10, 10}, {630, 60}, {0, 0, 0}, -1);
                cv::putText(display, txt, {20, 50},
                            cv::FONT_HERSHEY_SIMPLEX, 1.0,
                            {0, 255, 255}, 2, cv::LINE_AA);
            }
        }

        cv::putText(display, "C=clear  ESC=quit",
                    {8, CAP_H - 8}, cv::FONT_HERSHEY_SIMPLEX,
                    0.45, {200, 200, 200}, 1, cv::LINE_AA);

        cv::imshow("Spell Casting", display);

        int key = cv::waitKey(1);
        if (key == 27)
            break;
        if (key == 'c' || key == 'C')
        {
            canvas = cv::Mat::zeros(CAP_H, CAP_W, CV_8UC3);
            prevPt = {-1, -1};
            gesture.clear();
            lastSpell = Spell::NONE;
        }
    }

done:
    pclose(pipe);
    return 0;
}
