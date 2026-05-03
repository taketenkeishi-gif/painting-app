#pragma once

#include "app/panels/ToolPanel.h"
#include "app/ui/ToolDescriptor.h"

#include <QGridLayout>
#include <QIcon>
#include <QLinearGradient>
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
  const QRectF frame(2.0, 2.0, static_cast<qreal>(size - 4), static_cast<qreal>(size - 4));

  auto drawLine = [&](const QColor& c, qreal w, const QPointF& a, const QPointF& b) {
    painter.setPen(QPen(c, w, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawLine(a, b);
  };
  auto drawText = [&](const QString& t) {
    painter.setPen(QColor(230, 238, 252));
    QFont font = painter.font();
    font.setPixelSize(16);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(frame.toRect(), Qt::AlignCenter, t);
  };

  if (subToolId == "gradient_linear") {
    QLinearGradient g(frame.topLeft(), frame.topRight());
    g.setColorAt(0.0, QColor(240, 242, 250));
    g.setColorAt(1.0, QColor(90, 132, 210));
    painter.fillRect(frame.adjusted(4, 7, -4, -7), g);
  } else if (subToolId == "comic_panel") {
    painter.setPen(QPen(QColor(232, 240, 252), 1.4));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(frame.adjusted(5, 5, -5, -5));
    painter.drawLine(QPointF(frame.left() + 7, frame.bottom() - 7), QPointF(frame.left() + 13, frame.bottom() - 7));
  } else if (subToolId == "text_basic") {
    drawText("T");
  } else if (subToolId == "ruler_straight") {
    drawLine(QColor(228, 236, 250), 1.5, QPointF(frame.left() + 5, frame.bottom() - 6), QPointF(frame.right() - 5, frame.top() + 6));
  } else if (subToolId == "line_correction_smooth") {
    QPainterPath smooth;
    smooth.moveTo(frame.left() + 4, frame.center().y() + 1);
    smooth.cubicTo(frame.left() + 8, frame.top() + 8, frame.left() + 13, frame.bottom() - 7, frame.right() - 4, frame.center().y() - 1);
    painter.setPen(QPen(QColor(232, 240, 252), 1.5));
    painter.drawPath(smooth);
  } else if (subToolId == "operation_object") {
    painter.setPen(QPen(QColor(230, 238, 252), 1.2));
    painter.setBrush(QColor(84, 118, 170));
    painter.drawEllipse(QPointF(frame.center().x(), frame.center().y()), 4.0, 4.0);
    drawLine(QColor(230, 238, 252), 1.2, QPointF(frame.center().x() + 5, frame.center().y()), QPointF(frame.right() - 4, frame.center().y()));
  } else if (subToolId == "airbrush_soft") {
    painter.setPen(Qt::NoPen);
    for (int i = 0; i < 18; ++i) {
      painter.setBrush(QColor(220, 232, 252, 25 + (i % 6) * 18));
      const qreal x = frame.left() + 5 + (i * 3) % 12;
      const qreal y = frame.top() + 6 + (i * 5) % 10;
      painter.drawEllipse(QPointF(x, y), 1.2 + (i % 3), 1.2 + (i % 3));
    }
  } else if (subToolId == "color_mix_blend") {
    QLinearGradient g(frame.left() + 4, frame.center().y(), frame.right() - 4, frame.center().y());
    g.setColorAt(0.0, QColor(232, 96, 96));
    g.setColorAt(0.5, QColor(190, 154, 180));
    g.setColorAt(1.0, QColor(98, 138, 228));
    painter.setPen(Qt::NoPen);
    painter.setBrush(g);
    painter.drawRoundedRect(frame.adjusted(4, 8, -4, -8), 3.0, 3.0);
  } else if (subToolId == "liquify_push") {
    QPainterPath wave;
    wave.moveTo(frame.left() + 4, frame.center().y() + 4);
    wave.cubicTo(frame.left() + 8, frame.top() + 3, frame.left() + 14, frame.bottom() - 3, frame.right() - 6, frame.center().y() - 3);
    painter.setPen(QPen(QColor(220, 232, 252), 1.5));
    painter.drawPath(wave);
  } else if (subToolId == "clone_stamp_basic") {
    painter.setPen(QPen(QColor(230, 238, 252), 1.2));
    painter.setBrush(QColor(70, 96, 140));
    painter.drawEllipse(QPointF(frame.left() + 8, frame.center().y()), 3.2, 3.2);
    painter.setBrush(QColor(95, 124, 176));
    painter.drawRect(frame.right() - 10, frame.center().y() - 3, 6, 6);
  } else if (subToolId == "sketch_pencil") {
    drawLine(QColor(210, 220, 238), 1.1, QPointF(frame.left() + 5, frame.bottom() - 6), QPointF(frame.right() - 5, frame.top() + 7));
    drawLine(QColor(146, 162, 188), 0.9, QPointF(frame.left() + 6, frame.bottom() - 4), QPointF(frame.right() - 4, frame.top() + 9));
  } else {
    drawText("?");
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
