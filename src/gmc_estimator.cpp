#include "gmc_estimator.hpp"
#include <opencv2/video.hpp>        // calcOpticalFlowFarneback
#include <opencv2/imgproc.hpp>      // cvtColor, COLOR_BGR2GRAY
#include <vector>
#include <algorithm>

namespace gmc {

GmcEstimator::GmcEstimator() {}

std::expected<GmcResult, EstimatorError>
GmcEstimator::estimate(const cv::Mat& frame1,
                       const cv::Mat& frame2,
                       const cv::Mat& mask) const
{
    // ---------- walidacja ----------
    if (frame1.empty() || frame2.empty())
        return std::unexpected(EstimatorError::EmptyFrame);
    if (frame1.size() != frame2.size())
        return std::unexpected(EstimatorError::SizeMismatch);

    // ---------- konwersja na skalę szarości ----------
    cv::Mat gray1, gray2;
    if (frame1.channels() == 1) {
        gray1 = frame1;
        gray2 = frame2;
    } else {
        cv::cvtColor(frame1, gray1, cv::COLOR_BGR2GRAY);
        cv::cvtColor(frame2, gray2, cv::COLOR_BGR2GRAY);
    }

    // ---------- optical flow ----------
    cv::Mat flow;
    cv::calcOpticalFlowFarneback(gray1, gray2, flow,
        0.5,    // pyr_scale
        3,      // levels
        15,     // winsize
        3,      // iterations
        5,      // poly_n
        1.2,    // poly_sigma
        0       // flags
    );

    // ---------- zebranie wektorów z uwzględnieniem maski ----------
    std::vector<double> dxs, dys;
    dxs.reserve(flow.rows * flow.cols);
    dys.reserve(flow.rows * flow.cols);

    for (int y = 0; y < flow.rows; ++y) {
        for (int x = 0; x < flow.cols; ++x) {
            if (!mask.empty() && mask.at<uchar>(y, x) == 0)
                continue;   // pomijamy piksele poza maską

            const cv::Vec2f& f = flow.at<cv::Vec2f>(y, x);
            dxs.push_back(f[0]);
            dys.push_back(f[1]);
        }
    }

    // ---------- sprawdzenie minimalnej liczby wektorów ----------
    if (dxs.size() < 20)   // arbitralny próg
        return std::unexpected(EstimatorError::InsufficientFeatures);

    // ---------- mediana (odporna na outliery) ----------
    auto median = [](std::vector<double>& v) -> double {
        size_t n = v.size();
        auto mid = v.begin() + n / 2;
        std::nth_element(v.begin(), mid, v.end());
        double med = *mid;
        if (n % 2 == 0) {
            auto mid2 = v.begin() + n / 2 - 1;
            std::nth_element(v.begin(), mid2, v.end());
            med = (med + *mid2) / 2.0;
        }
        return med;
    };

    GmcResult res{};
    res.dx        = median(dxs);
    res.dy        = median(dys);
    res.angle_rad = 0.0;   // jeszcze nie szacujemy obrotu
    res.scale     = 1.0;   // i skali

    return res;
}

} // namespace gmc