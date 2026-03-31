#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <opencv2/opencv.hpp>
#include <opencv2/ml.hpp>

using namespace cv;
using namespace cv::ml;

int main()
{
    // 1. Setup HOG Descriptor (Must match your training parameters exactly!)
    HOGDescriptor hog(
        Size(64, 64),
        Size(16, 16),
        Size(8, 8),
        Size(8, 8),
        9);

    // 2. Load the trained SVM model
    Ptr<SVM> svm;
    try
    {
        svm = SVM::load("hand_svm.xml");
    }
    catch (const cv::Exception &e)
    {
        std::cerr << "Error: Could not load hand_svm.xml. Did you run the training script?\n";
        return 1;
    }

    // 3. Camera Setup (matching your RPi5 pipe)
    const int capWidth = 640;
    const int capHeight = 480;
    const int frameSize = capWidth * capHeight * 3 / 2;
    std::string cmd = "rpicam-vid --width 640 --height 480 --framerate 30 "
                      "--codec yuv420 --output - --timeout 0 --nopreview 2>/dev/null";

    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
    {
        std::cerr << "Failed to open camera pipe\n";
        return 1;
    }

    std::vector<uint8_t> buf(frameSize);
    cv::namedWindow("Hand Recognition", cv::WINDOW_NORMAL);
    cv::setWindowProperty("Hand Recognition", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);

    std::cout << "Starting Inference... Press 'ESC' to exit." << std::endl;

    while (true)
    {
        // Read YUV frame from pipe
        size_t total = 0;
        while (total < frameSize)
        {
            size_t got = fread(buf.data() + total, 1, frameSize - total, pipe);
            if (got == 0)
                break;
            total += got;
        }

        // Convert YUV to BGR
        Mat yuv(capHeight * 3 / 2, capWidth, CV_8UC1, buf.data());
        Mat bgr;
        cvtColor(yuv, bgr, COLOR_YUV2BGR_I420);

        // 4. Pre-process for SVM
        Mat resized;
        resize(bgr, resized, Size(64, 64)); // Match training size

        std::vector<float> descriptors;
        hog.compute(resized, descriptors);

        // 5. Predict
        // We convert the vector to a 1-row Mat of floats
        Mat testMat = Mat(descriptors).reshape(1, 1);
        float response = svm->predict(testMat);

        // 6. UI Feedback
        std::string label = (response == 1.0) ? "OPEN" : "CLOSED";
        Scalar color = (response == 1.0) ? Scalar(0, 255, 0) : Scalar(0, 0, 255);

        // Draw a background box for the text to make it readable
        cv::rectangle(bgr, {10, 10}, {300, 80}, Scalar(0, 0, 0), -1);
        cv::putText(bgr, "STATE: " + label, {20, 60},
                    FONT_HERSHEY_SIMPLEX, 1.5, color, 3);

        cv::imshow("Hand Recognition", bgr);

        if (waitKey(1) == 27)
            break; // ESC to quit
    }

    pclose(pipe);
    return 0;
}