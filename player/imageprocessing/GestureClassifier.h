// GestureClassifier.h
// Pattern matching for IR wand gestures drawn in the air

#pragma once

#include <vector>
#include <opencv2/opencv.hpp>
#include "Spells.h"

/// Classify a completed gesture from a sequence of tracked IR points.
/// Matches against known patterns: circle, square, zig-zag, spiral, etc.
/// @param pts Accumulated IR positions from one drawing stroke
/// @param frameSize Camera frame dimensions (for normalization)
/// @return Recognized gesture type, or Spell::NONE if unrecognized
Spell classifyGesture(const std::vector<cv::Point> &pts, const cv::Size &frameSize);
