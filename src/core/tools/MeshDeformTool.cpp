#include "core/tools/MeshDeformTool.h"
#include "core/mesh/GridMeshGenerator.h"
#include <cmath>
#include <algorithm>

namespace core {

// ── Constructor ──────────────────────────────────────────────────────────────

MeshDeformTool::MeshDeformTool()
    : m_active(false)
{}

// ── Session lifecycle ────────────────────────────────────────────────────────

void MeshDeformTool::beginSession(core::PixelBuffer           src,
                                   core::SelectionMask          selection,
                                   const mesh::IMeshGenerator&  generator,
                                   const mesh::MeshGenConfig&   cfg)
{
    m_source    = std::move(src);
    m_selection = std::move(selection);

    m_mesh = generator.generate(m_source, m_selection, cfg);
    m_mesh.clearPins();   // freshly generated mesh has no pins

    // Initialise preview as an exact copy of the source
    m_preview = m_source;

    m_active = true;

    // Populate lastDeformedPositions with identity (original positions)
    // so the wireframe overlay is immediately visible.
    solveAndUpdatePreview();
}

void MeshDeformTool::cancelSession() noexcept
{
    m_active = false;
    m_mesh.reset();
}

// ── Pin management ───────────────────────────────────────────────────────────

int MeshDeformTool::addPin(core::FPoint localPos)
{
    mesh::Pin pin;
    pin.id       = m_mesh.nextPinId++;
    pin.original = localPos;
    pin.current  = localPos;
    pin.fixed    = false;

    m_mesh.pins.push_back(pin);

    solveAndUpdatePreview();
    return pin.id;
}

void MeshDeformTool::movePin(int id, core::FPoint newLocalPos)
{
    auto it = std::find_if(m_mesh.pins.begin(), m_mesh.pins.end(),
                           [id](const mesh::Pin& p){ return p.id == id; });
    if (it == m_mesh.pins.end())
        return;

    it->current = newLocalPos;
    solveAndUpdatePreview();
}

void MeshDeformTool::removePin(int id)
{
    auto it = std::find_if(m_mesh.pins.begin(), m_mesh.pins.end(),
                           [id](const mesh::Pin& p){ return p.id == id; });
    if (it == m_mesh.pins.end())
        return;

    m_mesh.pins.erase(it);
    solveAndUpdatePreview();
}

int MeshDeformTool::hitTestPin(core::FPoint localPos, float radiusPx) const noexcept
{
    for (const mesh::Pin& p : m_mesh.pins) {
        float dx = p.current.x - localPos.x;
        float dy = p.current.y - localPos.y;
        if (std::sqrt(dx * dx + dy * dy) < radiusPx)
            return p.id;
    }
    return -1;
}

// ── Deformation settings ─────────────────────────────────────────────────────

void MeshDeformTool::setMode(mesh::DeformMode m) noexcept
{
    m_solver.setMode(m);
    if (m_active)
        solveAndUpdatePreview();
}

mesh::DeformMode MeshDeformTool::mode() const noexcept
{
    return m_solver.mode();
}

void MeshDeformTool::regenerateMesh(const mesh::IMeshGenerator& generator,
                                     const mesh::MeshGenConfig&  cfg)
{
    // Save existing pins before regeneration
    std::vector<mesh::Pin> savedPins = m_mesh.pins;

    // Regenerate mesh geometry
    m_mesh = generator.generate(m_source, m_selection, cfg);

    // Restore saved pins — re-add all of them with their original IDs and
    // positions.  Pins that happened to land outside the new mesh bounds will
    // still be preserved; the solver handles out-of-mesh pins gracefully.
    m_mesh.pins    = std::move(savedPins);
    // Keep nextPinId consistent (must be > every existing id)
    if (!m_mesh.pins.empty()) {
        int maxId = -1;
        for (const mesh::Pin& p : m_mesh.pins)
            if (p.id > maxId) maxId = p.id;
        m_mesh.nextPinId = maxId + 1;
    }

    solveAndUpdatePreview();
}

// ── Output ───────────────────────────────────────────────────────────────────

core::PixelBuffer MeshDeformTool::renderFinal() const
{
    core::PixelBuffer dst(m_source.width(), m_source.height());

    if (m_mesh.pins.empty()) {
        // No deformation — return a plain copy of the source
        dst = m_source;
        return dst;
    }

    // Solve deformation on a mutable copy of the mesh
    mesh::DeformMesh meshCopy = m_mesh;
    m_solver.solve(meshCopy);

    m_finalRenderer.render(m_source, dst, m_selection, meshCopy);
    return dst;
}

// ── Private helpers ──────────────────────────────────────────────────────────

void MeshDeformTool::solveAndUpdatePreview()
{
    if (m_mesh.pins.empty()) {
        m_preview = m_source;
        // Cache identity positions
        m_lastDeformedPositions.resize(m_mesh.vertices.size());
        for (std::size_t i = 0; i < m_mesh.vertices.size(); ++i)
            m_lastDeformedPositions[i] = m_mesh.vertices[i].original;
        return;
    }

    mesh::DeformMesh meshCopy = m_mesh;
    m_solver.solve(meshCopy);

    // Cache solved deformed positions for wireframe overlay
    m_lastDeformedPositions.resize(meshCopy.vertices.size());
    for (std::size_t i = 0; i < meshCopy.vertices.size(); ++i)
        m_lastDeformedPositions[i] = meshCopy.vertices[i].deformed;

    m_preview.resize(m_source.width(), m_source.height());
    m_previewRenderer.render(m_source, m_preview, m_selection, meshCopy);
}

MeshDeformTool::PinSnapshot MeshDeformTool::snapshotPins() const noexcept
{
    return { m_mesh.pins, m_mesh.nextPinId };
}

void MeshDeformTool::restorePins(PinSnapshot snap)
{
    m_mesh.pins      = std::move(snap.pins);
    m_mesh.nextPinId = snap.nextPinId;
    solveAndUpdatePreview();
}

} // namespace core
