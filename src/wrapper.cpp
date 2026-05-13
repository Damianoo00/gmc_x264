// wrapper.cpp
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <x264.h>
#include <dlfcn.h>
#include <cstdio>
#include <opencv2/core.hpp>

#include "yuv_converter.hpp"
#include "grass_detector.hpp"
#include "gmc_estimator.hpp"
#include "qp_map.hpp"

static int (*real_x264_encoder_encode)(x264_t *, x264_nal_t **, int *,
                                       x264_picture_t *, x264_picture_t *) = nullptr;

static bool gmc_enabled = true;
static int frame_count = 0;
static cv::Mat prev_bgr;
static std::vector<float> qp_offsets_buffer;

static void free_qp_offsets(void* opaque) {
    (void)opaque;
}

__attribute__((constructor))
static void init() {
    real_x264_encoder_encode =
        (decltype(real_x264_encoder_encode))dlsym(RTLD_NEXT, "x264_encoder_encode");
    
    const char* env_val = getenv("GMC_ENABLED");
    if (env_val && (env_val[0] == '0' || env_val[0] == 'n' || env_val[0] == 'N')) {
        gmc_enabled = false;
    }
    
    fprintf(stderr, "[GMC] Wrapper loaded. GMC_ENABLED=%s\n", gmc_enabled ? "1" : "0");
}

extern "C"
int x264_encoder_encode(x264_t *h,
                        x264_nal_t **pp_nal, int *pi_nal,
                        x264_picture_t *pic_in,
                        x264_picture_t *pic_out)
{
    if (!real_x264_encoder_encode) return -1;
    if (!pic_in)
        return real_x264_encoder_encode(h, pp_nal, pi_nal, pic_in, pic_out);

    if (!gmc_enabled) {
        return real_x264_encoder_encode(h, pp_nal, pi_nal, pic_in, pic_out);
    }

    if (pic_in->img.i_csp == X264_CSP_I420 && pic_in->img.i_plane > 0) {
        x264_param_t param;
        x264_encoder_parameters(h, &param);
        int width  = param.i_width;
        int height = param.i_height;

        cv::Mat cur_bgr = i420_to_bgr(pic_in->img.plane[0], pic_in->img.i_stride[0],
                                      pic_in->img.plane[1], pic_in->img.i_stride[1],
                                      pic_in->img.plane[2], pic_in->img.i_stride[2],
                                      width, height);

        if (!prev_bgr.empty() && prev_bgr.size() == cur_bgr.size()) {
            frame_count++;
            
            gmc::GrassDetector grass;
            auto grass_res = grass.detect(prev_bgr);
            cv::Mat mask;
            bool has_grass = grass_res.has_value();
            if (has_grass)
                mask = *grass_res;

            gmc::GmcEstimator estimator;
            auto motion = estimator.estimate(prev_bgr, cur_bgr, mask);
            
            if (has_grass && motion.has_value()) {
                fprintf(stderr, "[GMC] Frame %d: motion dx=%.2f dy=%.2f\n", 
                        frame_count, motion->dx, motion->dy);
                
                gmc::QpMapBuilder qp_builder;
                gmc::QpMap qp_map = qp_builder.build(width, height,
                                                      motion->dx, motion->dy);
                
                int mb_count = qp_map.mb_width * qp_map.mb_height;
                if (!qp_map.offsets.empty() && mb_count > 0) {
                    if (!pic_in->prop.quant_offsets) {
                        qp_offsets_buffer.resize(mb_count, 0.0f);
                        pic_in->prop.quant_offsets = qp_offsets_buffer.data();
                        pic_in->prop.quant_offsets_free = free_qp_offsets;
                        fprintf(stderr, "[GMC] Allocated QP offsets buffer (%d MBs)\n", mb_count);
                    }
                    
                    int applied_count = 0;
                    for (int i = 0; i < mb_count && static_cast<size_t>(i) < qp_map.offsets.size(); ++i) {
                        if (qp_map.offsets[i] != 0) {
                            pic_in->prop.quant_offsets[i] = static_cast<float>(qp_map.offsets[i]);
                            applied_count++;
                        }
                    }
                    
                    if (applied_count > 0) {
                        fprintf(stderr, "[GMC] Applied %d QP offsets (out of %d)\n", 
                                applied_count, mb_count);
                    }
                }
            } else {
                fprintf(stderr, "[GMC] Frame %d: no motion (%s, %s)\n", 
                        frame_count, 
                        has_grass ? "grass" : "no_grass",
                        motion.has_value() ? "motion" : "no_motion");
            }
        }
        prev_bgr = cur_bgr.clone();
    }

    return real_x264_encoder_encode(h, pp_nal, pi_nal, pic_in, pic_out);
}