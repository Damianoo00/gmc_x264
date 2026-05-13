#include "qp_map.hpp"
#include <cmath>

namespace gmc {

QpMapBuilder::QpMapBuilder() {}

QpMap QpMapBuilder::build(int width, int height, double dx, double dy) const {
    const int mb_size = 16;
    int mb_width  = (width  + mb_size - 1) / mb_size;
    int mb_height = (height + mb_size - 1) / mb_size;

    QpMap map;
    map.mb_width  = mb_width;
    map.mb_height = mb_height;
    map.offsets.reserve(static_cast<size_t>(mb_width) * mb_height);

    double motion_magnitude = std::sqrt(dx * dx + dy * dy);
    
    for (int my = 0; my < mb_height; ++my) {
        for (int mx = 0; mx < mb_width; ++mx) {
            double cx = mx * mb_size + mb_size / 2.0;
            double cy = my * mb_size + mb_size / 2.0;
            double new_cx = cx + dx;
            double new_cy = cy + dy;

            int16_t offset = 0;
            
            if (new_cx < 0 || new_cx >= width || new_cy < 0 || new_cy >= height) {
                offset = 3;
            } else if (motion_magnitude > 0.05) {
                int qp_boost = static_cast<int>(motion_magnitude * 50);
                if (qp_boost > 6) qp_boost = 6;
                if (qp_boost > 0) {
                    offset = static_cast<int16_t>(qp_boost);
                }
            }
            
            map.offsets.push_back(offset);
        }
    }
    return map;
}

} // namespace gmc