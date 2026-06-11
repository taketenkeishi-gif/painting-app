#include "core/mesh/GridMeshGenerator.h"
#include "core/mesh/MeshTypes.h"
#include <algorithm>
#include <cmath>

namespace core::mesh {

DeformMesh GridMeshGenerator::generate(const core::PixelBuffer&    src,
                                        const core::SelectionMask& selection,
                                        const MeshGenConfig&       cfg) const
{
    // ------------------------------------------------------------------ //
    // 1. Determine bounding region
    // ------------------------------------------------------------------ //
    int bufW = src.width();
    int bufH = src.height();

    int x0 = 0, y0 = 0, rw = bufW, rh = bufH;

    if (selection.hasSelection()) {
        auto optRect = selection.boundingRect();
        if (optRect.has_value()) {
            const core::Rect& br = optRect.value();
            x0 = br.x;
            y0 = br.y;
            rw = br.width;
            rh = br.height;
        }
    }

    // Clamp to buffer bounds
    x0 = std::max(0, x0);
    y0 = std::max(0, y0);
    if (x0 + rw > bufW) rw = bufW - x0;
    if (y0 + rh > bufH) rh = bufH - y0;
    if (rw <= 0) rw = 1;
    if (rh <= 0) rh = 1;

    // ------------------------------------------------------------------ //
    // 2. Allocate mesh
    // ------------------------------------------------------------------ //
    const int rows = cfg.gridRows;
    const int cols = cfg.gridCols;

    const int vertCols = cols + 1;
    const int vertRows = rows + 1;
    const int vertCount = vertRows * vertCols;

    DeformMesh mesh;
    mesh.vertices.resize(static_cast<std::size_t>(vertCount));
    mesh.regionX = x0;
    mesh.regionY = y0;
    mesh.regionW = rw;
    mesh.regionH = rh;

    // ------------------------------------------------------------------ //
    // 3. Build vertices
    // ------------------------------------------------------------------ //
    const float fCols = static_cast<float>(cols);
    const float fRows = static_cast<float>(rows);

    for (int r = 0; r < vertRows; ++r) {
        for (int c = 0; c < vertCols; ++c) {
            float px = static_cast<float>(x0) + (static_cast<float>(c) / fCols) * static_cast<float>(rw);
            float py = static_cast<float>(y0) + (static_cast<float>(r) / fRows) * static_cast<float>(rh);

            core::FPoint pos { px, py };

            int idx = r * vertCols + c;
            mesh.vertices[static_cast<std::size_t>(idx)].original = pos;
            mesh.vertices[static_cast<std::size_t>(idx)].deformed = pos;

            // ---------------------------------------------------------- //
            // 4. Determine fixed flag
            // ---------------------------------------------------------- //
            bool fixed = false;

            if (selection.hasSelection()) {
                int ix = static_cast<int>(std::round(px));
                int iy = static_cast<int>(std::round(py));

                if (!selection.contains(ix, iy)) {
                    fixed = true;
                } else {
                    // Check 8-neighbours for boundary detection
                    for (int dy = -1; dy <= 1 && !fixed; ++dy) {
                        for (int dx = -1; dx <= 1 && !fixed; ++dx) {
                            if (dx == 0 && dy == 0) continue;
                            if (!selection.contains(ix + dx, iy + dy)) {
                                fixed = true;
                            }
                        }
                    }
                }
            }

            mesh.vertices[static_cast<std::size_t>(idx)].fixed = fixed;
        }
    }

    // ------------------------------------------------------------------ //
    // 5. Build triangles (2 per cell)
    // ------------------------------------------------------------------ //
    mesh.triangles.reserve(static_cast<std::size_t>(rows * cols * 2));

    auto idx = [&](int r, int c) -> int {
        return r * vertCols + c;
    };

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            // Upper-left triangle: (r,c), (r,c+1), (r+1,c)
            {
                MeshTriangle tri;
                tri.v[0] = idx(r,     c    );
                tri.v[1] = idx(r,     c + 1);
                tri.v[2] = idx(r + 1, c    );
                mesh.triangles.push_back(tri);
            }
            // Lower-right triangle: (r,c+1), (r+1,c+1), (r+1,c)
            {
                MeshTriangle tri;
                tri.v[0] = idx(r,     c + 1);
                tri.v[1] = idx(r + 1, c + 1);
                tri.v[2] = idx(r + 1, c    );
                mesh.triangles.push_back(tri);
            }
        }
    }

    return mesh;
}

} // namespace core::mesh
