#include <gtest/gtest.h>
#include "qp_map.hpp"
#include <numeric>

using namespace gmc;

class QpMapBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        builder_ = QpMapBuilder{};
    }
    QpMapBuilder builder_;
};

TEST_F(QpMapBuilderTest, BuildsCorrectNumberOfBlocks) {
    // 32x16 -> 2 bloki szerokości, 1 wysokości
    QpMap map = builder_.build(32, 16, 0.0, 0.0);
    EXPECT_EQ(map.mb_width, 2);
    EXPECT_EQ(map.mb_height, 1);
    EXPECT_EQ(map.offsets.size(), 2u);
}

TEST_F(QpMapBuilderTest, AllZerosForZeroMotion) {
    QpMap map = builder_.build(64, 64, 0.0, 0.0);
    int16_t sum = std::accumulate(map.offsets.begin(), map.offsets.end(), static_cast<uint16_t>(0));
    EXPECT_EQ(sum, 0);
}

TEST_F(QpMapBuilderTest, SomeOffsetsPositiveForLargeMotion) {
    // 64x64, przesunięcie (30,30) – skrajne bloki wyjdą poza obraz
    QpMap map = builder_.build(64, 64, 30.0, 30.0);
    int16_t sum = std::accumulate(map.offsets.begin(), map.offsets.end(), static_cast<uint16_t>(0));
    EXPECT_GT(sum, 0);
}

TEST_F(QpMapBuilderTest, OutOfBoundsBlockGetsOffset) {
    // 64x64, przesunięcie (+50, 0) – blok najbardziej na prawo (indeks 3)
    QpMap map = builder_.build(64, 64, 50.0, 0.0);
    // Ostatni blok w dolnym rzędzie (3,3) powinien mieć offset >0
    EXPECT_GT(map.offsets.back(), 0);
}