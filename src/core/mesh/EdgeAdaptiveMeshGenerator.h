#pragma once
#include "core/mesh/IMeshGenerator.h"

namespace core::mesh {

/// Edge-adaptive mesh generator.
/// Runs Sobel edge detection on the source image, inserts extra vertices in
/// high-gradient regions, then triangulates with Bowyer-Watson Delaunay.
/// Produces denser triangles near object boundaries for higher-quality deformation.
class EdgeAdaptiveMeshGenerator final : public IMeshGenerator {
public:
    DeformMesh  generate(const core::PixelBuffer&    src,
                          const core::SelectionMask& selection,
                          const MeshGenConfig&       cfg) const override;
    const char* name() const noexcept override { return "EdgeAdaptive"; }

private:
    /// Returns Sobel gradient magnitude [0, 255] at pixel (x, y).
    static float sobelGradient(const core::PixelBuffer& src, int x, int y) noexcept;
};

} // namespace core::mesh
