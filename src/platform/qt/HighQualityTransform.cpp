#include "platform/qt/HighQualityTransform.h"

#include <cmath>
#include <algorithm>

namespace platform::qt {

namespace {

// Bicubic Convolution Kernel (Catmull-Rom spline, a=-0.5)
inline float bicubicKernel(float x) {
  x = std::abs(x);
  if (x < 1.f) {
    return 1.f - 2.f * x * x + x * x * x;
  }
  if (x < 2.f) {
    return -4.f + 8.f * x - 5.f * x * x + x * x * x;
  }
  return 0.f;
}

// Lanczos3 kernel
inline float lanczos3Kernel(float x) {
  x = std::abs(x);
  if (x < 3.f) {
    const float pi = 3.14159265358979323846f;
    const float sinc = (x < 0.0001f) ? 1.f : std::sin(pi * x) / (pi * x);
    const float window = std::sin(pi * x / 3.f) / (pi * x / 3.f);
    return sinc * window;
  }
  return 0.f;
}

typedef float (*KernelFunc)(float);

struct Pixel {
  float r, g, b, a;
  Pixel() : r(0), g(0), b(0), a(0) {}
  Pixel(float r_, float g_, float b_, float a_) : r(r_), g(g_), b(b_), a(a_) {}
};

Pixel samplePixel(const QImage& img, float x, float y,
                  KernelFunc kernel, int tapCount, float tapRadius) {
  if (img.isNull() || tapCount < 1) {
    return Pixel(0, 0, 0, 0);
  }

  const int w = img.width();
  const int h = img.height();
  const int ix = static_cast<int>(std::floor(x));
  const int iy = static_cast<int>(std::floor(y));
  const float dx = x - ix;
  const float dy = y - iy;

  Pixel result;
  float totalWeight = 0.f;

  for (int jy = -tapRadius + 1; jy <= tapRadius; ++jy) {
    const int py = iy + jy;
    if (py < 0 || py >= h) continue;

    const float wy = kernel(dy - static_cast<float>(jy));

    for (int jx = -tapRadius + 1; jx <= tapRadius; ++jx) {
      const int px = ix + jx;
      if (px < 0 || px >= w) continue;

      const float wx = kernel(dx - static_cast<float>(jx));
      const float weight = wx * wy;
      totalWeight += weight;

      const QColor col(img.pixel(px, py));
      result.r += weight * col.red();
      result.g += weight * col.green();
      result.b += weight * col.blue();
      result.a += weight * col.alpha();
    }
  }

  if (totalWeight > 0.0001f) {
    result.r /= totalWeight;
    result.g /= totalWeight;
    result.b /= totalWeight;
    result.a /= totalWeight;
  }

  return result;
}

HighQualityTransform::TransformResult
transformWithKernel(const QImage& source, const QTransform& transform,
                    KernelFunc kernel, int tapCount, float tapRadius,
                    QColor fillColor) {
  HighQualityTransform::TransformResult result;
  if (source.isNull()) {
    return result;
  }

  const int srcW = source.width();
  const int srcH = source.height();

  // raw bounding rect: 変換後の全域（負座標含む）
  const QRect rawBounds = transform.mapToPolygon(QRect(0, 0, srcW, srcH)).boundingRect();

  // メモリ保護: 出力サイズを srcW*4 × srcH*4 に制限するが、
  // 負座標はクリップしない（off-canvas ピクセルを保持）
  const int dstW = std::clamp(rawBounds.width(),  1, std::max(1, srcW * 4));
  const int dstH = std::clamp(rawBounds.height(), 1, std::max(1, srcH * 4));

  // pixel(0,0) が対応するキャンバス座標 = rawBounds の左上（負座標も保持）
  result.offsetX = rawBounds.x();
  result.offsetY = rawBounds.y();

  result.image = QImage(dstW, dstH, QImage::Format_RGBA8888);
  result.image.fill(fillColor);

  const QTransform invTransform = transform.inverted();

  for (int y = 0; y < dstH; ++y) {
    for (int x = 0; x < dstW; ++x) {
      // dst は変換後座標空間（rawBounds 基準）
      const QPointF dst(rawBounds.x() + x, rawBounds.y() + y);
      const QPointF src = invTransform.map(dst);

      if (src.x() < 0 || src.x() >= srcW || src.y() < 0 || src.y() >= srcH) {
        continue;
      }

      const Pixel p = samplePixel(source, static_cast<float>(src.x()),
                                 static_cast<float>(src.y()),
                                 kernel, tapCount, tapRadius);

      const QColor col(
          static_cast<int>(std::clamp(p.r, 0.f, 255.f)),
          static_cast<int>(std::clamp(p.g, 0.f, 255.f)),
          static_cast<int>(std::clamp(p.b, 0.f, 255.f)),
          static_cast<int>(std::clamp(p.a, 0.f, 255.f)));

      result.image.setPixel(x, y, col.rgba());
    }
  }

  return result;
}

} // namespace

HighQualityTransform::TransformResult HighQualityTransform::transform(
    const QImage& source,
    const QTransform& transform,
    InterpolationMethod method,
    QColor fillColor) {

  TransformResult result;
  if (source.isNull()) {
    return result;
  }

  // Qt 標準の変換で十分な場合（identity）は高速パスを使う
  if (transform.isIdentity()) {
    result.image   = source;
    result.offsetX = 0;
    result.offsetY = 0;
    return result;
  }

  switch (method) {
    case InterpolationMethod::Bilinear: {
      // Qt の SmoothPixmapTransform は bilinear 相当。
      // transformed() は origin を自動調整した画像を返す。
      // trueMatrix() で返却画像の origin を取得して offsetX/Y に保存する。
      const QTransform trueM = QTransform::fromTranslate(0, 0);  // 計算用ダミー
      // Qt の QImage::transformed() は内部で trueMatrix を使い
      // pixel(0,0) が変換後の bounding rect 左上に来るよう調整する。
      // その左上座標を rawBounds.left/top で算出する。
      const int srcW = source.width();
      const int srcH = source.height();
      const QRect rawBounds = transform.mapToPolygon(QRect(0,0,srcW,srcH)).boundingRect();
      result.image   = source.transformed(transform, Qt::SmoothTransformation);
      // transformed() は rawBounds の全域をカバーする（負座標も含む）
      result.offsetX = rawBounds.left();
      result.offsetY = rawBounds.top();
      return result;
    }

    case InterpolationMethod::Bicubic:
      return transformWithKernel(source, transform, bicubicKernel, 4, 2.f, fillColor);

    case InterpolationMethod::Lanczos3:
      return transformWithKernel(source, transform, lanczos3Kernel, 6, 3.f, fillColor);
  }

  return result;
}

} // namespace platform::qt
