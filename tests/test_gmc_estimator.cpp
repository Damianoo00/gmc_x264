#include <gtest/gtest.h>
#include "gmc_estimator.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

using namespace gmc;

// ========== Fixture ==========
class GmcEstimatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        estimator_ = GmcEstimator{};
    }
    GmcEstimator estimator_;
};

// ========== Testy błędów ==========
TEST_F(GmcEstimatorTest, ReturnsErrorForEmptyFrame) {
    cv::Mat empty;
    cv::Mat valid(100, 100, CV_8UC3);

    // pierwsza pusta
    auto res1 = estimator_.estimate(empty, valid);
    ASSERT_FALSE(res1.has_value());
    EXPECT_EQ(res1.error(), EstimatorError::EmptyFrame);

    // druga pusta
    auto res2 = estimator_.estimate(valid, empty);
    ASSERT_FALSE(res2.has_value());
    EXPECT_EQ(res2.error(), EstimatorError::EmptyFrame);
}

TEST_F(GmcEstimatorTest, ReturnsErrorForSizeMismatch) {
    cv::Mat a(100, 100, CV_8UC3);
    cv::Mat b(200, 100, CV_8UC3);
    auto res = estimator_.estimate(a, b);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), EstimatorError::SizeMismatch);
}

// ========== Testy poprawności działania ==========
TEST_F(GmcEstimatorTest, EstimatesZeroMotion) {
    // Dwie identyczne syntetyczne klatki
    cv::Mat frame(480, 640, CV_8UC3, cv::Scalar(60, 180, 80)); // zielony
    auto res = estimator_.estimate(frame, frame);
    // Oczekujemy wyniku, nie błędu
    ASSERT_TRUE(res.has_value());
    EXPECT_NEAR(res->dx, 0.0, 0.5);
    EXPECT_NEAR(res->dy, 0.0, 0.5);
    EXPECT_NEAR(res->angle_rad, 0.0, 1e-3);
    EXPECT_NEAR(res->scale, 1.0, 1e-3);
}

TEST_F(GmcEstimatorTest, DetectsKnownTranslation) {
    // klatka z wyraźnymi cechami (np. szachownica)
    cv::Mat pattern(480, 640, CV_8UC1);
    for (int y = 0; y < 480; ++y)
        for (int x = 0; x < 640; ++x)
            pattern.at<uchar>(y, x) = ((x/20 + y/20) % 2) ? 255 : 0;
    cv::Mat colorPattern;
    cv::cvtColor(pattern, colorPattern, cv::COLOR_GRAY2BGR);

    // przesuwamy obraz o znany wektor: dx=10, dy=5
    cv::Mat shifted;
    cv::Mat M = (cv::Mat_<double>(2,3) << 1, 0, 10, 0, 1, 5);
    cv::warpAffine(colorPattern, shifted, M, colorPattern.size());

    auto res = estimator_.estimate(colorPattern, shifted);
    ASSERT_TRUE(res.has_value());
    EXPECT_NEAR(res->dx, 10.0, 1.0);
    EXPECT_NEAR(res->dy, 5.0, 1.0);
}

TEST_F(GmcEstimatorTest, UsesMaskToFocusOnRegion) {
    // Szachownica 480x640
    cv::Mat pattern(480, 640, CV_8UC1);
    for (int y = 0; y < 480; ++y)
        for (int x = 0; x < 640; ++x)
            pattern.at<uchar>(y, x) = ((x/20 + y/20) % 2) ? 255 : 0;
    cv::Mat colorPattern;
    cv::cvtColor(pattern, colorPattern, cv::COLOR_GRAY2BGR);

    // Przesunięcie w prawo o 5px
    cv::Mat shifted;
    cv::Mat M = (cv::Mat_<double>(2,3) << 1, 0, 5, 0, 1, 0);
    cv::warpAffine(colorPattern, shifted, M, colorPattern.size());

    // Maska tylko lewy górny prostokąt 200x150 (wystarczająco duży i teksturowany)
    cv::Mat mask = cv::Mat::zeros(pattern.size(), CV_8UC1);
    mask(cv::Rect(0, 0, 200, 150)) = 255;

    auto res = estimator_.estimate(colorPattern, shifted, mask);
    ASSERT_TRUE(res.has_value());
    EXPECT_NEAR(res->dx, 5.0, 1.0);
    EXPECT_NEAR(res->dy, 0.0, 1.0);
}