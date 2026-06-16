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

  // 変換結果。image.pixel(0,0) は変換後座標 (offsetX, offsetY) に対応する。
  // Bilinear は Qt::transformed() を使うため Qt が origin を自動調整する。
  // Bicubic/Lanczos3 は clipping 後の raw boundingRect 左上を offsetX/Y に保存する。
  struct TransformResult {
    QImage image;
    int offsetX = 0;  // image.pixel(0,0) の変換後 X 座標
    int offsetY = 0;  // image.pixel(0,0) の変換後 Y 座標
  };

  static TransformResult transform(
      const QImage& source,
      const QTransform& transform,
      InterpolationMethod method = InterpolationMethod::Bicubic,
      QColor fillColor = Qt::transparent);
};

} // namespace platform::qt
