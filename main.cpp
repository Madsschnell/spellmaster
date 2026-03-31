#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <opencv2/opencv.hpp>

int main()
{
    const int capWidth = 640, capHeight = 480;
    const int frameSize = capWidth * capHeight * 3 / 2;
    std::string cmd = "rpicam-vid --width 640 --height 480 --framerate 30 --codec yuv420 --output - --timeout 0 --nopreview 2>/dev/null";
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
        return 1;

    std::vector<uint8_t> buf(frameSize);
    cv::namedWindow("Collector", cv::WINDOW_NORMAL);

    enum Mode
    {
        WAIT_OPEN,
        CAP_OPEN,
        WAIT_CLOSED,
        CAP_CLOSED,
        WAIT_NONE,
        CAP_NONE,
        DONE
    };
    Mode mode = WAIT_OPEN;
    int countdown = 5, captureCount = 0;
    const int TARGET = 300;
    auto lastTime = std::chrono::steady_clock::now();

    while (mode != DONE)
    {
        size_t total = 0;
        while (total < frameSize)
        {
            size_t got = fread(buf.data() + total, 1, frameSize - total, pipe);
            if (got == 0)
                break;
            total += got;
        }
        cv::Mat yuv(capHeight * 3 / 2, capWidth, CV_8UC1, buf.data());
        cv::Mat bgr, display;
        cv::cvtColor(yuv, bgr, cv::COLOR_YUV2BGR_I420);
        display = bgr.clone();

        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - lastTime).count();

        if (mode == WAIT_OPEN || mode == WAIT_CLOSED || mode == WAIT_NONE)
        {
            if (elapsed >= 1.0)
            {
                countdown--;
                lastTime = now;
            }
            std::string msg = (mode == WAIT_OPEN) ? "SHOW OPEN" : (mode == WAIT_CLOSED) ? "SHOW CLOSED"
                                                                                        : "CLEAR FRAME (NONE)";
            cv::putText(display, msg + " " + std::to_string(countdown), {50, 240}, 1, 3.0, {0, 255, 0}, 3);
            if (countdown <= 0)
            {
                captureCount = 0;
                mode = (mode == WAIT_OPEN) ? CAP_OPEN : (mode == WAIT_CLOSED) ? CAP_CLOSED
                                                                              : CAP_NONE;
            }
        }
        else
        {
            std::string folder = (mode == CAP_OPEN) ? "open/" : (mode == CAP_CLOSED) ? "closed/"
                                                                                     : "none/";
            cv::imwrite(folder + std::to_string(time(NULL)) + "_" + std::to_string(captureCount) + ".jpg", bgr);
            captureCount++;
            cv::putText(display, "CAPTURING " + std::to_string(captureCount), {50, 240}, 1, 3.0, {255, 255, 0}, 3);
            if (captureCount >= TARGET)
            {
                countdown = 5;
                lastTime = now;
                if (mode == CAP_OPEN)
                    mode = WAIT_CLOSED;
                else if (mode == CAP_CLOSED)
                    mode = WAIT_NONE;
                else
                    mode = DONE;
            }
        }
        cv::imshow("Collector", display);
        if (cv::waitKey(1) == 27)
            break;
    }
    pclose(pipe);
    return 0;
}