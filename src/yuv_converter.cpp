#include "yuv_converter.hpp"
#include <opencv2/imgproc.hpp>

cv::Mat i420_to_bgr(const uint8_t* y, int y_stride,
                    const uint8_t* u, int u_stride,
                    const uint8_t* v, int v_stride,
                    int width, int height)
{
    cv::Mat y_plane(height, width, CV_8UC1);
    cv::Mat uv_plane(height/2, width/2, CV_8UC2);
    
    for (int r = 0; r < height; ++r) {
        memcpy(y_plane.ptr<uchar>(r), y + r * y_stride, width);
    }
    
    for (int r = 0; r < height/2; ++r) {
        uchar* uv_row = uv_plane.ptr<uchar>(r);
        for (int c = 0; c < width/2; ++c) {
            uv_row[c*2] = u[r * u_stride + c];
            uv_row[c*2 + 1] = v[r * v_stride + c];
        }
    }
    
    cv::Mat bgr;
    cv::cvtColorTwoPlane(y_plane, uv_plane, bgr, cv::COLOR_YUV2BGR_NV12);
    return bgr;
}
