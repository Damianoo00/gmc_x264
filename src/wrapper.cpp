// wrapper.cpp
#include <cstdint>               // konieczne przed x264.h
#include <x264.h>
#include <dlfcn.h>
#include <cstdio>
#include <opencv2/core.hpp>

#include "yuv_converter.hpp"
#include "grass_detector.hpp"
#include "gmc_estimator.hpp"
#include "qp_map.hpp"

// ── oryginalna funkcja ──────────────────────────────────────
static int (*real_x264_encoder_encode)(x264_t *, x264_nal_t **, int *,
                                       x264_picture_t *, x264_picture_t *) = nullptr;

__attribute__((constructor))
static void init() {
    real_x264_encoder_encode =
        (decltype(real_x264_encoder_encode))dlsym(RTLD_NEXT, "x264_encoder_encode");
    if (!real_x264_encoder_encode)
        fprintf(stderr, "GMC wrapper: nie znaleziono x264_encoder_encode!\n");
}

// ── pamięć poprzedniej klatki ───────────────────────────────
static cv::Mat prev_bgr;

// ── przechwycenie x264_encoder_encode ────────────────────────
extern "C"
int x264_encoder_encode(x264_t *h,
                        x264_nal_t **pp_nal, int *pi_nal,
                        x264_picture_t *pic_in,
                        x264_picture_t *pic_out)
{
    if (!real_x264_encoder_encode) return -1;
    if (!pic_in)
        return real_x264_encoder_encode(h, pp_nal, pi_nal, pic_in, pic_out);

    // działamy tylko na I420
    if (pic_in->img.i_csp == X264_CSP_I420 && pic_in->img.i_plane > 0) {
        // Pobieramy wymiary obrazu z parametrów enkodera
        x264_param_t param;
        x264_encoder_parameters(h, &param);
        int width  = param.i_width;
        int height = param.i_height;

        // konwersja za pomocą naszego konwertera (używa plane)
        cv::Mat cur_bgr = i420_to_bgr(pic_in->img.plane[0], pic_in->img.i_stride[0],
                                      pic_in->img.plane[1], pic_in->img.i_stride[1],
                                      pic_in->img.plane[2], pic_in->img.i_stride[2],
                                      width, height);

        // analiza tylko gdy mamy już poprzednią klatkę
        if (!prev_bgr.empty() && prev_bgr.size() == cur_bgr.size()) {
            // 1. Maska murawy
            gmc::GrassDetector grass;
            auto grass_res = grass.detect(prev_bgr);
            cv::Mat mask;
            if (grass_res.has_value())
                mask = *grass_res;

            // 2. Estymacja ruchu
            gmc::GmcEstimator estimator;
            auto motion = estimator.estimate(prev_bgr, cur_bgr, mask);
            if (motion.has_value()) {
                // 3. Mapa QP
                gmc::QpMapBuilder qp_builder;
                gmc::QpMap qp_map = qp_builder.build(width, height,
                                                      motion->dx, motion->dy);
                // 4. Wstrzyknięcie offsetów do enkodera
                if (pic_in->prop.quant_offsets && !qp_map.offsets.empty()) {
                    for (size_t i = 0; i < qp_map.offsets.size(); ++i)
                        pic_in->prop.quant_offsets[i] =
                            static_cast<uint8_t>(qp_map.offsets[i]);
                }
            }
        }
        // zapamiętaj bieżącą klatkę
        prev_bgr = cur_bgr.clone();
    }

    // wywołaj prawdziwy enkoder
    return real_x264_encoder_encode(h, pp_nal, pi_nal, pic_in, pic_out);
}