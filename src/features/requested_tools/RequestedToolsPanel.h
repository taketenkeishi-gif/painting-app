#pragma once

#include "app/panels/ToolPanel.h"
#include "app/ui/ToolDescriptor.h"

#include <QGridLayout>
#include <QIcon>
#include <QMainWindow>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QToolButton>
#include <QVariant>
#include <QWidget>

#include <array>
#include <string_view>

namespace features::requested_tools {
namespace detail {

struct RequestedToolButtonSpec {
  const char* objectName;
  const char* label;
  const char* shortLabel;
  core::ToolKind kind;
  const char* subToolId;
};

inline const std::array<RequestedToolButtonSpec, 11>& requestedToolSpecs()
{
  static const std::array<RequestedToolButtonSpec, 11> specs {{
      {"requested_tool_gradient", "Gradient", "Gr", core::ToolKind::Fill, "gradient_linear"},
      {"requested_tool_comic", "Comic", "Cm", core::ToolKind::RectSelection, "comic_panel"},
      {"requested_tool_text", "Text", "Tx", core::ToolKind::MoveLayer, "text_basic"},
      {"requested_tool_ruler", "Ruler", "Ru", core::ToolKind::Line, "ruler_straight"},
      {"requested_tool_line_correction", "Line Correction", "Lc", core::ToolKind::Line, "line_correction_smooth"},
      {"requested_tool_operation", "Operation", "Op", core::ToolKind::MoveLayer, "operation_object"},
      {"requested_tool_airbrush", "Airbrush", "Ab", core::ToolKind::Brush, "airbrush_soft"},
      {"requested_tool_color_mix", "Color Mix", "Mx", core::ToolKind::Brush, "color_mix_blend"},
      {"requested_tool_liquify", "Liquify", "Lq", core::ToolKind::MoveLayer, "liquify_push"},
      {"requested_tool_clone_stamp", "Clone Stamp", "Cs", core::ToolKind::Brush, "clone_stamp_basic"},
      {"requested_tool_sketch", "Sketch", "Sk", core::ToolKind::Brush, "sketch_pencil"},
  }};
  return specs;
}

inline QString requestedToolJaLabel(std::string_view subToolId)
{
  if (subToolId == "gradient_linear") return QString::fromUtf8(u8"グラデーション");
  if (subToolId == "comic_panel") return QString::fromUtf8(u8"コミック");
  if (subToolId == "text_basic") return QString::fromUtf8(u8"テキスト");
  if (subToolId == "ruler_straight") return QString::fromUtf8(u8"定規");
  if (subToolId == "line_correction_smooth") return QString::fromUtf8(u8"線修正");
  if (subToolId == "operation_object") return QString::fromUtf8(u8"操作");
  if (subToolId == "airbrush_soft") return QString::fromUtf8(u8"エアブラシ");
  if (subToolId == "color_mix_blend") return QString::fromUtf8(u8"色混ぜ");
  if (subToolId == "liquify_push") return QString::fromUtf8(u8"ゆがみ");
  if (subToolId == "clone_stamp_basic") return QString::fromUtf8(u8"コピースタンプ");
  if (subToolId == "sketch_pencil") return QString::fromUtf8(u8"スケッチ");
  return {};
}

inline QIcon makeRequestedToolIcon(std::string_view subToolId, int size = 24)
{
  QPixmap pixmap(size, size);
  pixmap.fill(Qt::transparent);

  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing, true);
  const QRectF frame(3.0, 3.0, static_cast<qreal>(size - 6), static_cast<qreal>(size - 6));
  painter.setPen(QPen(QColor(242, 246, 252), 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter.setBrush(Qt::NoBrush);

  auto drawTextGlyph = [&](const QString& t) {
    QFont font = painter.font();
    font.setPixelSize(std::max(12, size - 8));
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(frame.toRect(), Qt::AlignCenter, t);
  };

  if (subToolId == "gradient_linear") {
    painter.drawLine(QPointF(frame.left() + 1, frame.bottom() - 1), QPointF(frame.right() - 1, frame.top() + 1));
    painter.drawLine(QPointF(frame.left() + 1, frame.bottom() - 6), QPointF(frame.right() - 6, frame.top() + 1));
  } else if (subToolId == "comic_panel") {
    painter.drawRect(frame.adjusted(1, 1, -1, -1));
    painter.drawLine(QPointF(frame.left() + 6, frame.center().y()), QPointF(frame.right() - 6, frame.center().y()));
  } else if (subToolId == "text_basic") {
    drawTextGlyph("T");
  } else if (subToolId == "ruler_straight") {
    painter.drawRect(frame.adjusted(2, 8, -2, -8));
    for (int i = 0; i < 4; ++i) {
      const qreal x = frame.left() + 5 + i * 4;
      painter.drawLine(QPointF(x, frame.center().y() - 4), QPointF(x, frame.center().y() + 4));
    }
  } else if (subToolId == "line_correction_smooth") {
    QPainterPath smooth;
    smooth.moveTo(frame.left(), frame.center().y() + 1);
    smooth.cubicTo(frame.left() + 5, frame.top() + 2, frame.right() - 5, frame.bottom() - 2, frame.right(), frame.center().y() - 1);
    painter.drawPath(smooth);
  } else if (subToolId == "operation_object") {
    painter.drawRect(frame.adjusted(4, 4, -4, -4));
    painter.drawLine(QPointF(frame.center().x(), frame.top()), QPointF(frame.center().x(), frame.bottom()));
    painter.drawLine(QPointF(frame.left(), frame.center().y()), QPointF(frame.right(), frame.center().y()));
  } else if (subToolId == "airbrush_soft") {
    for (int i = 0; i < 5; ++i) {
      const qreal x = frame.left() + 2 + i * 3;
      const qreal y = frame.bottom() - 2 - i * 2;
      painter.drawPoint(QPointF(x, y));
    }
  } else if (subToolId == "color_mix_blend") {
    painter.drawEllipse(QPointF(frame.left() + 8, frame.center().y()), 4.5, 4.5);
    painter.drawEllipse(QPointF(frame.right() - 8, frame.center().y()), 4.5, 4.5);
    painter.drawLine(QPointF(frame.left() + 10, frame.center().y()), QPointF(frame.right() - 10, frame.center().y()));
  } else if (subToolId == "liquify_push") {
    QPainterPath wave;
    wave.moveTo(frame.left(), frame.center().y());
    wave.cubicTo(frame.left() + 4, frame.top() + 2, frame.right() - 6, frame.bottom() - 2, frame.right(), frame.center().y());
    painter.drawPath(wave);
  } else if (subToolId == "clone_stamp_basic") {
    painter.drawEllipse(QPointF(frame.left() + 7, frame.center().y()), 3.2, 3.2);
    painter.drawRect(frame.right() - 10, frame.center().y() - 4, 8, 8);
    painter.drawLine(QPointF(frame.left() + 10, frame.center().y()), QPointF(frame.right() - 10, frame.center().y()));
  } else if (subToolId == "sketch_pencil") {
    painter.drawLine(QPointF(frame.left() + 3, frame.bottom() - 2), QPointF(frame.right() - 5, frame.top() + 4));
    painter.drawLine(QPointF(frame.right() - 5, frame.top() + 4), QPointF(frame.right() - 1, frame.top() + 8));
    painter.drawLine(QPointF(frame.left() + 5, frame.bottom() - 2), QPointF(frame.right() - 3, frame.top() + 6));
  } else {
    drawTextGlyph("?");
  }

  return QIcon(pixmap);
}

inline QGridLayout* findToolGrid(app::panels::ToolPanel& panel)
{
  if (auto* grid = qobject_cast<QGridLayout*>(panel.layout())) {
    return grid;
  }

  const auto grids = panel.findChildren<QGridLayout*>();
  if (!grids.isEmpty()) {
    return grids.front();
  }

  return nullptr;
}

inline bool alreadyInstalled(app::panels::ToolPanel& panel)
{
  return panel.findChild<QToolButton*>("requested_tool_gradient") != nullptr;
}

inline void addRequestedButtonsToGrid(app::panels::ToolPanel& panel, QGridLayout& grid)
{
  int maxRow = -1;
  int maxColumn = 1;

  for (int index = 0; index < grid.count(); ++index) {
    int row = 0;
    int column = 0;
    int rowSpan = 0;
    int columnSpan = 0;
    grid.getItemPosition(index, &row, &column, &rowSpan, &columnSpan);
    if (row > maxRow) {
      maxRow = row;
    }
    if (column > maxColumn) {
      maxColumn = column;
    }
  }

  const int columns = maxColumn >= 2 ? 2 : maxColumn + 1;
  const int startRow = maxRow + 1;
  const auto& specs = requestedToolSpecs();

  for (int index = 0; index < static_cast<int>(specs.size()); ++index) {
    const RequestedToolButtonSpec& spec = specs[static_cast<std::size_t>(index)];
    auto* button = new QToolButton(&panel);
    button->setObjectName(QString::fromLatin1(spec.objectName));
    button->setProperty("requestedToolPlaceholder", true);
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setIcon(makeRequestedToolIcon(spec.subToolId));
    button->setIconSize(QSize(22, 22));
    button->setFixedSize(QSize(32, 32));
    button->setCheckable(true);
    button->setAutoExclusive(false);
    button->setToolTip(QString::fromLatin1(spec.label));
    button->setStatusTip(QString::fromLatin1(spec.label) + QStringLiteral(" placeholder"));

    QObject::connect(button, &QToolButton::clicked, &panel, [button, &panel]() {
      const auto buttons = panel.findChildren<QToolButton*>();
      for (QToolButton* other : buttons) {
        if (other != nullptr && other->property("requestedToolPlaceholder").toBool()) {
          other->setChecked(other == button);
        }
      }
    });

    const int row = startRow + (index / columns);
    const int column = index % columns;
    grid.addWidget(button, row, column);
  }
}

} // namespace detail

inline void attachRequestedToolsPanel(QMainWindow& window)
{
  auto* panel = window.findChild<app::panels::ToolPanel*>();
  if (panel == nullptr) {
    return;
  }

  if (detail::alreadyInstalled(*panel)) {
    return;
  }

  QGridLayout* grid = detail::findToolGrid(*panel);
  if (grid == nullptr) {
    return;
  }

  detail::addRequestedButtonsToGrid(*panel, *grid);
}

} // namespace features::requested_tools
