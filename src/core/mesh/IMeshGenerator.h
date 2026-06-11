#pragma once
#include "core/mesh/MeshTypes.h"
#include "core/buffer/PixelBuffer.h"
#include "core/selection/SelectionMask.h"

namespace core::mesh {

/// Abstract mesh generator.
/// Produces a triangulated DeformMesh within the selection region.
/// Boundary vertices are marked fixed; pin list is empty after generation.
class IMeshGenerator {
public:
    virtual ~IMeshGenerator() = default;

    /// Generate a mesh for the given source buffer + selection.
    virtual DeformMesh generate(const core::PixelBuffer&    src,
                                 const core::SelectionMask& selection,
                                 const MeshGenConfig&       cfg) const = 0;

    virtual const char* name() const noexcept = 0;
};

} // namespace core::mesh
