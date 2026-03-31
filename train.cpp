#include <opencv2/opencv.hpp>
#include <opencv2/ml.hpp>
#include <iostream>
#include <vector>

using namespace cv;
using namespace cv::ml;

HOGDescriptor hog(
    Size(64, 64),
    Size(16, 16),
    Size(8, 8),
    Size(8, 8),
    9);

void loadImages(const std::string &folder, int label,
                std::vector<Mat> &samples,
                std::vector<int> &labels)
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
    std::vector<Mat> samples;
    std::vector<int> labels;

    loadImages("open/*.jpg", 1, samples, labels);
    loadImages("closed/*.jpg", 0, samples, labels);

    Mat trainData;
    vconcat(samples, trainData);

    Mat labelMat(labels);

    Ptr<SVM> svm = SVM::create();
    svm->setType(SVM::C_SVC);
    svm->setKernel(SVM::LINEAR);
    svm->setTermCriteria(TermCriteria(TermCriteria::MAX_ITER, 1000, 1e-6));

    svm->train(trainData, ROW_SAMPLE, labelMat);
    svm->save("hand_svm.xml");

    std::cout << "Training complete!\n";
}