#pragma once
#include "core/mesh/IMeshGenerator.h"

namespace core::mesh {

/// Uniform-grid mesh generator (default).
/// Divides the selection bounding rectangle into a regular grid and
/// triangulates with 2 triangles per cell.
/// Vertices outside the selection or on its boundary are marked fixed.
class GridMeshGenerator final : public IMeshGenerator {
public:
    DeformMesh  generate(const core::PixelBuffer&    src,
                          const core::SelectionMask& selection,
                          const MeshGenConfig&       cfg) const override;
    const char* name() const noexcept override { return "Grid"; }
};

} // namespace core::mesh
