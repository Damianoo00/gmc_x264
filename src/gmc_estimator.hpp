#pragma once
#include <expected>
#include <opencv2/core.hpp>

namespace gmc {

// ------------------- struktura wyniku -------------------
struct GmcResult {
    double dx;           // przesunięcie poziome w pikselach (frame2 - frame1)
    double dy;           // przesunięcie pionowe
    double angle_rad;   // obrót w radianach (przeciwnie do ruchu wskazówek zegara)
    double scale;       // zmiana skali (1.0 = brak zmiany)
};

// ------------------- enum błędów -------------------
enum class EstimatorError {
    EmptyFrame,            // jedna z klatek jest pusta
    SizeMismatch,          // klatki mają różne rozmiary
    InsufficientFeatures,  // za mało punktów do estymacji
    InternalError          // błąd wewnętrzny
};

// ------------------- klasa GmcEstimator -------------------
class GmcEstimator {
public:
    // Konfiguracja opcjonalna (na razie domyślna)
    explicit GmcEstimator();

    // Estymacja ruchu pomiędzy frame1 a frame2.
    // maska – opcjonalna, piksele >0 będą brane pod uwagę
    std::expected<GmcResult, EstimatorError>
    estimate(const cv::Mat& frame1,
             const cv::Mat& frame2,
             const cv::Mat& mask = cv::Mat()) const;
};

} // namespace gmc