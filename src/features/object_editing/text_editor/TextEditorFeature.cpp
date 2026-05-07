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

QFont baseTextFont(int fontSize) {
  QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
  font.setFamilies(QStringList {QStringLiteral("Yu Gothic UI"), QStringLiteral("Meiryo"), QStringLiteral("Noto Sans CJK JP"), font.family()});
  font.setPointSize(std::max(8, fontSize));
  return font;
}

QFont textFont(int fontSize) {
  return baseTextFont(fontSize);
}

QFont textFontForObject(const TextEditorFeature::TextObject& object) {
  QFont font = baseTextFont(object.fontSize);
  if (!object.fontFamily.empty()) {
    font.setFamily(QString::fromUtf8(object.fontFamily.data(), static_cast<int>(object.fontFamily.size())));
  }
  font.setBold(object.bold);
  font.setItalic(object.italic);
  font.setUnderline(object.underline);
  font.setStrikeOut(object.strikeOut);
  return font;
}

QString toQString(const std::string& value) {
  return QString::fromUtf8(value.data(), static_cast<int>(value.size()));
}

std::string toUtf8String(const QString& value) {
  const QByteArray utf8 = value.toUtf8();
  return std::string(utf8.constData(), static_cast<std::size_t>(utf8.size()));
}

int clampedCaretIndex(int index, const QString& text) {
  const int size = static_cast<int>(text.size());
  if (index < 0) {
    return 0;
  }
  if (index > size) {
    return size;
  }
  return index;
}

