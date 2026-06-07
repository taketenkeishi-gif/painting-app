#pragma once

#include <QImage>
#include <QTransform>

namespace platform::qt {

// 高品質な画像変換（Lanczos3 補間相当）
class HighQualityTransform {
public:
  enum class InterpolationMethod {
    Bilinear,   // 2x2 カーネル（速い）
    Bicubic,    // 4x4 カーネル（バランス型）
    Lanczos3,   // 6x6 カーネル（高品質、遅い）
  };

  static QImage transform(
      const QImage& source,
      const QTransform& transform,
      InterpolationMethod method = InterpolationMethod::Bicubic,
      QColor fillColor = Qt::transparent);
};

} // namespace platform::qt
