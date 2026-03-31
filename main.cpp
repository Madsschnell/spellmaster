#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <opencv2/opencv.hpp>

// ============================
// AUTO DATA COLLECTION VERSION
// ============================

int main()
{
    const int capWidth = 640;
    const int capHeight = 480;
    const int frameSize = capWidth * capHeight * 3 / 2;

    std::string cmd =
        "rpicam-vid --width 640 --height 480 "
        "--framerate 30 --codec yuv420 "
        "--output - --timeout 0 --nopreview 2>/dev/null";

    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
    {
        std::cerr << "Camera failed\n";
        return 1;
    }

    std::vector<uint8_t> buf(frameSize);

    cv::namedWindow("Camera", cv::WINDOW_NORMAL);
    cv::setWindowProperty("Camera", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);

    enum Mode
    {
        WAIT_OPEN,
        CAPTURE_OPEN,
        WAIT_CLOSED,
        CAPTURE_CLOSED,
        DONE
    };
    Mode mode = WAIT_OPEN;

    int countdown = 5;
    int captureCount = 0;
    const int TARGET = 300;

    auto lastTime = std::chrono::steady_clock::now();

    while (true)
    {
        // Read frame
        size_t total = 0;
        while (total < frameSize)
        {
            size_t got = fread(buf.data() + total, 1, frameSize - total, pipe);
            if (got == 0)
                goto done;
            total += got;
        }

        cv::Mat yuv(capHeight * 3 / 2, capWidth, CV_8UC1, buf.data());
        cv::Mat bgr;
        cv::cvtColor(yuv, bgr, cv::COLOR_YUV2BGR_I420);

        cv::Mat display;
        cv::resize(bgr, display, {800, 480});

        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - lastTime).count();

        // ============================
        // STATE MACHINE
        // ============================

        if (mode == WAIT_OPEN)
        {
            if (elapsed >= 1.0)
            {
                countdown--;
                lastTime = now;
            }

            cv::putText(display, "SHOW OPEN HAND",
                        {100, 150}, cv::FONT_HERSHEY_SIMPLEX, 1.5,
                        {0, 255, 0}, 3);

            cv::putText(display, std::to_string(countdown),
                        {350, 300}, cv::FONT_HERSHEY_SIMPLEX, 4,
                        {0, 255, 0}, 5);

            if (countdown <= 0)
            {
                mode = CAPTURE_OPEN;
                captureCount = 0;
            }
        }

        else if (mode == CAPTURE_OPEN)
        {
            // Save images fast
            cv::imwrite("open/" + std::to_string(time(NULL)) +
                            "_" + std::to_string(captureCount) + ".jpg",
                        bgr);

            captureCount++;

            cv::putText(display, "CAPTURING OPEN",
                        {100, 100}, cv::FONT_HERSHEY_SIMPLEX, 1.5,
                        {0, 255, 0}, 3);

            cv::putText(display,
                        std::to_string(captureCount) + "/" + std::to_string(TARGET),
                        {200, 300}, cv::FONT_HERSHEY_SIMPLEX, 2,
                        {0, 255, 0}, 3);

            if (captureCount >= TARGET)
            {
                mode = WAIT_CLOSED;
                countdown = 5;
                lastTime = now;
            }
        }

        else if (mode == WAIT_CLOSED)
        {
            if (elapsed >= 1.0)
            {
                countdown--;
                lastTime = now;
            }

            cv::putText(display, "SHOW CLOSED HAND",
                        {80, 150}, cv::FONT_HERSHEY_SIMPLEX, 1.5,
                        {0, 0, 255}, 3);

            cv::putText(display, std::to_string(countdown),
                        {350, 300}, cv::FONT_HERSHEY_SIMPLEX, 4,
                        {0, 0, 255}, 5);

            if (countdown <= 0)
            {
                mode = CAPTURE_CLOSED;
                captureCount = 0;
            }
        }

        else if (mode == CAPTURE_CLOSED)
        {
            cv::imwrite("closed/" + std::to_string(time(NULL)) +
                            "_" + std::to_string(captureCount) + ".jpg",
                        bgr);

            captureCount++;

            cv::putText(display, "CAPTURING CLOSED",
                        {80, 100}, cv::FONT_HERSHEY_SIMPLEX, 1.5,
                        {0, 0, 255}, 3);

            cv::putText(display,
                        std::to_string(captureCount) + "/" + std::to_string(TARGET),
                        {200, 300}, cv::FONT_HERSHEY_SIMPLEX, 2,
                        {0, 0, 255}, 3);

            if (captureCount >= TARGET)
            {
                mode = DONE;
            }
        }

        else if (mode == DONE)
        {
            cv::putText(display, "DONE!",
                        {250, 250}, cv::FONT_HERSHEY_SIMPLEX, 2,
                        {255, 255, 255}, 4);

            cv::imshow("Camera", display);
            cv::waitKey(3000);
            break;
        }

        cv::imshow("Camera", display);

        if (cv::waitKey(1) == 27)
            break;
    }

done:
    pclose(pipe);
    return 0;
}