double angleDegFromCenter(core::Point center, core::Point point) {
  return std::atan2(static_cast<double>(point.y - center.y), static_cast<double>(point.x - center.x)) * 180.0 / 3.14159265358979323846;
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
  const QFont font = textFontForObject(object);
  QFontMetricsF metrics(font);
  const QString text = toQString(object.text);
  const QString measured = text.isEmpty() ? QStringLiteral(" ") : text;
  const qreal w = std::max<qreal>(16.0, metrics.horizontalAdvance(measured)) * std::max<qreal>(0.1, object.scaleX);
  const qreal ascent = std::max<qreal>(1.0, metrics.ascent()) * std::max<qreal>(0.1, object.scaleY);
  const qreal descent = std::max<qreal>(1.0, metrics.descent()) * std::max<qreal>(0.1, object.scaleY);
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
int TextEditorFeature::caretIndexAtPoint(const TextObject& object, core::Point point) const {
  const QString text = toQString(object.text);
  if (text.isEmpty()) {
    return 0;
  }

  const double radians = static_cast<double>(object.rotationDeg) * 3.14159265358979323846 / 180.0;
  const double c = std::cos(radians);
  const double s = std::sin(radians);
  const double dx = static_cast<double>(point.x - object.position.x);
  const double dy = static_cast<double>(point.y - object.position.y);
  const double localX = (dx * c + dy * s) / std::max(0.1F, object.scaleX);

  const QFont font = textFontForObject(object);
  QFontMetricsF metrics(font);
  int bestIndex = 0;
  double bestDistance = std::abs(localX);
  for (int i = 1; i <= text.size(); ++i) {
    const double x = metrics.horizontalAdvance(text.left(i));
    const double distance = std::abs(localX - x);
    if (distance < bestDistance) {
      bestDistance = distance;
      bestIndex = i;
    }
  }
  return bestIndex;
}

core::Point TextEditorFeature::boundsAnchorPoint(const core::Rect& bounds, Handle handle) const {
  const int left = bounds.x;
  const int top = bounds.y;
  const int right = bounds.x + bounds.width;
  const int bottom = bounds.y + bounds.height;
  const int centerX = bounds.x + bounds.width / 2;
  const int centerY = bounds.y + bounds.height / 2;

  if (handle == Handle::TL) {
    return core::Point {right, bottom};
  }
  if (handle == Handle::T) {
    return core::Point {centerX, bottom};
  }
  if (handle == Handle::TR) {
    return core::Point {left, bottom};
  }
  if (handle == Handle::L) {
    return core::Point {right, centerY};
  }
  if (handle == Handle::R) {
    return core::Point {left, centerY};
  }
  if (handle == Handle::BL) {
    return core::Point {right, top};
  }
  if (handle == Handle::B) {
    return core::Point {centerX, top};
  }
  return core::Point {left, top};
}

bool TextEditorFeature::beginTextInput(core::Point point, const core::Color& color, int fontSize) {
  const auto existing = hitTextIdAt(point);
  if (existing.has_value()) {
    m_editId = *existing;
    const auto* object = findById(m_editId);
    m_editOriginalText = object == nullptr ? std::string {} : object->text;
    m_editCaretIndex = object == nullptr ? 0 : caretIndexAtPoint(*object, point);
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
    m_editCaretIndex = 0;
    m_editCreatedNow = true;
  }
  m_selectedId = m_editId;
  if (const auto* object = findById(m_editId)) {
    m_selectedBounds = object->bounds;
  }
  m_operationActive = false;
  m_activeHandle = Handle::None;
  m_editPreeditText.clear();
  m_editPreeditStartIndex = m_editCaretIndex;
  m_editSessionActive = true;
  return true;
}

bool TextEditorFeature::beginTextRangeSelectionAt(core::Point point) {
  const auto existing = hitTextIdAt(point);
  if (!existing.has_value()) {
    return false;
  }
  TextObject* object = findById(*existing);
  if (object == nullptr) {
    return false;
  }
  m_editId = *existing;
  m_editOriginalText = object->text;
  m_editCaretIndex = caretIndexAtPoint(*object, point);
  m_editSelectionAnchorIndex = m_editCaretIndex;
  m_editSelectionFocusIndex = m_editCaretIndex;
  m_editRangeSelectionActive = true;
  m_editCreatedNow = false;
  m_editPreeditText.clear();
  m_editPreeditStartIndex = m_editCaretIndex;
  m_editSessionActive = true;
  m_selectedId = object->id;
  m_selectedBounds = object->bounds;
  return true;
}

bool TextEditorFeature::updateTextRangeSelectionAt(core::Point point) {
  if (!m_editRangeSelectionActive || !m_editSessionActive) {
    return false;
  }
  TextObject* object = findById(m_editId);
  if (object == nullptr) {
    return false;
  }
  m_editSelectionFocusIndex = caretIndexAtPoint(*object, point);
  m_editCaretIndex = m_editSelectionFocusIndex;
  m_selectedId = object->id;
  m_selectedBounds = object->bounds;
  return true;
}

bool TextEditorFeature::endTextRangeSelection() {
  const bool wasActive = m_editRangeSelectionActive;
  m_editRangeSelectionActive = false;
  return wasActive;
}

bool TextEditorFeature::hasSelectedTextRange() const noexcept {
  return m_editSessionActive && m_editSelectionAnchorIndex != m_editSelectionFocusIndex;
}

std::optional<core::Rect> TextEditorFeature::selectedTextRangeRect() const {
  if (!hasSelectedTextRange()) {
    return std::nullopt;
  }

  const ObjectOverlayModel overlay = selectionOverlay();
  for (auto it = overlay.primitives.rbegin(); it != overlay.primitives.rend(); ++it) {
    if (it->kind == OverlayPrimitive::Kind::Rect) {
      return it->rect;
    }
  }
  return std::nullopt;
}


bool TextEditorFeature::handleKeyPress(int key, const std::string& textUtf8) {
  if (!m_editSessionActive) {
    return false;
  }
  TextObject* object = findById(m_editId);
  if (object == nullptr) {
    return false;
  }

  QString current = toQString(object->text);
  m_editCaretIndex = clampedCaretIndex(m_editCaretIndex, current);

  if (!m_editPreeditText.empty()) {
    const QString preedit = toQString(m_editPreeditText);
    const int preeditStart = clampedCaretIndex(m_editPreeditStartIndex, current);
    if (!preedit.isEmpty() && preeditStart + preedit.size() <= current.size() &&
        current.mid(preeditStart, preedit.size()) == preedit) {
      current.remove(preeditStart, preedit.size());
      object->text = toUtf8String(current);
      object->bounds = measureBounds(*object);
      m_selectedBounds = object->bounds;
    }
    m_editCaretIndex = clampedCaretIndex(preeditStart, current);
    m_editPreeditText.clear();
    m_editPreeditStartIndex = m_editCaretIndex;
  }

  if (key == Qt::Key_Return || key == Qt::Key_Enter) {
    object->bounds = measureBounds(*object);
    m_selectedBounds = object->bounds;
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
      m_editCaretIndex = clampedCaretIndex(m_editCaretIndex, toQString(object->text));
    }
    m_editSessionActive = false;
    m_editPreeditText.clear();
    m_editPreeditStartIndex = 0;
    return true;
  }
  if (key == Qt::Key_Left) {
    m_editCaretIndex = std::max(0, m_editCaretIndex - 1);
    return true;
  }
  if (key == Qt::Key_Right) {
    const int size = static_cast<int>(current.size());
    if (m_editCaretIndex < size) {
      m_editCaretIndex += 1;
    }
    return true;
  }
  if (key == Qt::Key_Home) {
    m_editCaretIndex = 0;
    return true;
  }
  if (key == Qt::Key_End) {
    m_editCaretIndex = static_cast<int>(current.size());
    return true;
  }
  if (key == Qt::Key_Backspace) {
    if (m_editCaretIndex > 0) {
      current.remove(m_editCaretIndex - 1, 1);
      m_editCaretIndex -= 1;
      object->text = toUtf8String(current);
      object->bounds = measureBounds(*object);
      m_selectedBounds = object->bounds;
    }
    return true;
  }
  if (key == Qt::Key_Delete) {
    if (m_editCaretIndex < current.size()) {
      current.remove(m_editCaretIndex, 1);
      object->text = toUtf8String(current);
      object->bounds = measureBounds(*object);
      m_selectedBounds = object->bounds;
    }
    return true;
  }

  if (!textUtf8.empty()) {
    const QString insertion = toQString(textUtf8);
    current.insert(m_editCaretIndex, insertion);
    m_editCaretIndex += insertion.size();
    object->text = toUtf8String(current);
    object->bounds = measureBounds(*object);
    m_selectedBounds = object->bounds;
    return true;
  }

  return false;
}

