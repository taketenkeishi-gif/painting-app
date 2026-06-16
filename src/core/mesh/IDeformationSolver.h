#pragma once
#include "core/mesh/MeshTypes.h"

namespace core::mesh {

/// Abstract deformation solver.
/// Given a DeformMesh with pins set, updates every non-fixed vertex's
/// .deformed position.  Implementations should be stateless.
class IDeformationSolver {
public:
    virtual ~IDeformationSolver() = default;

    /// Solve deformed positions for all non-fixed vertices.
    /// @param mesh  Modified in-place: writes vertex.deformed.
    virtual void solve(DeformMesh& mesh) const = 0;

    /// Short identifier shown in UI mode selector.
    virtual const char* name() const noexcept = 0;
};

} // namespace core::mesh
