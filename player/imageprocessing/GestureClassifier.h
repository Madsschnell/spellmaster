#ifndef GESTURE_CLASSIFIER_H
#define GESTURE_CLASSIFIER_H

#include <vector>
#include <opencv2/opencv.hpp>
#include "Spells.h"

// Classify a completed gesture (sequence of points in image coordinates)
Spell classifyGesture(const std::vector<cv::Point> &pts, const cv::Size &frameSize);

#endif // GESTURE_CLASSIFIER_H
