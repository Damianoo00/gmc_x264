#include <gtest/gtest.h>
#include "grass_detector.hpp"
#include <opencv2/core.hpp>

using namespace gmc;

class GrassDetectorTest : public ::testing::Test {
protected:
void SetUp() override {
        detector_ = GrassDetector{};  // domyślny konstruktor = default_green()
    }

    GrassDetector detector_;
};

// ============================================================
// TESTY BŁĘDÓW — najpierw przypadki brzegowe
// ============================================================

// Używamy fixture — TEST_F zamiast TEST
TEST_F(GrassDetectorTest, ReturnsErrorForEmptyFrame) {
    cv::Mat empty;  // domyślny konstruktor Mat = pusty

    auto result = detector_.detect(empty);

    // ASSERT — zatrzymuje test jeśli nie przechodzi
    // (w przeciwieństwie do EXPECT które kontynuuje)
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), GrassDetectorError::EmptyFrame);
}

TEST_F(GrassDetectorTest, ReturnsErrorForWrongColorSpace) {
    // Obraz w skali szarości (1 kanał) zamiast BGR (3 kanały)
    cv::Mat gray(100, 100, CV_8UC1, cv::Scalar(128));

    auto result = detector_.detect(gray);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), GrassDetectorError::WrongColorSpace);
}

// ============================================================
// TESTY POPRAWNEGO DZIAŁANIA
// ============================================================

TEST_F(GrassDetectorTest, DetectsFullyGreenImage) {
    // Syntetyczny obraz — BGR (nie RGB!)
    // cv::Scalar(B, G, R) — OpenCV używa BGR
    // Zielony w BGR: B=60, G=180, R=80
    cv::Mat green(100, 100, CV_8UC3, cv::Scalar(60, 180, 80));

    auto result = detector_.detect(green);

    ASSERT_TRUE(result.has_value());

    // Prawie cały obraz powinien być wykryty jako trawa
    double total   = 100.0 * 100.0;
    double nonzero = cv::countNonZero(*result);
    EXPECT_GT(nonzero / total, 0.90);  // >90% pikseli = murawa
}

TEST_F(GrassDetectorTest, IgnoresRedImage) {
    // Czerwony obraz — B=0, G=0, R=200
    cv::Mat red(100, 100, CV_8UC3, cv::Scalar(0, 0, 200));

    auto result = detector_.detect(red);

    // Albo błąd InsufficientGrass, albo bardzo mała maska
    if (result.has_value()) {
        double nonzero = cv::countNonZero(*result);
        EXPECT_LT(nonzero / (100.0 * 100.0), 0.05);
    } else {
        EXPECT_EQ(result.error(), GrassDetectorError::InsufficientGrass);
    }
}

TEST_F(GrassDetectorTest, MaskHasSameSizeAsInput) {
    cv::Mat green(480, 640, CV_8UC3, cv::Scalar(60, 180, 80));

    auto result = detector_.detect(green);

    ASSERT_TRUE(result.has_value());
    // Maska musi mieć dokładnie ten sam rozmiar co wejście
    EXPECT_EQ(result->rows, green.rows);
    EXPECT_EQ(result->cols, green.cols);
    // I być jednokanałowa (binary mask)
    EXPECT_EQ(result->channels(), 1);
}

// ============================================================
// TESTY KONFIGURACJI — różne presety HsvRange
// ============================================================

// Tu nie używamy fixture — tworzymy detector ręcznie
TEST(GrassDetectorConfig, ArtificialLightDetectsYellowerGrass) {
    GrassDetector day_detector{HsvRange::default_green()};
    GrassDetector night_detector{HsvRange::artificial_light()};

    // Żółtawa murawa przy reflektorach — H przesunięte w stronę żółci
    // W HSV: H≈30 (żółty), S=160, V=100 → w BGR to ok. (30, 150, 180)
    cv::Mat yellowish(100, 100, CV_8UC3, cv::Scalar(30, 150, 180));

    auto day_result   = day_detector.detect(yellowish);
    auto night_result = night_detector.detect(yellowish);

    int day_pixels = day_result.has_value()
                     ? cv::countNonZero(*day_result) : 0;
    int night_pixels = night_result.has_value()
                       ? cv::countNonZero(*night_result) : 0;

    // Preset nocny powinien wykryć więcej żółtawej trawy
    EXPECT_GT(night_pixels, day_pixels);
}