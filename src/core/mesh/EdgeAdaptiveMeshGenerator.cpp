#include "core/mesh/EdgeAdaptiveMeshGenerator.h"
#include "core/mesh/TriangulationBuilder.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace core::mesh {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/// Returns the luminance of a pixel, using 0 for out-of-bounds coordinates.
static float luminanceAt(const core::PixelBuffer& src, int x, int y) noexcept
{
    if (x < 0 || y < 0 || x >= src.width() || y >= src.height())
        return 0.0f;
    const core::Color c = src.pixel(x, y);
    return 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
}

/// Returns true when two FPoints are closer than `eps` in both axes.
static bool nearlyEqual(const core::FPoint& a, const core::FPoint& b,
                        float eps = 0.5f) noexcept
{
    return std::abs(a.x - b.x) < eps && std::abs(a.y - b.y) < eps;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// sobelGradient
// ---------------------------------------------------------------------------

float EdgeAdaptiveMeshGenerator::sobelGradient(const core::PixelBuffer& src,
                                                int x, int y) noexcept
{
    // 3×3 Sobel kernels
    //  Gx: [-1  0  1]      Gy: [-1 -2 -1]
    //      [-2  0  2]           [ 0  0  0]
    //      [-1  0  1]           [ 1  2  1]

    const float gx =
        -1.0f * luminanceAt(src, x - 1, y - 1) +
         0.0f * luminanceAt(src, x,     y - 1) +
         1.0f * luminanceAt(src, x + 1, y - 1) +
        -2.0f * luminanceAt(src, x - 1, y    ) +
         0.0f * luminanceAt(src, x,     y    ) +
         2.0f * luminanceAt(src, x + 1, y    ) +
        -1.0f * luminanceAt(src, x - 1, y + 1) +
         0.0f * luminanceAt(src, x,     y + 1) +
         1.0f * luminanceAt(src, x + 1, y + 1);

    const float gy =
        -1.0f * luminanceAt(src, x - 1, y - 1) +
        -2.0f * luminanceAt(src, x,     y - 1) +
        -1.0f * luminanceAt(src, x + 1, y - 1) +
         0.0f * luminanceAt(src, x - 1, y    ) +
         0.0f * luminanceAt(src, x,     y    ) +
         0.0f * luminanceAt(src, x + 1, y    ) +
         1.0f * luminanceAt(src, x - 1, y + 1) +
         2.0f * luminanceAt(src, x,     y + 1) +
         1.0f * luminanceAt(src, x + 1, y + 1);

    const float magnitude = std::sqrt(gx * gx + gy * gy);
    return std::max(0.0f, std::min(255.0f, magnitude));
}

// ---------------------------------------------------------------------------
// generate
// ---------------------------------------------------------------------------

DeformMesh EdgeAdaptiveMeshGenerator::generate(const core::PixelBuffer&    src,
                                                const core::SelectionMask& selection,
                                                const MeshGenConfig&       cfg) const
{
    // ------------------------------------------------------------------
    // 1. Bounding rectangle — same logic as GridMeshGenerator.
    //    Use the selection bounding rect when available; fall back to the
    //    full buffer dimensions.
    // ------------------------------------------------------------------
    int rx, ry, rw, rh;
    if (selection.hasSelection()) {
        const auto optRect = selection.boundingRect();
        if (optRect.has_value()) {
            rx = optRect->x;
            ry = optRect->y;
            rw = optRect->width;
            rh = optRect->height;
        } else {
            rx = 0; ry = 0;
            rw = src.width();
            rh = src.height();
        }
    } else {
        rx = 0; ry = 0;
        rw = src.width();
        rh = src.height();
    }

    // Guard against degenerate regions.
    if (rw <= 0 || rh <= 0) {
        DeformMesh empty;
        empty.regionX = rx; empty.regionY = ry;
        empty.regionW = rw; empty.regionH = rh;
        return empty;
    }

    const int rows = std::max(1, cfg.gridRows);
    const int cols = std::max(1, cfg.gridCols);

    // ------------------------------------------------------------------
    // 2. Base grid: (cols+1) x (rows+1) points.
    //    Positions are in layer-buffer space (absolute pixel coordinates).
    // ------------------------------------------------------------------
    std::vector<core::FPoint> points;
    points.reserve((rows + 1) * (cols + 1));

    const float cellW = static_cast<float>(rw) / static_cast<float>(cols);
    const float cellH = static_cast<float>(rh) / static_cast<float>(rows);

    for (int row = 0; row <= rows; ++row) {
        for (int col = 0; col <= cols; ++col) {
            const float px = static_cast<float>(rx) + col * cellW;
            const float py = static_cast<float>(ry) + row * cellH;
            points.push_back({px, py});
        }
    }

    // ------------------------------------------------------------------
    // 3. Adaptive edge vertices.
    //    For each base grid cell, sample the Sobel gradient at the cell
    //    centre.  When it exceeds edgeThreshold, insert up to 4 extra
    //    vertices at the midpoints of the cell's four edges.
    // ------------------------------------------------------------------
    if (cfg.adaptiveEdges) {
        // Collect extra points separately; merge afterwards.
        std::vector<core::FPoint> extras;
        extras.reserve(rows * cols * 4);

        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                // Cell corners in buffer space
                const float x0 = static_cast<float>(rx) + col * cellW;
                const float y0 = static_cast<float>(ry) + row * cellH;
                const float x1 = x0 + cellW;
                const float y1 = y0 + cellH;

                // Cell centre (integer sample position)
                const int cx = static_cast<int>(std::round((x0 + x1) * 0.5f));
                const int cy = static_cast<int>(std::round((y0 + y1) * 0.5f));

                if (sobelGradient(src, cx, cy) <= cfg.edgeThreshold)
                    continue;

                // 4 edge midpoints:
                //   top    edge midpoint: (mid_x, y0)
                //   bottom edge midpoint: (mid_x, y1)
                //   left   edge midpoint: (x0, mid_y)
                //   right  edge midpoint: (x1, mid_y)
                const float midX = (x0 + x1) * 0.5f;
                const float midY = (y0 + y1) * 0.5f;

                const core::FPoint candidates[4] = {
                    {midX, y0},
                    {midX, y1},
                    {x0,  midY},
                    {x1,  midY},
                };

                for (const auto& cand : candidates) {
                    // Check existing base points for duplicates.
                    bool duplicate = false;
                    for (const auto& p : points) {
                        if (nearlyEqual(p, cand)) { duplicate = true; break; }
                    }
                    if (!duplicate) {
                        // Also check already-queued extras.
                        for (const auto& e : extras) {
                            if (nearlyEqual(e, cand)) { duplicate = true; break; }
                        }
                    }
                    if (!duplicate)
                        extras.push_back(cand);
                }
            }
        }

        // Append extra points to the main list.
        points.insert(points.end(), extras.begin(), extras.end());
    }

    // ------------------------------------------------------------------
    // 4. Build the fixed-flag array.
    //    A vertex is fixed when it lies on or outside the selection
    //    boundary — same logic as GridMeshGenerator.
    //
    //    Rules (in priority order):
    //      a) If there is no active selection → no vertices are fixed.
    //      b) A vertex exactly on the buffer boundary is fixed.
    //      c) A vertex whose rounded pixel coordinate has maskValue == 0
    //         (outside selection) or sits on the selection edge is fixed.
    //         "On the edge" is approximated by checking whether any of
    //         the 4-connected neighbours has a different selection state.
    // ------------------------------------------------------------------
    const std::size_t nPts = points.size();
    std::vector<bool> fixed(nPts, false);

    const bool hasSelection = selection.hasSelection();

    for (std::size_t i = 0; i < nPts; ++i) {
        const int px = static_cast<int>(std::round(points[i].x));
        const int py = static_cast<int>(std::round(points[i].y));

        // Buffer boundary → fixed regardless of selection state.
        if (px <= 0 || py <= 0 ||
            px >= src.width() - 1 || py >= src.height() - 1) {
            fixed[i] = true;
            continue;
        }

        if (!hasSelection)
            continue;  // no selection → interior vertices remain free

        const std::uint8_t mv = selection.maskValue(px, py);
        if (mv == 0) {
            // Outside selection → fixed.
            fixed[i] = true;
        } else {
            // Check whether this vertex sits on the selection boundary by
            // testing 4-connected neighbours.
            const bool n0 = selection.maskValue(px - 1, py) == 0;
            const bool n1 = selection.maskValue(px + 1, py) == 0;
            const bool n2 = selection.maskValue(px, py - 1) == 0;
            const bool n3 = selection.maskValue(px, py + 1) == 0;
            if (n0 || n1 || n2 || n3)
                fixed[i] = true;
        }
    }

    // ------------------------------------------------------------------
    // 5. Triangulate using Bowyer-Watson Delaunay.
    // ------------------------------------------------------------------
    DeformMesh mesh = TriangulationBuilder::triangulate(points, fixed);

    // ------------------------------------------------------------------
    // 6. Store bounding region.
    // ------------------------------------------------------------------
    mesh.regionX = rx;
    mesh.regionY = ry;
    mesh.regionW = rw;
    mesh.regionH = rh;

    return mesh;
}

} // namespace core::mesh
