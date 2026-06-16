#pragma once
#include "core/mesh/MeshTypes.h"
#include <vector>

namespace core::mesh {

/// Bowyer-Watson incremental Delaunay triangulation.
/// Input:  flat list of 2-D points + parallel fixed-flag array.
/// Output: filled DeformMesh — vertex.deformed == vertex.original, no pins.
class TriangulationBuilder {
public:
    /// @param points  Vertex positions in local/buffer space.
    /// @param fixed   Same size as points; true = boundary/immovable vertex.
    static DeformMesh triangulate(const std::vector<core::FPoint>& points,
                                   const std::vector<bool>&         fixed);

private:
    static bool inCircumcircle(core::FPoint a, core::FPoint b,
                                core::FPoint c, core::FPoint p) noexcept;
};

} // namespace core::mesh
