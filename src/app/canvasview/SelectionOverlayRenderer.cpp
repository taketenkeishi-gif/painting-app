#include "app/canvasview/SelectionOverlayRenderer.h"

#include <algorithm>

#include <QPainter>
#include <QPainterPath>
#include <QTransform>

namespace app::canvasview {

// Marching ants strategy:
//   Scan every pixel boundary inside the selection bounding rect.
//   Assign each 1px boundary segment a color (black or white) based on
//   its position along the contour axis + marchingOffset. This makes the
//   black/white pattern "shift" each timer tick, giving the marching effect
//   without requiring a full contour-tracing pass.
//
//   Horizontal boundary at (x, y): phase = (x - marchingOffset) % period
//   Vertical   boundary at (x, y): phase = (y - marchingOffset) % period
//
//   All paths are drawn with a 1px cosmetic pen so thickness is
//   zoom-invariant.

void SelectionOverlayRenderer::render(QPainter&                  painter,
                                       const core::SelectionMask& mask,
                                       double                     zoom,
                                       QPointF                    panOffset,
                                       int                        marchingOffset) {
  if (!mask.hasSelection()) return;

  const int w = mask.width();
  const int h = mask.height();
  if (w <= 0 || h <= 0) return;

  const auto boundsOpt = mask.boundingRect();
  if (!boundsOpt.has_value()) return;

  constexpr int kDashLen = 4;          // canvas pixels per half-cycle
  constexpr int kPeriod  = kDashLen * 2;

  const int bx  = boundsOpt->x;
  const int by  = boundsOpt->y;
  const int bx1 = std::min(w, bx + boundsOpt->width);
  const int by1 = std::min(h, by + boundsOpt->height);

  // inside() clamps out-of-bounds to "unselected"
  auto inside = [&](int x, int y) -> bool {
    if (x < 0 || y < 0 || x >= w || y >= h) return false;
    return mask.maskValue(x, y) >= 128;
  };

  QPainterPath blackPath, whitePath;

  // ── Horizontal boundary segments (top/bottom edge of selected rows) ──
  for (int y = by; y <= by1; ++y) {
    for (int x = bx; x < bx1; ++x) {
      if (inside(x, y) != inside(x, y - 1)) {
        const int phase = ((x - marchingOffset) % kPeriod + kPeriod) % kPeriod;
        QPainterPath& p = (phase < kDashLen) ? blackPath : whitePath;
        p.moveTo(x,     y);
        p.lineTo(x + 1, y);
      }
    }
  }

  // ── Vertical boundary segments (left/right edge of selected columns) ──
  for (int y = by; y < by1; ++y) {
    for (int x = bx; x <= bx1; ++x) {
      if (inside(x, y) != inside(x - 1, y)) {
        const int phase = ((y - marchingOffset) % kPeriod + kPeriod) % kPeriod;
        QPainterPath& p = (phase < kDashLen) ? blackPath : whitePath;
        p.moveTo(x, y);
        p.lineTo(x, y + 1);
      }
    }
  }

  // ── Map canvas coordinates → widget coordinates ──────────────────────
  QTransform t;
  t.translate(panOffset.x(), panOffset.y());
  t.scale(zoom, zoom);

  blackPath = t.map(blackPath);
  whitePath = t.map(whitePath);

  painter.setRenderHint(QPainter::Antialiasing, false);

  QPen pen;
  pen.setCosmetic(true);
  pen.setWidth(1);

  pen.setColor(QColor(0, 0, 0, 210));
  painter.setPen(pen);
  painter.drawPath(blackPath);

  pen.setColor(QColor(255, 255, 255, 210));
  painter.setPen(pen);
  painter.drawPath(whitePath);
}

} // namespace app::canvasview
