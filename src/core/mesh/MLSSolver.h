#pragma once
#include "core/mesh/IDeformationSolver.h"

namespace core::mesh {

/// Moving-Least-Squares deformation solver.
/// Reference: Schaefer et al. 2006 "Image Deformation Using Moving Least Squares"
///
/// Similarity mode: rotation + uniform scale (Photoshop-style Warp)
/// Rigid mode:      rotation only, no scale  (AE-style Puppet)
class MLSSolver final : public IDeformationSolver {
public:
    explicit MLSSolver(DeformMode mode = DeformMode::Similarity);

    void        solve(DeformMesh& mesh) const override;
    const char* name()                  const noexcept override;

    void       setMode(DeformMode m) noexcept { m_mode  = m; }
    DeformMode mode()                const noexcept { return m_mode; }

    /// Weight falloff exponent alpha (paper default 1.0).
    void  setAlpha(float a) noexcept { m_alpha = a; }
    float alpha()           const noexcept { return m_alpha; }

private:
    core::FPoint solvePoint(const core::FPoint& v, const DeformMesh& mesh) const;

    DeformMode m_mode  {DeformMode::Similarity};
    float      m_alpha {1.f};
};

} // namespace core::mesh
