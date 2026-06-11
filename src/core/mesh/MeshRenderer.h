#pragma once
#include "core/mesh/MeshTypes.h"
#include "core/buffer/PixelBuffer.h"
#include "core/selection/SelectionMask.h"
#include "core/color/Color.h"

namespace core::mesh {

/// High-quality mesh-deformation renderer.
///
/// For each output pixel in a deformed triangle, computes the source
/// location via barycentric interpolation and samples it with Catmull-Rom
/// bicubic filtering.  Pixels outside the selection are passed through
/// unchanged.
class MeshRenderer {
public:
    /// @param ssaa  Supersampling factor per axis (1=off, 2=2x2 default).
    explicit MeshRenderer(int ssaa = 2);

    /// Render deformation result.
    /// @param src   Original (pre-deformation) layer pixels.
    /// @param dst   Output buffer (same size as src — may be a fresh buffer).
    /// @param sel   Selection mask.
    /// @param mesh  Mesh with vertex.deformed filled by a solver.
    void render(const core::PixelBuffer&    src,
                core::PixelBuffer&          dst,
                const core::SelectionMask&  sel,
                const DeformMesh&           mesh) const;

    void setSuperSampleFactor(int n) noexcept { m_ssaa = (n < 1 ? 1 : n); }
    int  superSampleFactor()         const noexcept { return m_ssaa; }

private:
    /// Catmull-Rom cubic kernel
    static float catmullRom(float t) noexcept;
    /// Bicubic sample at fractional (sx, sy) — clamps to buffer bounds
    static core::Color sampleBicubic(const core::PixelBuffer& src,
                                      float sx, float sy) noexcept;
    /// Render one triangle (with optional SSAA)
    void renderTriangle(const core::PixelBuffer&   src,
                        core::PixelBuffer&          dst,
                        const core::SelectionMask&  sel,
                        const MeshVertex&           va,
                        const MeshVertex&           vb,
                        const MeshVertex&           vc) const;

    int m_ssaa {2};
};

} // namespace core::mesh
