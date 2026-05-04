#include "features/object_editing/text_editor/TextEditorFeature.h"

#include <algorithm>
#include <cmath>

#include <QFont>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QString>
#include <QTransform>
#include <Qt>

namespace features::object_editing::text_editor {

namespace {

bool inRect(const core::Rect& r, core::Point p, int pad = 0) {
  return p.x >= r.x - pad && p.y >= r.y - pad && p.x <= r.x + r.width + pad && p.y <= r.y + r.height + pad;
}

int distanceSquared(core::Point a, core::Point b) {
  const int dx = a.x - b.x;
  const int dy = a.y - b.y;
  return dx * dx + dy * dy;
}

bool nearPoint(core::Point a, core::Point b, int radius = 10) {
  return distanceSquared(a, b) <= radius * radius;
}

QFont textFont(int fontSize) {
  QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
  font.setFamilies(QStringList {QStringLiteral("Yu Gothic UI"), QStringLiteral("Meiryo"), QStringLiteral("Noto Sans CJK JP"), font.family()});
  font.setPointSize(std::max(8, fontSize));
  return font;
}

QString toQString(const std::string& value) {
  return QString::fromUtf8(value.data(), static_cast<int>(value.size()));
}

std::string toUtf8String(const QString& value) {
  const QByteArray utf8 = value.toUtf8();
  return std::string(utf8.constData(), static_cast<std::size_t>(utf8.size()));
}

} // namespace

TextEditorFeature::TextObject* TextEditorFeature::findById(const std::string& id) {
  for (auto& object : m_objects) {
    if (object.id == id) {
      return &object;
    }
  }
  return nullptr;
}

const TextEditorFeature::TextObject* TextEditorFeature::findById(const std::string& id) const {
  for (const auto& object : m_objects) {
    if (object.id == id) {
      return &object;
    }
  }
  return nullptr;
}

core::Rect TextEditorFeature::measureBounds(const TextObject& object) const {
  const QFont font = textFont(object.fontSize);
  QFontMetricsF metrics(font);
  const QString text = toQString(object.text);
  const QString measured = text.isEmpty() ? QStringLiteral(" ") : text;
  const qreal w = std::max<qreal>(16.0, metrics.horizontalAdvance(measured));
  const qreal ascent = std::max<qreal>(1.0, metrics.ascent());
  const qreal descent = std::max<qreal>(1.0, metrics.descent());
  const QRectF localRect(0.0, -ascent, w, ascent + descent);
  QTransform transform;
  transform.translate(object.position.x, object.position.y);
  transform.rotate(object.rotationDeg);
  const QRectF mapped = transform.mapRect(localRect).adjusted(-3.0, -3.0, 3.0, 3.0);
  return core::Rect {
      static_cast<int>(std::floor(mapped.x())),
      static_cast<int>(std::floor(mapped.y())),
      std::max(1, static_cast<int>(std::ceil(mapped.width()))),
      std::max(1, static_cast<int>(std::ceil(mapped.height())))};
}

bool TextEditorFeature::beginTextInput(core::Point point, const core::Color& color, int fontSize) {
  const auto existing = hitTextIdAt(point);
  if (existing.has_value()) {
    m_editId = *existing;
    const auto* object = findById(m_editId);
    m_editOriginalText = object == nullptr ? std::string {} : object->text;
    m_editCreatedNow = false;
  } else {
    TextObject object;
    object.id = "txt_" + std::to_string(m_objects.size() + 1) + "_" + std::to_string(point.x) + "_" + std::to_string(point.y);
    object.position = point;
    object.fontSize = std::max(8, fontSize * 2);
    object.color = color;
    object.text.clear();
    object.bounds = measureBounds(object);
    m_objects.push_back(object);
    m_editId = object.id;
    m_editOriginalText.clear();
    m_editCreatedNow = true;
  }
  m_selectedId = m_editId;
  if (const auto* object = findById(m_editId)) {
    m_selectedBounds = object->bounds;
  }
  m_operationActive = false;
  m_activeHandle = Handle::None;
  m_editSessionActive = true;
  return true;
}

bool TextEditorFeature::handleKeyPress(int key, const std::string& textUtf8) {
  if (!m_editSessionActive) {
    return false;
  }
  TextObject* object = findById(m_editId);
  if (object == nullptr) {
    return false;
  }
  if (key == Qt::Key_Return || key == Qt::Key_Enter) {
    if (m_editCreatedNow && object->text.empty()) {
      removeById(m_editId);
    }
    m_editSessionActive = false;
    return true;
  }
  if (key == Qt::Key_Escape) {
    if (m_editCreatedNow) {
      removeById(m_editId);
    } else {
      object->text = m_editOriginalText;
      object->bounds = measureBounds(*object);
      m_selectedBounds = object->bounds;
    }
    m_editSessionActive = false;
    return true;
  }
  if (key == Qt::Key_Backspace) {
    const QString current = toQString(object->text);
    if (!current.isEmpty()) {
      object->text = toUtf8String(current.left(current.size() - 1));
    }
  } else if (!textUtf8.empty()) {
    object->text += textUtf8;
  } else {
    return false;
  }
  object->bounds = measureBounds(*object);
  m_selectedBounds = object->bounds;
  return true;
}

std::optional<std::string> TextEditorFeature::hitTextIdAt(core::Point point) const {
  for (std::size_t i = m_objects.size(); i > 0; --i) {
    const auto& object = m_objects[i - 1];
    if (object.visible && !object.locked && inRect(object.bounds, point, 6)) {
      return object.id;
    }
  }
  return std::nullopt;
}

std::optional<std::string> TextEditorFeature::textForId(const std::string& id) const {
  const auto* object = findById(id);
  if (object == nullptr) {
    return std::nullopt;
  }
  return object->text;
}

std::optional<core::Rect> TextEditorFeature::boundsForId(const std::string& id) const {
  const auto* object = findById(id);
  if (object == nullptr) {
    return std::nullopt;
  }
  return object->bounds;
}

bool TextEditorFeature::setTextForId(const std::string& id, const std::string& text) {
  auto* object = findById(id);
  if (object == nullptr) {
    return false;
  }
  object->text = text;
  object->bounds = measureBounds(*object);
  if (m_selectedId.has_value() && *m_selectedId == id) {
    m_selectedBounds = object->bounds;
  }
  return true;
}

bool TextEditorFeature::removeById(const std::string& id) {
  for (auto it = m_objects.begin(); it != m_objects.end(); ++it) {
    if (it->id != id) {
      continue;
    }
    m_objects.erase(it);
    if (m_selectedId.has_value() && *m_selectedId == id) {
      m_selectedId.reset();
      m_selectedBounds.reset();
    }
    return true;
  }
  return false;
}

TextEditorFeature::Handle TextEditorFeature::hitHandle(const TextObject& object, core::Point point) const {
  const core::Rect b = object.bounds;
  const core::Point tl {b.x, b.y};
  const core::Point tr {b.x + b.width, b.y};
  const core::Point bl {b.x, b.y + b.height};
  const core::Point br {b.x + b.width, b.y + b.height};
  const core::Point rot {b.x + b.width / 2, b.y - 22};
  if (nearPoint(point, rot, 12)) return Handle::Rotate;
  if (nearPoint(point, tl)) return Handle::TL;
  if (nearPoint(point, tr)) return Handle::TR;
  if (nearPoint(point, bl)) return Handle::BL;
  if (nearPoint(point, br)) return Handle::BR;
  if (inRect(b, point, 6)) return Handle::Move;
  return Handle::None;
}

bool TextEditorFeature::beginOperation(core::Point point) {
  if (m_editSessionActive) {
    m_editSessionActive = false;
  }

  m_operationActive = false;
  m_activeHandle = Handle::None;

  for (std::size_t i = m_objects.size(); i > 0; --i) {
    TextObject& object = m_objects[i - 1];
    if (!object.visible || object.locked) {
      continue;
    }

    const Handle handle = hitHandle(object, point);
    if (handle == Handle::None) {
      continue;
    }

    m_selectedId = object.id;
    m_selectedBounds = object.bounds;
    m_activeHandle = handle;
    m_lastPoint = point;
    m_operationCenter = core::Point {object.bounds.x + object.bounds.width / 2, object.bounds.y + object.bounds.height / 2};
    m_operationActive = true;
    return true;
  }

  m_selectedId.reset();
  m_selectedBounds.reset();
  return false;
}

bool TextEditorFeature::updateOperation(core::Point point) {
  if (!m_operationActive || !m_selectedId.has_value()) {
    return false;
  }
  TextObject* object = findById(*m_selectedId);
  if (object == nullptr) {
    return false;
  }
  const int dx = point.x - m_lastPoint.x;
  const int dy = point.y - m_lastPoint.y;
  m_lastPoint = point;
  if (dx == 0 && dy == 0) {
    return false;
  }
  if (m_activeHandle == Handle::Move) {
    object->position.x += dx;
    object->position.y += dy;
  } else if (m_activeHandle == Handle::Rotate) {
    // text-editor-stable-rotate-center
    const double cx = static_cast<double>(m_operationCenter.x);
    const double cy = static_cast<double>(m_operationCenter.y);
    const double angle = std::atan2(static_cast<double>(point.y) - cy, static_cast<double>(point.x) - cx) * 180.0 / 3.14159265358979323846;
    object->rotationDeg = static_cast<float>(angle);
  } else {
    int delta = 0;
    if (m_activeHandle == Handle::TL) {
      delta = -dx - dy;
    } else if (m_activeHandle == Handle::TR) {
      delta = dx - dy;
    } else if (m_activeHandle == Handle::BL) {
      delta = -dx + dy;
    } else {
      delta = dx + dy;
    }
    object->fontSize = std::max(8, object->fontSize + delta / 4);
  }
  object->bounds = measureBounds(*object);
  m_selectedBounds = object->bounds;
  return true;
}

bool TextEditorFeature::endOperation() {
  const bool wasActive = m_operationActive;
  m_operationActive = false;
  m_activeHandle = Handle::None;
  return wasActive;
}

ObjectOverlayModel TextEditorFeature::selectionOverlay() const {
  ObjectOverlayModel out;
  if (!m_selectedBounds.has_value()) {
    return out;
  }
  OverlayPrimitive box;
  box.kind = OverlayPrimitive::Kind::Rect;
  box.rect = *m_selectedBounds;
  out.primitives.push_back(box);
  const core::Rect b = *m_selectedBounds;
  for (const core::Point& point : {core::Point {b.x, b.y}, core::Point {b.x + b.width, b.y},
                                   core::Point {b.x, b.y + b.height}, core::Point {b.x + b.width, b.y + b.height}}) {
    OverlayPrimitive handle;
    handle.kind = OverlayPrimitive::Kind::HandlePoint;
    handle.p1 = point;
    out.primitives.push_back(handle);
  }
  OverlayPrimitive stem;
  stem.kind = OverlayPrimitive::Kind::Line;
  stem.p1 = core::Point {b.x + b.width / 2, b.y};
  stem.p2 = core::Point {b.x + b.width / 2, b.y - 22};
  out.primitives.push_back(stem);
  OverlayPrimitive rotate;
  rotate.kind = OverlayPrimitive::Kind::HandlePoint;
  rotate.p1 = stem.p2;
  out.primitives.push_back(rotate);

  /* text-editor-live-caret-overlay */
  if (m_editSessionActive && m_selectedId.has_value()) {
    const auto* object = findById(*m_selectedId);
    if (object != nullptr) {
      const QFont font = textFont(object->fontSize);
      const QFontMetricsF metrics(font);
      const QString text = toQString(object->text);
      const qreal advance = std::max<qreal>(0.0, metrics.horizontalAdvance(text));
      const qreal ascent = std::max<qreal>(1.0, metrics.ascent());
      const qreal descent = std::max<qreal>(1.0, metrics.descent());
      const double radians = static_cast<double>(object->rotationDeg) * 3.14159265358979323846 / 180.0;
      const double c = std::cos(radians);
      const double s = std::sin(radians);
      auto mapLocal = [&](double x, double y) -> core::Point {
        return core::Point {
            static_cast<int>(std::lround(static_cast<double>(object->position.x) + x * c - y * s)),
            static_cast<int>(std::lround(static_cast<double>(object->position.y) + x * s + y * c))};
      };
      OverlayPrimitive caret;
      caret.kind = OverlayPrimitive::Kind::Line;
      caret.p1 = mapLocal(advance + 2.0, -ascent);
      caret.p2 = mapLocal(advance + 2.0, descent);
      out.primitives.push_back(caret);
    }
  }
  return out;
}

} // namespace features::object_editing::text_editor

