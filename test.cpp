#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/ml.hpp>

using namespace cv;
using namespace cv::ml;

int main()
{
    HOGDescriptor hog(Size(64, 64), Size(16, 16), Size(8, 8), Size(8, 8), 9);
    Ptr<SVM> svm = SVM::load("hand_svm.xml");

    const int capWidth = 640, capHeight = 480;
    const int frameSize = capWidth * capHeight * 3 / 2;
    std::string cmd = "rpicam-vid --width 640 --height 480 --framerate 30 --codec yuv420 --output - --timeout 0 --nopreview 2>/dev/null";
    FILE *pipe = popen(cmd.c_str(), "r");
    std::vector<uint8_t> buf(frameSize);

    cv::namedWindow("Recognition", cv::WINDOW_NORMAL);

    while (true)
    {
        size_t total = 0;
        while (total < frameSize)
        {
            size_t got = fread(buf.data() + total, 1, frameSize - total, pipe);
            if (got == 0)
                break;
            total += got;
        }

        Mat yuv(capHeight * 3 / 2, capWidth, CV_8UC1, buf.data());
        Mat bgr;
        cvtColor(yuv, bgr, COLOR_YUV2BGR_I420);

        Mat resized;
        resize(bgr, resized, Size(64, 64));
        std::vector<float> descriptors;
        hog.compute(resized, descriptors);

        Mat testMat = Mat(descriptors).reshape(1, 1);

        // Use predict and round the result to be safe
        float res = svm->predict(testMat);
        int response = cvRound(res);

        std::string labelText = "NONE";
        Scalar color = Scalar(200, 200, 200);

        if (response == 1)
        {
            labelText = "OPEN";
            color = Scalar(0, 255, 0);
        }
        else if (response == 0)
        {
            labelText = "CLOSED";
            color = Scalar(0, 0, 255);
        }
        else if (response == 2)
        {
            labelText = "NONE";
            color = Scalar(255, 255, 255);
        }

        cv::rectangle(bgr, {10, 10}, {350, 80}, Scalar(0, 0, 0), -1);
        cv::putText(bgr, "STATE: " + labelText, {20, 60}, FONT_HERSHEY_SIMPLEX, 1.2, color, 2);

        cv::imshow("Recognition", bgr);
        if (waitKey(1) == 27)
            break;
    }
    pclose(pipe);
    return 0;
}