#pragma once

#include <QPointF>

#include "core/selection/SelectionMask.h"

class QPainter;

namespace app::canvasview {

// Renders Photoshop-style marching ants for a SelectionMask.
// All selection tools share this single renderer — no tool draws its own
// committed selection outline.
class SelectionOverlayRenderer {
public:
  // panOffset: canvas origin in widget coordinates (i.e. QPointF(target.x(), target.y()))
  // zoom:      canvas-to-screen scale factor
  // marchingOffset: animation phase (0..15, incremented each timer tick)
  static void render(QPainter&                  painter,
                     const core::SelectionMask& mask,
                     double                     zoom,
                     QPointF                    panOffset,
                     int                        marchingOffset);
};

} // namespace app::canvasview
