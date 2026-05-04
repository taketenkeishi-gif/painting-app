#include "features/object_editing/text_editor/TextEditorFeature.h"

#include <algorithm>
#include <cmath>

#include <QFont>
#include <QFontMetricsF>
#include <QTransform>

namespace features::object_editing::text_editor {

namespace {
bool inRect(const core::Rect& r, core::Point p, int pad = 0) {
  return p.x >= r.x - pad && p.y >= r.y - pad && p.x <= r.x + r.width + pad && p.y <= r.y + r.height + pad;
}
bool nearPoint(core::Point a, core::Point b, int radius = 6) {
  const int dx = a.x - b.x;
  const int dy = a.y - b.y;
  return dx * dx + dy * dy <= radius * radius;
}
}

TextEditorFeature::TextObject* TextEditorFeature::findById(const std::string& id) {
  for (auto& o : m_objects) if (o.id == id) return &o;
  return nullptr;
}
const TextEditorFeature::TextObject* TextEditorFeature::findById(const std::string& id) const {
  for (const auto& o : m_objects) if (o.id == id) return &o;
  return nullptr;
}

core::Rect TextEditorFeature::measureBounds(const TextObject& object) const {
  QFont font(QStringLiteral("Yu Gothic UI"));
  font.setPointSize(std::max(8, object.fontSize));
  QFontMetricsF metrics(font);
  const QString text = QString::fromUtf8(object.text.c_str());
  const qreal w = std::max<qreal>(12.0, metrics.horizontalAdvance(text.isEmpty() ? QStringLiteral(" ") : text));
  const qreal h = std::max<qreal>(12.0, metrics.height());
  const QRectF localRect(0.0, -h, w, h);
  QTransform t;
  t.translate(object.position.x, object.position.y);
  t.rotate(object.rotationDeg);
  const QRectF mapped = t.mapRect(localRect);
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
    const auto* obj = findById(m_editId);
    m_editOriginalText = obj == nullptr ? std::string {} : obj->text;
    m_editCreatedNow = false;
  } else {
    TextObject o;
    o.id = "txt_" + std::to_string(m_objects.size() + 1) + "_" + std::to_string(point.x) + "_" + std::to_string(point.y);
    o.position = point;
    o.fontSize = std::max(8, fontSize * 2);
    o.color = color;
    o.text.clear();
    o.bounds = measureBounds(o);
    m_objects.push_back(o);
    m_editId = o.id;
    m_editOriginalText.clear();
    m_editCreatedNow = true;
  }
  m_selectedId = m_editId;
  if (const auto* obj = findById(m_editId)) m_selectedBounds = obj->bounds;
  m_editSessionActive = true;
  return true;
}

bool TextEditorFeature::handleKeyPress(int key, const std::string& textUtf8) {
  if (!m_editSessionActive) return false;
  TextObject* obj = findById(m_editId);
  if (obj == nullptr) return false;
  if (key == Qt::Key_Return || key == Qt::Key_Enter) {
    m_editSessionActive = false;
    return true;
  }
  if (key == Qt::Key_Escape) {
    if (m_editCreatedNow) {
      removeById(m_editId);
    } else {
      obj->text = m_editOriginalText;
      obj->bounds = measureBounds(*obj);
    }
    m_editSessionActive = false;
    return true;
  }
  if (key == Qt::Key_Backspace) {
    if (!obj->text.empty()) {
      const QString q = QString::fromUtf8(obj->text.c_str());
      obj->text = q.left(q.size() - 1).toUtf8().toStdString();
    }
  } else if (!textUtf8.empty()) {
    obj->text += textUtf8;
  } else {
    return false;
  }
  obj->bounds = measureBounds(*obj);
  m_selectedBounds = obj->bounds;
  return true;
}