bool TextEditorFeature::handlePreeditText(const std::string& textUtf8) {
  if (!m_editSessionActive) {
    return false;
  }
  TextObject* object = findById(m_editId);
  if (object == nullptr) {
    return false;
  }

  QString current = toQString(object->text);
  const QString previousPreedit = toQString(m_editPreeditText);
  int preeditStart = clampedCaretIndex(m_editPreeditStartIndex, current);

  if (!previousPreedit.isEmpty() && preeditStart + previousPreedit.size() <= current.size() &&
      current.mid(preeditStart, previousPreedit.size()) == previousPreedit) {
    current.remove(preeditStart, previousPreedit.size());
  } else {
    preeditStart = clampedCaretIndex(m_editCaretIndex, current);
  }

  const QString nextPreedit = toQString(textUtf8);
  if (!nextPreedit.isEmpty()) {
    current.insert(preeditStart, nextPreedit);
    m_editCaretIndex = preeditStart + nextPreedit.size();
  } else {
    m_editCaretIndex = preeditStart;
  }

  m_editPreeditText = textUtf8;
  m_editPreeditStartIndex = preeditStart;
  object->text = toUtf8String(current);
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

bool TextEditorFeature::setSelectedTextColor(const core::Color& color) {
  if (!m_selectedId.has_value()) {
    return false;
  }
  auto* object = findById(*m_selectedId);
  if (object == nullptr) {
    return false;
  }
  object->color = color;
  return true;
}

bool TextEditorFeature::setSelectedTextFontSize(int fontSize) {
  if (!m_selectedId.has_value()) {
    return false;
  }
  auto* object = findById(*m_selectedId);
  if (object == nullptr) {
    return false;
  }
  object->fontSize = std::max(8, fontSize * 2);
  object->bounds = measureBounds(*object);
  m_selectedBounds = object->bounds;
  return true;
}

bool TextEditorFeature::setSelectedTextFontFamily(const std::string& fontFamily) {
  if (!m_selectedId.has_value()) {
    return false;
  }
  auto* object = findById(*m_selectedId);
  if (object == nullptr) {
    return false;
  }
  object->fontFamily = fontFamily;
  object->bounds = measureBounds(*object);
  m_selectedBounds = object->bounds;
  return true;
}

bool TextEditorFeature::setSelectedTextBold(bool enabled) {
  if (!m_selectedId.has_value()) {
    return false;
  }
  auto* object = findById(*m_selectedId);
  if (object == nullptr) {
    return false;
  }
  object->bold = enabled;
  object->bounds = measureBounds(*object);
  m_selectedBounds = object->bounds;
  return true;
}

bool TextEditorFeature::setSelectedTextItalic(bool enabled) {
  if (!m_selectedId.has_value()) {
    return false;
  }
  auto* object = findById(*m_selectedId);
  if (object == nullptr) {
    return false;
  }
  object->italic = enabled;
  object->bounds = measureBounds(*object);
  m_selectedBounds = object->bounds;
  return true;
}

bool TextEditorFeature::setSelectedTextUnderline(bool enabled) {
  if (!m_selectedId.has_value()) {
    return false;
  }
  auto* object = findById(*m_selectedId);
  if (object == nullptr) {
    return false;
  }
  object->underline = enabled;
  object->bounds = measureBounds(*object);
  m_selectedBounds = object->bounds;
  return true;
}

bool TextEditorFeature::setSelectedTextStrikeOut(bool enabled) {
  if (!m_selectedId.has_value()) {
    return false;
  }
  auto* object = findById(*m_selectedId);
  if (object == nullptr) {
    return false;
  }
  object->strikeOut = enabled;
  object->bounds = measureBounds(*object);
  m_selectedBounds = object->bounds;
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
  const core::Point t {b.x + b.width / 2, b.y};
  const core::Point tr {b.x + b.width, b.y};
  const core::Point l {b.x, b.y + b.height / 2};
  const core::Point r {b.x + b.width, b.y + b.height / 2};
  const core::Point bl {b.x, b.y + b.height};
  const core::Point btm {b.x + b.width / 2, b.y + b.height};
  const core::Point br {b.x + b.width, b.y + b.height};
  const core::Point rot {b.x + b.width / 2, b.y - 22};
  if (nearPoint(point, rot, 12)) return Handle::Rotate;
  if (nearPoint(point, tl)) return Handle::TL;
  if (nearPoint(point, t)) return Handle::T;
  if (nearPoint(point, tr)) return Handle::TR;
  if (nearPoint(point, l)) return Handle::L;
  if (nearPoint(point, r)) return Handle::R;
  if (nearPoint(point, bl)) return Handle::BL;
  if (nearPoint(point, btm)) return Handle::B;
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
    m_operationStartPoint = point;
    m_operationStartFontSize = object.fontSize;
    m_operationStartScaleX = object.scaleX;
    m_operationStartScaleY = object.scaleY;
    m_operationStartBounds = object.bounds;
    m_operationStartRotationDeg = object.rotationDeg;
    m_operationCenter = core::Point {object.bounds.x + object.bounds.width / 2, object.bounds.y + object.bounds.height / 2};
    m_operationStartPointerAngleDeg = angleDegFromCenter(m_operationCenter, point);
    m_operationFixedAnchor = boundsAnchorPoint(object.bounds, handle);
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
    const double currentAngle = angleDegFromCenter(m_operationCenter, point);
    object->rotationDeg = static_cast<float>(m_operationStartRotationDeg + currentAngle - m_operationStartPointerAngleDeg);
  } else if (m_activeHandle == Handle::T || m_activeHandle == Handle::B || m_activeHandle == Handle::L || m_activeHandle == Handle::R) {
    const int totalDx = point.x - m_operationStartPoint.x;
    const int totalDy = point.y - m_operationStartPoint.y;
    const int startWidth = std::max(1, m_operationStartBounds.width);
    const int startHeight = std::max(1, m_operationStartBounds.height);
    if (m_activeHandle == Handle::L) {
      const double targetWidth = std::max(8.0, static_cast<double>(startWidth - totalDx));
      object->scaleX = std::max(0.1F, static_cast<float>(m_operationStartScaleX * targetWidth / static_cast<double>(startWidth)));
      object->scaleY = m_operationStartScaleY;
    } else if (m_activeHandle == Handle::R) {
      const double targetWidth = std::max(8.0, static_cast<double>(startWidth + totalDx));
      object->scaleX = std::max(0.1F, static_cast<float>(m_operationStartScaleX * targetWidth / static_cast<double>(startWidth)));
      object->scaleY = m_operationStartScaleY;
    } else if (m_activeHandle == Handle::T) {
      const double targetHeight = std::max(8.0, static_cast<double>(startHeight - totalDy));
      object->scaleX = m_operationStartScaleX;
      object->scaleY = std::max(0.1F, static_cast<float>(m_operationStartScaleY * targetHeight / static_cast<double>(startHeight)));
    } else {
      const double targetHeight = std::max(8.0, static_cast<double>(startHeight + totalDy));
      object->scaleX = m_operationStartScaleX;
      object->scaleY = std::max(0.1F, static_cast<float>(m_operationStartScaleY * targetHeight / static_cast<double>(startHeight)));
    }
    object->bounds = measureBounds(*object);
    const core::Point currentAnchor = boundsAnchorPoint(object->bounds, m_activeHandle);
    object->position.x += m_operationFixedAnchor.x - currentAnchor.x;
    object->position.y += m_operationFixedAnchor.y - currentAnchor.y;
  } else {
    const int totalDx = point.x - m_operationStartPoint.x;
    const int totalDy = point.y - m_operationStartPoint.y;
    int delta = 0;
    if (m_activeHandle == Handle::TL) {
      delta = -totalDx - totalDy;
    } else if (m_activeHandle == Handle::TR) {
      delta = totalDx - totalDy;
    } else if (m_activeHandle == Handle::BL) {
      delta = -totalDx + totalDy;
    } else {
      delta = totalDx + totalDy;
    }
    const float uniform = std::max(0.1F, static_cast<float>(1.0 + static_cast<double>(delta) / 120.0));
    object->scaleX = std::max(0.1F, m_operationStartScaleX * uniform);
    object->scaleY = std::max(0.1F, m_operationStartScaleY * uniform);
    object->bounds = measureBounds(*object);
    const core::Point currentAnchor = boundsAnchorPoint(object->bounds, m_activeHandle);
    object->position.x += m_operationFixedAnchor.x - currentAnchor.x;
    object->position.y += m_operationFixedAnchor.y - currentAnchor.y;
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
  for (const core::Point& point : {core::Point {b.x, b.y}, core::Point {b.x + b.width / 2, b.y},
                                   core::Point {b.x + b.width, b.y}, core::Point {b.x, b.y + b.height / 2},
                                   core::Point {b.x + b.width, b.y + b.height / 2}, core::Point {b.x, b.y + b.height},
                                   core::Point {b.x + b.width / 2, b.y + b.height}, core::Point {b.x + b.width, b.y + b.height}}) {
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
      const QFont font = textFontForObject(*object);
      const QFontMetricsF metrics(font);
      const QString text = toQString(object->text);
      const int caretIndex = clampedCaretIndex(m_editCaretIndex, text);
      const qreal advance = std::max<qreal>(0.0, metrics.horizontalAdvance(text.left(caretIndex)));
      const qreal ascent = std::max<qreal>(1.0, metrics.ascent());
      const qreal descent = std::max<qreal>(1.0, metrics.descent());
      const double scaleX = std::max(0.1F, object->scaleX);
      const double scaleY = std::max(0.1F, object->scaleY);
      const double radians = static_cast<double>(object->rotationDeg) * 3.14159265358979323846 / 180.0;
      const double c = std::cos(radians);
      const double s = std::sin(radians);
      auto mapLocal = [&](double x, double y) -> core::Point {
        return core::Point {
            static_cast<int>(std::lround(static_cast<double>(object->position.x) + x * c - y * s)),
            static_cast<int>(std::lround(static_cast<double>(object->position.y) + x * s + y * c))};
      };
      /* text-editor-range-selection-overlay */
      if (hasSelectedTextRange()) {
        const int selectionStart = std::min(clampedCaretIndex(m_editSelectionAnchorIndex, text), clampedCaretIndex(m_editSelectionFocusIndex, text));
        const int selectionEnd = std::max(clampedCaretIndex(m_editSelectionAnchorIndex, text), clampedCaretIndex(m_editSelectionFocusIndex, text));
        const qreal leftAdvance = std::max<qreal>(0.0, metrics.horizontalAdvance(text.left(selectionStart)));
        const qreal rightAdvance = std::max<qreal>(leftAdvance, metrics.horizontalAdvance(text.left(selectionEnd)));
        const double selectionLeft = static_cast<double>(leftAdvance) * scaleX;
        const double selectionRight = static_cast<double>(rightAdvance) * scaleX;
        const double selectionTop = -static_cast<double>(ascent) * scaleY;
        const double selectionBottom = static_cast<double>(descent) * scaleY;
        const core::Point p1 = mapLocal(selectionLeft, selectionTop);
        const core::Point p2 = mapLocal(selectionRight, selectionTop);
        const core::Point p3 = mapLocal(selectionRight, selectionBottom);
        const core::Point p4 = mapLocal(selectionLeft, selectionBottom);
        const int minX = std::min(std::min(p1.x, p2.x), std::min(p3.x, p4.x));
        const int minY = std::min(std::min(p1.y, p2.y), std::min(p3.y, p4.y));
        const int maxX = std::max(std::max(p1.x, p2.x), std::max(p3.x, p4.x));
        const int maxY = std::max(std::max(p1.y, p2.y), std::max(p3.y, p4.y));
        OverlayPrimitive selection;
        selection.kind = OverlayPrimitive::Kind::Rect;
        selection.rect = core::Rect {minX, minY, std::max(1, maxX - minX), std::max(1, maxY - minY)};
        out.primitives.push_back(selection);
      }

      OverlayPrimitive caret;
      caret.kind = OverlayPrimitive::Kind::Line;
      const double caretX = static_cast<double>(advance) * scaleX;
      caret.p1 = mapLocal(caretX, -static_cast<double>(ascent) * scaleY);
      caret.p2 = mapLocal(caretX, static_cast<double>(descent) * scaleY);
      out.primitives.push_back(caret);
    }
  }
  return out;
}

} // namespace features::object_editing::text_editor

