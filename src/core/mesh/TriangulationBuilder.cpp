#include "core/mesh/TriangulationBuilder.h"
#include <cmath>
#include <algorithm>
#include <vector>

namespace core::mesh {

// ---------------------------------------------------------------------------
// inCircumcircle
// Standard 3x3 determinant test.  Returns true when p lies strictly inside
// the circumcircle of counter-clockwise triangle (a, b, c).
// ---------------------------------------------------------------------------
bool TriangulationBuilder::inCircumcircle(core::FPoint a, core::FPoint b,
                                           core::FPoint c, core::FPoint p) noexcept
{
    const float ax = a.x - p.x;
    const float ay = a.y - p.y;
    const float bx = b.x - p.x;
    const float by = b.y - p.y;
    const float cx = c.x - p.x;
    const float cy = c.y - p.y;

    const float bsq = bx * bx + by * by;
    const float csq = cx * cx + cy * cy;
    const float asq = ax * ax + ay * ay;

    const float det =
          ax * (by * csq - cy * bsq)
        - ay * (bx * csq - cx * bsq)
        + asq * (bx * cy  - by * cx);

    return det > 0.0f;
}

// ---------------------------------------------------------------------------
// triangulate  — Bowyer-Watson incremental Delaunay
// ---------------------------------------------------------------------------
DeformMesh TriangulationBuilder::triangulate(const std::vector<core::FPoint>& points,
                                              const std::vector<bool>&         fixed)
{
    const int N = static_cast<int>(points.size());

    // Super-triangle vertex indices are N, N+1, N+2.
    // We work with a combined point set: [0..N-1] = input, [N..N+2] = super verts.
    // Only indices are stored in triangles; FPoint values are built on demand.

    // Build super-triangle large enough to contain all input points.
    float minX = points[0].x, maxX = points[0].x;
    float minY = points[0].y, maxY = points[0].y;
    for (const auto& p : points) {
        if (p.x < minX) minX = p.x;
        if (p.x > maxX) maxX = p.x;
        if (p.y < minY) minY = p.y;
        if (p.y > maxY) maxY = p.y;
    }
    const float dx   = maxX - minX;
    const float dy   = maxY - minY;
    const float dmax = (dx > dy ? dx : dy) * 10.0f + 1.0f;
    const float midX = (minX + maxX) * 0.5f;
    const float midY = (minY + maxY) * 0.5f;

    // Super-triangle vertices stored separately for lookup.
    core::FPoint sv[3];
    sv[0] = { midX - 20.0f * dmax, midY - dmax };
    sv[1] = { midX,                 midY + 20.0f * dmax };
    sv[2] = { midX + 20.0f * dmax, midY - dmax };

    // Helper: resolve index to FPoint (input or super-vertex).
    auto pt = [&](int idx) -> core::FPoint {
        return (idx < N) ? points[idx] : sv[idx - N];
    };

    // Triangle list: array of {v0, v1, v2}.
    struct Tri { int v[3]; };
    std::vector<Tri> tris;
    tris.reserve(N * 3 + 8);
    tris.push_back({ {N, N + 1, N + 2} });

    // Edge: pair of vertex indices (unordered for boundary detection).
    struct Edge { int a, b; };

    std::vector<bool> badMask;
    std::vector<Edge> poly;

    for (int i = 0; i < N; ++i) {
        const core::FPoint pi = points[i];

        // --- find bad triangles ---
        badMask.assign(tris.size(), false);
        for (int t = 0; t < static_cast<int>(tris.size()); ++t) {
            const Tri& tri = tris[t];
            if (inCircumcircle(pt(tri.v[0]), pt(tri.v[1]), pt(tri.v[2]), pi)) {
                badMask[t] = true;
            }
        }

        // --- find boundary edges (appear exactly once across bad triangles) ---
        poly.clear();
        for (int t = 0; t < static_cast<int>(tris.size()); ++t) {
            if (!badMask[t]) continue;
            const Tri& tri = tris[t];
            for (int e = 0; e < 3; ++e) {
                const int ea = tri.v[e];
                const int eb = tri.v[(e + 1) % 3];
                // Check whether this edge is shared with another bad triangle.
                bool shared = false;
                for (int t2 = 0; t2 < static_cast<int>(tris.size()); ++t2) {
                    if (t2 == t || !badMask[t2]) continue;
                    const Tri& tri2 = tris[t2];
                    for (int e2 = 0; e2 < 3; ++e2) {
                        const int ea2 = tri2.v[e2];
                        const int eb2 = tri2.v[(e2 + 1) % 3];
                        if ((ea == ea2 && eb == eb2) || (ea == eb2 && eb == ea2)) {
                            shared = true;
                            break;
                        }
                    }
                    if (shared) break;
                }
                if (!shared) {
                    poly.push_back({ ea, eb });
                }
            }
        }

        // --- remove bad triangles ---
        {
            int write = 0;
            for (int t = 0; t < static_cast<int>(tris.size()); ++t) {
                if (!badMask[t]) tris[write++] = tris[t];
            }
            tris.resize(write);
        }

        // --- create new triangles from boundary to pi ---
        for (const Edge& e : poly) {
            tris.push_back({ {e.a, e.b, i} });
        }
    }

    // --- remove triangles that share a vertex with the super-triangle ---
    {
        int write = 0;
        for (int t = 0; t < static_cast<int>(tris.size()); ++t) {
            const Tri& tri = tris[t];
            if (tri.v[0] >= N || tri.v[1] >= N || tri.v[2] >= N) continue;
            tris[write++] = tri;
        }
        tris.resize(write);
    }

    // --- build DeformMesh ---
    DeformMesh mesh;
    mesh.vertices.resize(N);
    for (int i = 0; i < N; ++i) {
        mesh.vertices[i].original = points[i];
        mesh.vertices[i].deformed = points[i];
        mesh.vertices[i].fixed    = (i < static_cast<int>(fixed.size())) ? fixed[i] : false;
    }
    mesh.triangles.resize(tris.size());
    for (int t = 0; t < static_cast<int>(tris.size()); ++t) {
        mesh.triangles[t].v[0] = tris[t].v[0];
        mesh.triangles[t].v[1] = tris[t].v[1];
        mesh.triangles[t].v[2] = tris[t].v[2];
    }

    return mesh;
}

} // namespace core::mesh
