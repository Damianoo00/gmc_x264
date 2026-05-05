#pragma once
#include <expected>
#include <opencv2/core.hpp>

namespace gmc {

struct HsvRange {
    int h_min, h_max;
    int s_min, s_max;
    int v_min, v_max;
    static HsvRange default_green();
    static HsvRange artificial_light();
    static HsvRange dry_grass();
};

enum class GrassDetectorError {
    EmptyFrame,
    WrongColorSpace,
    InsufficientGrass,
};

class GrassDetector {
public:
    explicit GrassDetector(HsvRange range = HsvRange::default_green());
    std::expected<cv::Mat, GrassDetectorError> detect(const cv::Mat& frame) const;
private:
    HsvRange range_;
};

} // namespace gmc