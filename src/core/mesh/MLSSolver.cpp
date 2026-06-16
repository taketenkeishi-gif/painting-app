#include "core/mesh/MLSSolver.h"
#include <cmath>

namespace core::mesh {

MLSSolver::MLSSolver(DeformMode mode)
    : m_mode(mode)
{}

const char* MLSSolver::name() const noexcept
{
    return m_mode == DeformMode::Rigid ? "MLS-Rigid" : "MLS-Similarity";
}

// ---------------------------------------------------------------------------
// solvePoint — MLS deformation for a single query point v
// Reference: Schaefer et al. 2006, Section 2.1 (similarity) and 2.2 (rigid)
// ---------------------------------------------------------------------------
core::FPoint MLSSolver::solvePoint(const core::FPoint& v, const DeformMesh& mesh) const
{
    const auto& pins = mesh.pins;
    const float alpha = m_alpha;

    // --- compute weights w_i = 1 / dist^(2*alpha) ---
    // and check for near-coincident pin (snap to it immediately)
    std::vector<float> w(pins.size());
    for (std::size_t i = 0; i < pins.size(); ++i) {
        float dx = v.x - pins[i].original.x;
        float dy = v.y - pins[i].original.y;
        float dist2 = dx * dx + dy * dy;
        float dist  = std::sqrt(dist2);

        // snap: if v is effectively on top of a pin, return its current pos
        if (dist < 0.5f) {
            return pins[i].current;
        }

        // w_i = 1 / dist^(2*alpha)
        // For alpha == 1 this simplifies to 1/dist2, but we support general alpha.
        float denom = std::pow(std::max(dist2, 0.0001f), alpha);
        w[i] = 1.0f / denom;
    }

    // --- p* : weighted centroid of pin originals ---
    float wSum = 0.0f;
    float pStarX = 0.0f, pStarY = 0.0f;
    float qStarX = 0.0f, qStarY = 0.0f;
    for (std::size_t i = 0; i < pins.size(); ++i) {
        wSum    += w[i];
        pStarX  += w[i] * pins[i].original.x;
        pStarY  += w[i] * pins[i].original.y;
        qStarX  += w[i] * pins[i].current.x;
        qStarY  += w[i] * pins[i].current.y;
    }
    pStarX /= wSum;
    pStarY /= wSum;
    qStarX /= wSum;
    qStarY /= wSum;

    // --- mu_s = sum( w_i * |p_hat_i|^2 ) ---
    // --- complex accumulator cr + i*ci = sum( w_i * conj(p_hat_i) * q_hat_i ) ---
    // conj(a,b) * (c,d) = (a*c + b*d) + i*(a*d - b*c)
    float mu_s = 0.0f;
    float cr   = 0.0f;
    float ci   = 0.0f;
    for (std::size_t i = 0; i < pins.size(); ++i) {
        float phx = pins[i].original.x - pStarX;
        float phy = pins[i].original.y - pStarY;
        float qhx = pins[i].current.x  - qStarX;
        float qhy = pins[i].current.y  - qStarY;

        mu_s += w[i] * (phx * phx + phy * phy);

        // conj(ph) * qh where ph = (phx, phy), conj flips imaginary sign
        cr   += w[i] * ( phx * qhx + phy * qhy);
        ci   += w[i] * ( phx * qhy - phy * qhx);
    }

    // --- v_hat = v - p* ---
    float vhx = v.x - pStarX;
    float vhy = v.y - pStarY;

    core::FPoint result;

    if (m_mode == DeformMode::Similarity) {
        // f(v) = (v_hat * M) / mu_s  +  q*
        // where M = [[cr, ci],[-ci, cr]]  applied as complex multiply:
        // result = (vhx * cr - vhy * ci, vhx * ci + vhy * cr) / mu_s + q*
        if (std::fabs(mu_s) < 1e-8f) {
            result = {qStarX, qStarY};
        } else {
            result.x = (vhx * cr - vhy * ci) / mu_s + qStarX;
            result.y = (vhx * ci + vhy * cr) / mu_s + qStarY;
        }
    } else {
        // Rigid: normalize (cr, ci) to unit length, then multiply by |v_hat|
        float vhLen = std::hypot(vhx, vhy);
        float cLen  = std::hypot(cr, ci);

        if (cLen < 1e-8f || vhLen < 1e-8f) {
            result = {qStarX, qStarY};
        } else {
            // unit rotation matrix column: (cr/cLen, ci/cLen)
            float crN = cr / cLen;
            float ciN = ci / cLen;

            // rotate v_hat by the estimated rotation, scale to |v_hat|
            result.x = (vhx * crN - vhy * ciN) * vhLen / std::hypot(vhx, vhy) + qStarX;
            result.y = (vhx * ciN + vhy * crN) * vhLen / std::hypot(vhx, vhy) + qStarY;

            // Simplify: the scale factor vhLen / hypot(vhx,vhy) == 1 since
            // vhLen = hypot(vhx, vhy).  Write it cleanly:
            result.x = vhx * crN - vhy * ciN + qStarX;
            result.y = vhx * ciN + vhy * crN + qStarY;
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// solve — apply MLS to all non-fixed mesh vertices
// ---------------------------------------------------------------------------
void MLSSolver::solve(DeformMesh& mesh) const
{
    const auto& pins = mesh.pins;

    // Degenerate: no pins — leave every vertex at original position
    if (pins.empty()) {
        for (auto& vtx : mesh.vertices) {
            vtx.deformed = vtx.original;
        }
        return;
    }

    // Single pin: translate the whole mesh by the pin's displacement
    if (pins.size() == 1) {
        float dx = pins[0].current.x - pins[0].original.x;
        float dy = pins[0].current.y - pins[0].original.y;
        for (auto& vtx : mesh.vertices) {
            if (vtx.fixed) {
                vtx.deformed = vtx.original;
            } else {
                vtx.deformed.x = vtx.original.x + dx;
                vtx.deformed.y = vtx.original.y + dy;
            }
        }
        return;
    }

    // General case: run MLS per vertex
    for (auto& vtx : mesh.vertices) {
        if (vtx.fixed) {
            vtx.deformed = vtx.original;
        } else {
            vtx.deformed = solvePoint(vtx.original, mesh);
        }
    }
}

} // namespace core::mesh
