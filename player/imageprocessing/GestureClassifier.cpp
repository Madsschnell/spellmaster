#include "GestureClassifier.h"
#include <cmath>
#include <algorithm>

static double pathLength(const std::vector<cv::Point> &pts)
{
    if (pts.size() < 2)
        return 0.0;
    double L = 0.0;
    for (size_t i = 1; i < pts.size(); ++i)
        L += cv::norm(pts[i] - pts[i - 1]);
    return L;
}

static cv::Rect boundingBox(const std::vector<cv::Point> &pts)
{
    int minX = pts[0].x, maxX = pts[0].x;
    int minY = pts[0].y, maxY = pts[0].y;
    for (const auto &p : pts)
    {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }
    return cv::Rect(cv::Point(minX, minY), cv::Point(maxX, maxY));
}

static double startEndDist(const std::vector<cv::Point> &pts)
{
    if (pts.size() < 2)
        return 0.0;
    return cv::norm(pts.front() - pts.back());
}

static int countCorners(const std::vector<cv::Point> &pts, double angleThreshDeg = 45.0)
{
    if (pts.size() < 5)
        return 0;

    std::vector<cv::Point2f> simplified;
    simplified.reserve(pts.size() / 2 + 1);
    for (size_t i = 0; i < pts.size(); i += 2)
        simplified.push_back(pts[i]);

    int corners = 0;
    for (size_t i = 1; i + 1 < simplified.size(); ++i)
    {
        cv::Point2f v1 = simplified[i] - simplified[i - 1];
        cv::Point2f v2 = simplified[i + 1] - simplified[i];
        double n1 = cv::norm(v1), n2 = cv::norm(v2);
        if (n1 < 1e-3 || n2 < 1e-3)
            continue;
        double cosA = (v1.dot(v2)) / (n1 * n2);
        cosA = std::max(-1.0, std::min(1.0, cosA));
        double angle = std::acos(cosA) * 180.0 / CV_PI;
        if (angle > angleThreshDeg)
            corners++;
    }
    return corners;
}

static bool isCircleLike(const std::vector<cv::Point> &pts)
{
    if (pts.size() < 20)
        return false;

    cv::Point2f center;
    float radius;
    cv::minEnclosingCircle(pts, center, radius);

    double err = 0.0;
    for (auto &p : pts)
    {
        double d = cv::norm(cv::Point2f(p) - center);
        err += std::abs(d - radius);
    }
    err /= pts.size();

    double se = startEndDist(pts);
    double L = pathLength(pts);

    return (err < radius * 0.25) && (se < radius * 0.4) && (L > 2 * CV_PI * radius * 0.5);
}

static bool isLineLike(const std::vector<cv::Point> &pts)
{
    if (pts.size() < 5)
        return false;

    std::vector<cv::Point2f> fpts;
    fpts.reserve(pts.size());
    for (auto &p : pts)
        fpts.push_back(p);

    cv::Vec4f line;
    cv::fitLine(fpts, line, cv::DIST_L2, 0, 0.01, 0.01);

    cv::Point2f p0(line[2], line[3]);
    cv::Point2f dir(line[0], line[1]);

    double sumDist = 0.0;
    for (auto &p : fpts)
    {
        cv::Point2f v = p - p0;
        double proj = v.dot(dir);
        cv::Point2f projPt = p0 + dir * proj;
        sumDist += cv::norm(p - projPt);
    }
    double avgDist = sumDist / fpts.size();

    cv::Rect bb = boundingBox(pts);
    double diag = std::sqrt(bb.width * bb.width + bb.height * bb.height);

    return avgDist < diag * 0.05 && diag > 40.0;
}

static bool isZigZagLike(const std::vector<cv::Point> &pts)
{
    if (pts.size() < 10)
        return false;

    int signChanges = 0;
    int lastSign = 0;

    for (size_t i = 1; i < pts.size(); ++i)
    {
        int dx = pts[i].x - pts[i - 1].x;
        if (std::abs(dx) < 2)
            continue;
        int s = (dx > 0) ? 1 : -1;
        if (lastSign != 0 && s != lastSign)
            signChanges++;
        lastSign = s;
    }

    return signChanges >= 3;
}

static bool isTriangleLike(const std::vector<cv::Point> &pts)
{
    if (pts.size() < 15)
        return false;
    int corners = countCorners(pts, 40.0);
    double se = startEndDist(pts);
    cv::Rect bb = boundingBox(pts);
    double diag = std::sqrt(bb.width * bb.width + bb.height * bb.height);

    return (corners >= 2 && corners <= 4) && (se < diag * 0.3);
}

static bool isSquareLike(const std::vector<cv::Point> &pts)
{
    if (pts.size() < 20)
        return false;
    int corners = countCorners(pts, 40.0);
    cv::Rect bb = boundingBox(pts);
    double aspect = (bb.height > 0) ? (double)bb.width / (double)bb.height : 0.0;
    double se = startEndDist(pts);
    double diag = std::sqrt(bb.width * bb.width + bb.height * bb.height);

    return (corners >= 3 && corners <= 6) &&
           (aspect > 0.6 && aspect < 1.4) &&
           (se < diag * 0.3);
}

static bool isStarLike(const std::vector<cv::Point> &pts)
{
    if (pts.size() < 25)
        return false;
    int corners = countCorners(pts, 35.0);
    cv::Rect bb = boundingBox(pts);
    double diag = std::sqrt(bb.width * bb.width + bb.height * bb.height);
    double se = startEndDist(pts);

    return (corners >= 6) && (se < diag * 0.4);
}

static bool isSpiralLike(const std::vector<cv::Point> &pts)
{
    if (pts.size() < 25)
        return false;

    cv::Point2f c(0, 0);
    for (auto &p : pts)
        c += cv::Point2f(p);
    c *= (1.0f / pts.size());

    std::vector<double> r;
    r.reserve(pts.size());
    for (auto &p : pts)
        r.push_back(cv::norm(cv::Point2f(p) - c));

    int increasing = 0;
    for (size_t i = 1; i < r.size(); ++i)
        if (r[i] > r[i - 1])
            increasing++;

    double fracInc = (double)increasing / (double)(r.size() - 1);
    int corners = countCorners(pts, 50.0);

    return fracInc > 0.6 && corners <= 3;
}

Spell classifyGesture(const std::vector<cv::Point> &pts, const cv::Size &frameSize)
{
    (void)frameSize; // not used now, but kept for future use

    if (pts.size() < 10)
        return Spell::NONE;

    double L = pathLength(pts);
    if (L < 40.0)
        return Spell::NONE;

    if (isStarLike(pts))
        return Spell::METEOR;

    if (isSquareLike(pts))
        return Spell::SHIELD;

    if (isCircleLike(pts))
        return Spell::FIREBALL;

    if (isTriangleLike(pts))
        return Spell::DRAIN;

    if (isSpiralLike(pts))
        return Spell::HEAL;

    if (isLineLike(pts))
        return Spell::SLASH;

    if (isZigZagLike(pts))
        return Spell::LIGHTNING;

    return Spell::NONE;
}
