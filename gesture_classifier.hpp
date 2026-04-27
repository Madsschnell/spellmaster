#ifndef GESTURE_CLASSIFIER_HPP
#define GESTURE_CLASSIFIER_HPP

#include <vector>
#include <opencv2/opencv.hpp>
#include "spells.hpp"

// Classify a completed gesture (sequence of points in image coordinates)
Spell classifyGesture(const std::vector<cv::Point> &pts, const cv::Size &frameSize);

#endif // GESTURE_CLASSIFIER_HPP
