#pragma once
#include <vector>
#include "core/common/FPoint.h"

namespace core::mesh {

struct Pin {
    int          id       {-1};
    core::FPoint original {};       ///< local-space position at pin placement
    core::FPoint current  {};       ///< local-space position after drag
    bool         fixed    {false};  ///< boundary pin — cannot be moved
};

struct MeshVertex {
    core::FPoint original {};       ///< local-space position at mesh generation
    core::FPoint deformed {};       ///< local-space position after solver pass
    bool         fixed    {false};  ///< selection boundary — not deformed
};

struct MeshTriangle {
    int v[3] {0, 0, 0};            ///< vertex indices into DeformMesh::vertices
};

struct DeformMesh {
    std::vector<MeshVertex>   vertices;
    std::vector<MeshTriangle> triangles;
    std::vector<Pin>          pins;
    int nextPinId {0};
    /// Bounding region in layer-buffer space (top-left corner + size)
    int regionX {0}, regionY {0}, regionW {0}, regionH {0};

    void clearPins() noexcept { pins.clear(); nextPinId = 0; }
    void reset()     noexcept { vertices.clear(); triangles.clear(); pins.clear(); nextPinId = 0; }
    bool hasTriangles() const noexcept { return !triangles.empty(); }
};

/// Deformation algorithm mode
enum class DeformMode {
    Similarity,   ///< MLS similarity — rotation + uniform scale
    Rigid,        ///< MLS rigid     — rotation only, no scale
};

/// Mesh generator configuration
struct MeshGenConfig {
    int   gridRows       {16};    ///< vertex rows (manual density)
    int   gridCols       {16};    ///< vertex columns
    bool  adaptiveEdges  {true};  ///< insert extra verts near image edges
    float edgeThreshold  {30.f};  ///< Sobel gradient threshold for adaptive mode
};

} // namespace core::mesh
