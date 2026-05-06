#include <gtest/gtest.h>
#include "yuv_converter.hpp"
#include <opencv2/imgproc.hpp>
#include <vector>

TEST(YuvConverter, ConvertsGrayImage) {
    // Y = 128, U = 128, V = 128 -> szary BGR (128,128,128)
    const int w = 4, h = 4;
    std::vector<uint8_t> y_plane(w * h, 128);
    std::vector<uint8_t> u_plane(w/2 * h/2, 128);
    std::vector<uint8_t> v_plane(w/2 * h/2, 128);
    cv::Mat bgr = i420_to_bgr(y_plane.data(), w,
                              u_plane.data(), w/2,
                              v_plane.data(), w/2,
                              w, h);
    ASSERT_EQ(bgr.rows, h);
    ASSERT_EQ(bgr.cols, w);
    // Wszystkie piksele szare
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            cv::Vec3b px = bgr.at<cv::Vec3b>(y, x);
            EXPECT_NEAR(px[0], 128, 2); // B
            EXPECT_NEAR(px[1], 128, 2); // G
            EXPECT_NEAR(px[2], 128, 2); // R
        }
    }
}

TEST(YuvConverter, ConvertsRedImage) {
    // Czerwony BGR = (0,0,255). W YUV (BT.601): Y≈76, U≈84, V≈255
    const int w = 4, h = 4;
    std::vector<uint8_t> y_plane(w * h, 76);
    std::vector<uint8_t> u_plane(w/2 * h/2, 84);
    std::vector<uint8_t> v_plane(w/2 * h/2, 255);
    cv::Mat bgr = i420_to_bgr(y_plane.data(), w,
                              u_plane.data(), w/2,
                              v_plane.data(), w/2,
                              w, h);
    // Wszystkie piksele powinny być czerwone
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            cv::Vec3b px = bgr.at<cv::Vec3b>(y, x);
            EXPECT_NEAR(px[0], 0, 15);   // B
            EXPECT_NEAR(px[1], 0, 15);   // G
            EXPECT_NEAR(px[2], 255, 15); // R
        }
    }
}

TEST(YuvConverter, HandlesStridedPlanes) {
    // Y z paddingiem (stride > width)
    const int w = 4, h = 4;
    const int y_stride = 8;   // dodatkowe 4 bajty po każdym wierszu
    std::vector<uint8_t> y_plane(y_stride * h, 128);
    // wypełniamy co stride bajtów wartością 128, reszta śmieci
    for (int r = 0; r < h; ++r)
        std::fill_n(y_plane.begin() + r * y_stride, w, 128);

    std::vector<uint8_t> u_plane(w/2 * h/2, 128);
    std::vector<uint8_t> v_plane(w/2 * h/2, 128);

    cv::Mat bgr = i420_to_bgr(y_plane.data(), y_stride,
                              u_plane.data(), w/2,
                              v_plane.data(), w/2,
                              w, h);
    cv::Vec3b px = bgr.at<cv::Vec3b>(0, 0);
    EXPECT_NEAR(px[0], 128, 2);
    EXPECT_NEAR(px[1], 128, 2);
    EXPECT_NEAR(px[2], 128, 2);
}