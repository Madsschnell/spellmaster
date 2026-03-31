#include <opencv2/opencv.hpp>
#include <opencv2/ml.hpp>
#include <iostream>
#include <vector>

using namespace cv;
using namespace cv::ml;

void loadImages(const std::string &folder, int label, std::vector<Mat> &samples, std::vector<int> &labels, HOGDescriptor &hog)
{
    std::vector<String> files;
    glob(folder, files);
    for (auto &f : files)
    {
        Mat img = imread(f);
        if (img.empty())
            continue;
        resize(img, img, Size(64, 64));
        std::vector<float> feat;
        hog.compute(img, feat);
        samples.push_back(Mat(feat).clone().reshape(1, 1));
        labels.push_back(label);
    }
}

int main()
{
    HOGDescriptor hog(Size(64, 64), Size(16, 16), Size(8, 8), Size(8, 8), 9);
    std::vector<Mat> samples;
    std::vector<int> labels;

    std::cout << "Loading data..." << std::endl;
    loadImages("closed/*.jpg", 0, samples, labels, hog);
    loadImages("open/*.jpg", 1, samples, labels, hog);
    loadImages("none/*.jpg", 2, samples, labels, hog); // LABEL 2 IS NONE

    Mat trainData;
    vconcat(samples, trainData);
    Mat labelMat(labels);

    Ptr<SVM> svm = SVM::create();
    svm->setType(SVM::C_SVC);
    svm->setKernel(SVM::LINEAR);
    svm->train(trainData, ROW_SAMPLE, labelMat);
    svm->save("hand_svm.xml");
    std::cout << "Training complete! Model saved." << std::endl;
    return 0;
}