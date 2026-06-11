#include "core/mesh/MeshRenderer.h"
#include <cmath>
#include <algorithm>

namespace core::mesh {

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

MeshRenderer::MeshRenderer(int ssaa)
    : m_ssaa(ssaa < 1 ? 1 : ssaa)
{}

// ---------------------------------------------------------------------------
// Catmull-Rom cubic kernel
// ---------------------------------------------------------------------------

float MeshRenderer::catmullRom(float t) noexcept
{
    t = std::abs(t);
    if (t < 1.0f)
        return 1.5f * t * t * t - 2.5f * t * t + 1.0f;
    if (t < 2.0f)
        return -0.5f * t * t * t + 2.5f * t * t - 4.0f * t + 2.0f;
    return 0.0f;
}

// ---------------------------------------------------------------------------
// Bicubic sample
// ---------------------------------------------------------------------------

core::Color MeshRenderer::sampleBicubic(const core::PixelBuffer& src,
                                         float sx, float sy) noexcept
{
    const int w = src.width();
    const int h = src.height();

    const int ix = static_cast<int>(std::floor(sx));
    const int iy = static_cast<int>(std::floor(sy));
    const float fx = sx - static_cast<float>(ix);
    const float fy = sy - static_cast<float>(iy);

    float sumR = 0.0f, sumG = 0.0f, sumB = 0.0f, sumA = 0.0f, sumW = 0.0f;

    for (int dy = -1; dy <= 2; ++dy) {
        for (int dx = -1; dx <= 2; ++dx) {
            const int px = std::clamp(ix + dx, 0, w - 1);
            const int py = std::clamp(iy + dy, 0, h - 1);
            const float kx = catmullRom(static_cast<float>(dx) - fx);
            const float ky = catmullRom(static_cast<float>(dy) - fy);
            const float w_  = kx * ky;

            const core::Color c = src.pixel(px, py);
            sumR += w_ * static_cast<float>(c.r);
            sumG += w_ * static_cast<float>(c.g);
            sumB += w_ * static_cast<float>(c.b);
            sumA += w_ * static_cast<float>(c.a);
            sumW += w_;
        }
    }

    if (sumW < 1e-6f)
        return core::Color::Transparent();

    const auto clamp8 = [](float v) -> uint8_t {
        return static_cast<uint8_t>(std::clamp(v, 0.0f, 255.0f));
    };

    return core::Color{
        clamp8(sumR / sumW),
        clamp8(sumG / sumW),
        clamp8(sumB / sumW),
        clamp8(sumA / sumW)
    };
}

// ---------------------------------------------------------------------------
// Render one triangle
// ---------------------------------------------------------------------------

void MeshRenderer::renderTriangle(const core::PixelBuffer&  src,
                                   core::PixelBuffer&         dst,
                                   const core::SelectionMask& sel,
                                   const MeshVertex&          va,
                                   const MeshVertex&          vb,
                                   const MeshVertex&          vc) const
{
    // Bounding box of deformed triangle, clamped to dst dimensions
    const int dstW = dst.width();
    const int dstH = dst.height();

    const float minXf = std::min({va.deformed.x, vb.deformed.x, vc.deformed.x});
    const float minYf = std::min({va.deformed.y, vb.deformed.y, vc.deformed.y});
    const float maxXf = std::max({va.deformed.x, vb.deformed.x, vc.deformed.x});
    const float maxYf = std::max({va.deformed.y, vb.deformed.y, vc.deformed.y});

    const int x0 = std::clamp(static_cast<int>(std::floor(minXf)), 0, dstW - 1);
    const int y0 = std::clamp(static_cast<int>(std::floor(minYf)), 0, dstH - 1);
    const int x1 = std::clamp(static_cast<int>(std::floor(maxXf)), 0, dstW - 1);
    const int y1 = std::clamp(static_cast<int>(std::floor(maxYf)), 0, dstH - 1);

    // Deformed triangle edges for barycentric computation
    const float e0x = vb.deformed.x - va.deformed.x;
    const float e0y = vb.deformed.y - va.deformed.y;
    const float e1x = vc.deformed.x - va.deformed.x;
    const float e1y = vc.deformed.y - va.deformed.y;

    // Signed area * 2 of the deformed triangle
    const float d = e0x * e1y - e0y * e1x;
    if (std::abs(d) < 1e-6f)
        return;

    const float invSSAA = 1.0f / static_cast<float>(m_ssaa);
    const float sampleCount = static_cast<float>(m_ssaa * m_ssaa);

    for (int py = y0; py <= y1; ++py) {
        for (int px = x0; px <= x1; ++px) {
            float accR = 0.0f, accG = 0.0f, accB = 0.0f, accA = 0.0f;
            int   hitCount = 0;

            for (int sj = 0; sj < m_ssaa; ++sj) {
                for (int si = 0; si < m_ssaa; ++si) {
                    const float spx = static_cast<float>(px) + (static_cast<float>(si) + 0.5f) * invSSAA;
                    const float spy = static_cast<float>(py) + (static_cast<float>(sj) + 0.5f) * invSSAA;

                    const float dx_ = spx - va.deformed.x;
                    const float dy_ = spy - va.deformed.y;

                    const float u = (dx_ * e1y - dy_ * e1x) / d;
                    const float v = (e0x * dy_ - e0y * dx_) / d;
                    const float w = 1.0f - u - v;

                    if (u < -1e-4f || v < -1e-4f || w < -1e-4f)
                        continue;

                    const float srcX = u * vb.original.x + v * vc.original.x + w * va.original.x;
                    const float srcY = u * vb.original.y + v * vc.original.y + w * va.original.y;

                    const core::Color c = sampleBicubic(src, srcX, srcY);
                    accR += static_cast<float>(c.r);
                    accG += static_cast<float>(c.g);
                    accB += static_cast<float>(c.b);
                    accA += static_cast<float>(c.a);
                    ++hitCount;
                }
            }

            if (hitCount == 0)
                continue;

            if (sel.hasSelection() && !sel.contains(px, py))
                continue;

            const float inv = 1.0f / static_cast<float>(hitCount);
            const auto clamp8 = [](float v) -> uint8_t {
                return static_cast<uint8_t>(std::clamp(v, 0.0f, 255.0f));
            };

            dst.setPixel(px, py, core::Color{
                clamp8(accR * inv),
                clamp8(accG * inv),
                clamp8(accB * inv),
                clamp8(accA * inv)
            });
        }
    }
}

// ---------------------------------------------------------------------------
// Public render
// ---------------------------------------------------------------------------

void MeshRenderer::render(const core::PixelBuffer&   src,
                           core::PixelBuffer&          dst,
                           const core::SelectionMask&  sel,
                           const DeformMesh&            mesh) const
{
    // Copy src to dst (pass-through for unaffected pixels)
    const int w = src.width();
    const int h = src.height();
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            dst.setPixel(x, y, src.pixel(x, y));

    // Render each deformed triangle
    for (const MeshTriangle& t : mesh.triangles) {
        renderTriangle(src, dst, sel,
                       mesh.vertices[t.v[0]],
                       mesh.vertices[t.v[1]],
                       mesh.vertices[t.v[2]]);
    }
}

} // namespace core::mesh
