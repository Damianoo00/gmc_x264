#pragma once
#include <opencv2/core.hpp>
#include <cstdint>

/**
 * Konwertuje obraz YUV I420 (trzy płaszczyzny) do BGR (OpenCV Mat).
 * y, u, v – wskaźniki na dane płaszczyzn.
 * y_stride, u_stride, v_stride – kroki odpowiednich płaszczyzn.
 * width, height – rozmiar obrazu w pikselach (szer. i wys. płaszczyzny Y).
 */
cv::Mat i420_to_bgr(const uint8_t* y, int y_stride,
                    const uint8_t* u, int u_stride,
                    const uint8_t* v, int v_stride,
                    int width, int height);