std::optional<std::string> TextEditorFeature::hitTextIdAt(core::Point point) const {
  for (std::size_t i = m_objects.size(); i > 0; --i) {
    const auto& o = m_objects[i - 1];
    if (o.visible && !o.locked && inRect(o.bounds, point, 4)) return o.id;
  }
  return std::nullopt;
}
std::optional<std::string> TextEditorFeature::textForId(const std::string& id) const {
  const auto* o = findById(id);
  if (o == nullptr) return std::nullopt;
  return o->text;
}
std::optional<core::Rect> TextEditorFeature::boundsForId(const std::string& id) const {
  const auto* o = findById(id);
  if (o == nullptr) return std::nullopt;
  return o->bounds;
}
bool TextEditorFeature::setTextForId(const std::string& id, const std::string& text) {
  auto* o = findById(id);
  if (o == nullptr) return false;
  o->text = text;
  o->bounds = measureBounds(*o);
  if (m_selectedId.has_value() && *m_selectedId == id) m_selectedBounds = o->bounds;
  return true;
}
bool TextEditorFeature::removeById(const std::string& id) {
  for (auto it = m_objects.begin(); it != m_objects.end(); ++it) {
    if (it->id != id) continue;
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
  const core::Point rot {b.x + b.width / 2, b.y - 18};
  if (nearPoint(point, rot)) return Handle::Rotate;
  if (nearPoint(point, tl)) return Handle::TL;
  if (nearPoint(point, tr)) return Handle::TR;
  if (nearPoint(point, bl)) return Handle::BL;
  if (nearPoint(point, br)) return Handle::BR;
  if (inRect(b, point, 4)) return Handle::Move;
  return Handle::None;
}

bool TextEditorFeature::beginOperation(core::Point point) {
  if (m_editSessionActive) return false;
  const auto hit = hitTextIdAt(point);
  m_operationActive = false;
  if (!hit.has_value()) return false;
  TextObject* obj = findById(*hit);
  if (obj == nullptr) return false;
  m_selectedId = *hit;
  m_selectedBounds = obj->bounds;
  m_activeHandle = hitHandle(*obj, point);
  m_lastPoint = point;
  m_operationActive = true;
  return true;
}

bool TextEditorFeature::updateOperation(core::Point point) {
  if (!m_operationActive || !m_selectedId.has_value()) return false;
  TextObject* obj = findById(*m_selectedId);
  if (obj == nullptr) return false;
  const int dx = point.x - m_lastPoint.x;
  const int dy = point.y - m_lastPoint.y;
  m_lastPoint = point;
  if (dx == 0 && dy == 0) return false;
  if (m_activeHandle == Handle::Move) {
    obj->position.x += dx;
    obj->position.y += dy;
  } else if (m_activeHandle == Handle::Rotate) {
    obj->rotationDeg += static_cast<float>(dx);
  } else {
    obj->fontSize = std::max(8, obj->fontSize + dy / 2);
  }
  obj->bounds = measureBounds(*obj);
  m_selectedBounds = obj->bounds;
  return true;
}

bool TextEditorFeature::endOperation() {
  const bool was = m_operationActive;
  m_operationActive = false;
  m_activeHandle = Handle::None;
  return was;
}

ObjectOverlayModel TextEditorFeature::selectionOverlay() const {
  ObjectOverlayModel out;
  if (!m_selectedBounds.has_value()) return out;
  OverlayPrimitive box;
  box.kind = OverlayPrimitive::Kind::Rect;
  box.rect = *m_selectedBounds;
  out.primitives.push_back(box);
  const core::Rect b = *m_selectedBounds;
  for (const core::Point& p : {core::Point {b.x, b.y}, core::Point {b.x + b.width, b.y},
                               core::Point {b.x, b.y + b.height}, core::Point {b.x + b.width, b.y + b.height}}) {
    OverlayPrimitive h;
    h.kind = OverlayPrimitive::Kind::HandlePoint;
    h.p1 = p;
    out.primitives.push_back(h);
  }
  OverlayPrimitive stem;
  stem.kind = OverlayPrimitive::Kind::Line;
  stem.p1 = core::Point {b.x + b.width / 2, b.y};
  stem.p2 = core::Point {b.x + b.width / 2, b.y - 18};
  out.primitives.push_back(stem);
  OverlayPrimitive rot;
  rot.kind = OverlayPrimitive::Kind::HandlePoint;
  rot.p1 = stem.p2;
  out.primitives.push_back(rot);
  return out;
}

} // namespace features::object_editing::text_editor
