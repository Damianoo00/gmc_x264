#include "grass_detector.hpp"

#include <opencv2/imgproc.hpp>
#include <algorithm>

namespace gmc {
// ── HsvRange dla zielonej trawy w naturalnym świetle (dzień) ─────────────
HsvRange HsvRange::default_green() {
    // H: od zieleni lekko żółtawej do zieleni niebieskawej
    // S: średnio nasycona i więcej
    // V: niezbyt ciemna
    return HsvRange{
        .h_min = 35, .h_max = 85,
        .s_min = 50, .s_max = 255,
        .v_min = 50, .v_max = 255
    };
}

// ── HsvRange dla meczu przy sztucznym oświetleniu (reflektory) ───────────
HsvRange HsvRange::artificial_light() {
    // h_min obniżamy do 20, aby objąć typową żółtawą trawę przy reflektorach
    return HsvRange{
        .h_min = 20, .h_max = 85,
        .s_min = 30, .s_max = 255,
        .v_min = 40, .v_max = 255
    };
}

// ── HsvRange dla wyschniętej, letniej murawy ─────────────────────────────
HsvRange HsvRange::dry_grass() {
    // Suche trawy są bardziej żółte/brązowe → niższe H
    // i mają mniejszą saturację
    return HsvRange{
        .h_min = 20, .h_max = 45,
        .s_min = 30, .s_max = 200,
        .v_min = 40, .v_max = 220
    };
}

// ── GrassDetector ─────────────────────────────────────────────────────────
GrassDetector::GrassDetector(HsvRange range)
    : range_(range) {}

std::expected<cv::Mat, GrassDetectorError>
GrassDetector::detect(const cv::Mat& frame) const {
    // 1. Walidacja wejścia
    if (frame.empty()) {
        return std::unexpected(GrassDetectorError::EmptyFrame);
    }
    if (frame.channels() != 3) {
        return std::unexpected(GrassDetectorError::WrongColorSpace);
    }

    // 2. Konwersja BGR → HSV
    cv::Mat hsv;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

    // 3. Binaryzacja na podstawie zakresu HSV
    cv::Mat grass_mask;
    cv::inRange(hsv,
                cv::Scalar(range_.h_min, range_.s_min, range_.v_min),
                cv::Scalar(range_.h_max, range_.s_max, range_.v_max),
                grass_mask);

    // 4. Sprawdzenie, czy jest wystarczająco dużo "zielonych" pikseli
    const double total_pixels = static_cast<double>(frame.rows * frame.cols);
    const double grass_pixels = static_cast<double>(cv::countNonZero(grass_mask));
    const double grass_ratio  = grass_pixels / total_pixels;

    if (grass_ratio < 0.05) {   // mniej niż 5% – brak murawy
        return std::unexpected(GrassDetectorError::InsufficientGrass);
    }

    return grass_mask;
}
}