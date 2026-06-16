#pragma once
#include <memory>
#include "core/mesh/MeshTypes.h"
#include "core/mesh/IDeformationSolver.h"
#include "core/mesh/IMeshGenerator.h"
#include "core/mesh/MLSSolver.h"
#include "core/mesh/MeshRenderer.h"
#include "core/buffer/PixelBuffer.h"
#include "core/selection/SelectionMask.h"

namespace core {

/// Mesh deformation (puppet-warp) tool session.
/// AppController owns one instance, calls beginSession / commitSession.
/// Pointer events are forwarded from CanvasWidget via AppController.
class MeshDeformTool {
public:
    MeshDeformTool();

    // ── Session lifecycle ────────────────────────────────────────────────────
    /// Start a deformation session.
    /// @param src       Copy of the layer pixels to deform (local to layer).
    /// @param selection Selection mask (pixels outside = pass-through).
    /// @param generator Mesh generator to use.
    /// @param cfg       Mesh density / quality settings.
    void beginSession(core::PixelBuffer            src,
                      core::SelectionMask           selection,
                      const mesh::IMeshGenerator&  generator,
                      const mesh::MeshGenConfig&   cfg);

    /// Discard session without committing.
    void cancelSession() noexcept;

    bool isActive() const noexcept { return m_active; }

    // ── Pin management ───────────────────────────────────────────────────────
    /// Add a pin at local-space position.  Returns the new pin id.
    int  addPin(core::FPoint localPos);

    /// Move pin `id` to a new local-space position and re-solve.
    void movePin(int id, core::FPoint newLocalPos);

    /// Remove pin by id.
    void removePin(int id);

    /// Hit-test: return pin id whose .current is within radiusPx, or -1.
    int  hitTestPin(core::FPoint localPos, float radiusPx = 8.f) const noexcept;

    // ── Pin undo support ─────────────────────────────────────────────────────
    struct PinSnapshot {
        std::vector<mesh::Pin> pins;
        int nextPinId {0};
    };
    /// Capture current pin state for undo.
    PinSnapshot snapshotPins() const noexcept;
    /// Restore a previously captured pin state and re-solve.
    void restorePins(PinSnapshot snap);

    // ── Deformation settings ─────────────────────────────────────────────────
    void               setMode(mesh::DeformMode m) noexcept;
    mesh::DeformMode   mode()                      const noexcept;

    /// Regenerate the mesh (e.g. after density change). Pins are preserved.
    void regenerateMesh(const mesh::IMeshGenerator& generator,
                        const mesh::MeshGenConfig&  cfg);

    // ── Output ───────────────────────────────────────────────────────────────
    /// Fast preview buffer (lower quality, updated after each pin move).
    const core::PixelBuffer& previewBuffer() const noexcept { return m_preview; }

    /// Final high-quality render for commit.
    core::PixelBuffer renderFinal() const;

    /// Mesh data for canvas wireframe overlay.
    const mesh::DeformMesh& mesh()      const noexcept { return m_mesh; }
    const core::SelectionMask& selection() const noexcept { return m_selection; }

    /// Last solved deformed vertex positions (canvas-local coords).
    /// Updated every time solveAndUpdatePreview() is called.
    const std::vector<core::FPoint>& lastDeformedPositions() const noexcept {
        return m_lastDeformedPositions;
    }

private:
    void solveAndUpdatePreview();

    bool                 m_active    {false};
    core::PixelBuffer    m_source;
    core::PixelBuffer    m_preview;
    core::SelectionMask  m_selection;
    mesh::DeformMesh     m_mesh;
    mesh::MLSSolver      m_solver;
    mesh::MeshRenderer   m_previewRenderer {1};
    mesh::MeshRenderer   m_finalRenderer   {2};
    std::vector<core::FPoint> m_lastDeformedPositions;
};

} // namespace core
