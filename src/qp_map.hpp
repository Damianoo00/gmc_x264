#pragma once
#include <cstdint>
#include <vector>

namespace gmc {

struct QpMap {
    int mb_width;                 // liczba makrobloków w poziomie
    int mb_height;                // liczba makrobloków w pionie
    std::vector<int16_t> offsets; // offset QP per makroblok (indeks = y * mb_width + x)
};

class QpMapBuilder {
public:
    QpMapBuilder();

    // Tworzy mapę QP dla obrazu o wymiarach width x height,
    // dla ruchu globalnego (dx, dy) w pikselach.
    QpMap build(int width, int height, double dx, double dy) const;
};

} // namespace gmc