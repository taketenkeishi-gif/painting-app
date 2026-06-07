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

QImage transformWithKernel(const QImage& source, const QTransform& transform,
                          KernelFunc kernel, int tapCount, float tapRadius,
                          QColor fillColor) {
  if (source.isNull()) {
    return source;
  }

  const int srcW = source.width();
  const int srcH = source.height();

  QRect boundingRect = transform.mapToPolygon(QRect(0, 0, srcW, srcH)).boundingRect();
  boundingRect = boundingRect.intersected(
      QRect(0, 0, std::max(1, srcW * 4), std::max(1, srcH * 4)));

  const int dstW = std::max(1, boundingRect.width());
  const int dstH = std::max(1, boundingRect.height());

  QImage result(dstW, dstH, QImage::Format_RGBA8888);
  result.fill(fillColor);

  const QTransform invTransform = transform.inverted();

  for (int y = 0; y < dstH; ++y) {
    for (int x = 0; x < dstW; ++x) {
      const QPointF dst(boundingRect.x() + x, boundingRect.y() + y);
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

      result.setPixel(x, y, col.rgba());
    }
  }

  return result;
}

} // namespace

QImage HighQualityTransform::transform(const QImage& source,
                                       const QTransform& transform,
                                       InterpolationMethod method,
                                       QColor fillColor) {
  if (source.isNull()) {
    return source;
  }

  // Qt 標準の変換で十分な場合（identity, 90度回転等）は高速パスを使う
  if (transform.isIdentity()) {
    return source;
  }

  switch (method) {
    case InterpolationMethod::Bilinear:
      // Qt の SmoothPixmapTransform は bilinear 相当なので、元々の方法でOK
      return source.transformed(transform, Qt::SmoothTransformation);

    case InterpolationMethod::Bicubic:
      return transformWithKernel(source, transform, bicubicKernel, 4, 2.f, fillColor);

    case InterpolationMethod::Lanczos3:
      return transformWithKernel(source, transform, lanczos3Kernel, 6, 3.f, fillColor);
  }

  return source;
}

} // namespace platform::qt
