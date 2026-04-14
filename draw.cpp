// How it works:
//   1. Capture YUV420 from rpicam-vid, convert to BGR.
//   2. Convert to grayscale and threshold at a high value (e.g. 240) to isolate
//      the IR spot.
//   3. Find contours; pick the largest one as the wand tip.
//   4. If the wand is detected AND its area is above a minimum size, treat the
//      button as "held" and paint a coloured circle onto a persistent canvas.
//      (See NOTE below about button detection options.)
//   5. Composite the canvas over the live feed and show the result.
//
// NOTE – Button / "drawing active" detection:
//     The IR LED is only on when the button is held. When off, no blob is
//     detected.

#include <iostream>
#include <vector>
#include <deque>
#include <string>
#include <cstdio>
#include <cstdint>
#include <opencv2/opencv.hpp>

// ── Tunable parameters ────────────────────────────────────────────────────────

// Camera
static constexpr int CAP_W = 640;
static constexpr int CAP_H = 480;
static constexpr int FRAMERATE = 30;

// IR detection
static constexpr int IR_THRESHOLD = 240;   // 0-255; raise if false positives
static constexpr double MIN_AREA = 20.0;   // ignore tiny noise blobs
static constexpr double MAX_AREA = 8000.0; // ignore huge blobs

// Drawing
static constexpr int BRUSH_RADIUS = 8;      // pixels
static constexpr float BRUSH_ALPHA = 0.85f; // opacity of drawn trail (0-1)
static constexpr int TRAIL_LEN = 0;         // 0 = permanent; >0 = fade after N frames

// Draw window
static constexpr double DRAW_WINDOW_SEC = 10.0;

// Colours (BGR) – change to taste
static const cv::Scalar BRUSH_COLOR = {50, 220, 255};   // yellow-ish
static const cv::Scalar CURSOR_COLOR = {255, 255, 255}; // white crosshair

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

    // Persistent drawing canvas (BGRA so we can use the alpha channel)
    cv::Mat canvas = cv::Mat::zeros(CAP_H, CAP_W, CV_8UC3);

    // Optional: trail of recent positions for a glowing-line effect
    std::deque<cv::Point> trail;

    cv::namedWindow("IR Drawing", cv::WINDOW_NORMAL);
    cv::resizeWindow("IR Drawing", CAP_W, CAP_H);

    cv::Point prevPt(-1, -1);

    bool inDrawWindow = false;
    double drawWindowStart = 0.0;

    while (true)
    {
        // ── Read one YUV420 frame ─────────────────────────────────────────────
        size_t total = 0;
        while (total < (size_t)frameSize)
        {
            size_t got = fread(buf.data() + total, 1, frameSize - total, pipe);
            if (got == 0)
                goto done;
            total += got;
        }

        {
            cv::Mat yuv(CAP_H * 3 / 2, CAP_W, CV_8UC1, buf.data());
            cv::Mat bgr;
            cv::cvtColor(yuv, bgr, cv::COLOR_YUV2BGR_I420);

            // ── Detect IR blob ────────────────────────────────────────────────
            cv::Mat gray, thresh;
            cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);
            cv::threshold(gray, thresh, IR_THRESHOLD, 255, cv::THRESH_BINARY);

            // Optional: erode once to kill single-pixel noise, then dilate back
            cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, {5, 5});
            cv::morphologyEx(thresh, thresh, cv::MORPH_OPEN, kernel);

            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

            cv::Point tip(-1, -1);
            double bestArea = 0;
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

            // ── Decide whether to draw ────────────────────────────────────────
            bool irSeen = (tip.x >= 0);

            bool drawActive = irSeen;

            double now = (double)cv::getTickCount() / cv::getTickFrequency();

            if (drawActive)
            {
                if (!inDrawWindow)
                {
                    inDrawWindow = true;
                    drawWindowStart = now;
                }
                else if ((now - drawWindowStart) >= DRAW_WINDOW_SEC)
                {
                    canvas = cv::Mat::zeros(CAP_H, CAP_W, CV_8UC3);
                    prevPt = {-1, -1};
                    trail.clear();
                    drawWindowStart = now;
                }
            }
            else
            {
                inDrawWindow = false;
            }

            if (drawActive && prevPt.x >= 0)
            {
                // Draw a smooth line segment between consecutive detections
                cv::line(canvas, prevPt, tip,
                         BRUSH_COLOR, BRUSH_RADIUS * 2,
                         cv::LINE_AA);
            }
            else if (drawActive)
            {
                // First detected point: paint a dot
                cv::circle(canvas, tip, BRUSH_RADIUS, BRUSH_COLOR, -1, cv::LINE_AA);
            }

            prevPt = drawActive ? tip : cv::Point(-1, -1);

            // ── Composite: bgr + canvas ───────────────────────────────────────
            cv::Mat display;
            cv::addWeighted(bgr, 1.0f, canvas, BRUSH_ALPHA, 0.0, display);

            // ── HUD ──────────────────────────────────────────────────────────
            if (irSeen)
            {
                // Draw a small crosshair at the wand tip
                int cs = 12;
                cv::line(display, {tip.x - cs, tip.y}, {tip.x + cs, tip.y},
                         CURSOR_COLOR, 1, cv::LINE_AA);
                cv::line(display, {tip.x, tip.y - cs}, {tip.x, tip.y + cs},
                         CURSOR_COLOR, 1, cv::LINE_AA);
                cv::circle(display, tip, 6, CURSOR_COLOR, 1, cv::LINE_AA);
            }

            // Key hints
            cv::putText(display, "C=clear  S=save  ESC=quit",
                        {8, CAP_H - 8}, cv::FONT_HERSHEY_SIMPLEX,
                        0.45, {200, 200, 200}, 1, cv::LINE_AA);

            cv::imshow("IR Drawing", display);

            // ── Key handling ─────────────────────────────────────────────────
            int key = cv::waitKey(1);
            if (key == 27)
                break; // ESC – quit
            if (key == 'c' || key == 'C')
            { // C – clear canvas
                canvas = cv::Mat::zeros(CAP_H, CAP_W, CV_8UC3);
                prevPt = {-1, -1};
            }
            if (key == 's' || key == 'S')
            { // S – save drawing
                std::string fname = "drawing_" + std::to_string(time(nullptr)) + ".png";
                cv::imwrite(fname, canvas);
                std::cout << "Saved: " << fname << "\n";
            }
        }
    }

done:
    pclose(pipe);
    return 0;
}