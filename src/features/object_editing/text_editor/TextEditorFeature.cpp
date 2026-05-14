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

QFont textFontForStyleRun(const TextEditorFeature::TextStyleRun& style) {
  QFont font = baseTextFont(style.fontSize);
  if (!style.fontFamily.empty()) {
    font.setFamily(QString::fromUtf8(style.fontFamily.data(), static_cast<int>(style.fontFamily.size())));
  }
  font.setBold(style.bold);
  font.setItalic(style.italic);
  font.setUnderline(style.underline);
  font.setStrikeOut(style.strikeOut);
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
  const QString text = toQString(object.text);
  const QString measured = text.isEmpty() ? QStringLiteral(" ") : text;
  const qreal sx = std::max<qreal>(0.1, object.scaleX);
  const qreal sy = std::max<qreal>(0.1, object.scaleY);

  QRectF localRect;
  if (object.vertical) {
    // Vertical multi-column: \n = 次列へ
    const float ls = std::max(0.5f, object.lineSpacing);
    const QFont baseFont = textFontForObject(object);
    QFontMetricsF bfm(baseFont);
    const qreal colWidth = std::max(16.0, bfm.ascent() + bfm.descent());

    int numCols = 1;
    qreal maxColH = 0.0;
    qreal currentColH = 0.0;
    for (int i = 0; i < measured.size(); ++i) {
      if (measured[i] == QChar('\n')) {
        maxColH = std::max(maxColH, currentColH);
        currentColH = 0.0;
        ++numCols;
        continue;
      }
      const TextStyleRun style = styleAtIndex(object, i);
      QFontMetricsF m(textFontForStyleRun(style));
      currentColH += (m.ascent() + m.descent()) * static_cast<qreal>(ls);
    }
    maxColH = std::max(maxColH, currentColH);
    if (maxColH < 16.0) maxColH = 16.0;

    const qreal totalWidth = static_cast<qreal>(numCols) * colWidth;
    if (object.verticalRTL) {
      localRect = QRectF(-totalWidth * sx, 0.0, totalWidth * sx, maxColH * sy);
    } else {
      localRect = QRectF(0.0, 0.0, totalWidth * sx, maxColH * sy);
    }
  } else {
    // Horizontal multi-line: split by \n and compute per-line metrics
    qreal maxLineWidth = 16.0;
    qreal firstLineAscent = 1.0;
    qreal lastLineAscent = 1.0;
    qreal cumY = 0.0; // accumulates lineH for each line; final value = sum of all lineH
    bool firstLine = true;
    int lineStart = 0;
    while (lineStart <= measured.size()) {
      int lineEnd = lineStart;
      while (lineEnd < measured.size() && measured[lineEnd] != QChar('\n')) {
        ++lineEnd;
      }
      qreal lineW = 0.0;
      qreal lineAscent = 1.0;
      qreal lineDescent = 1.0;
      if (object.styleRuns.empty()) {
        const QFont font = textFontForObject(object);
        QFontMetricsF metrics(font);
        const QString lineText = (lineStart == lineEnd) ? QStringLiteral(" ") : measured.mid(lineStart, lineEnd - lineStart);
        lineW = std::max<qreal>(16.0, metrics.horizontalAdvance(lineText));
        lineAscent = std::max<qreal>(1.0, metrics.ascent());
        lineDescent = std::max<qreal>(1.0, metrics.descent());
      } else {
        for (int i = lineStart; i < lineEnd; ++i) {
          const TextStyleRun style = styleAtIndex(object, i);
          QFontMetricsF metrics(textFontForStyleRun(style));
          lineW += metrics.horizontalAdvance(measured.mid(i, 1));
          lineAscent = std::max<qreal>(lineAscent, metrics.ascent());
          lineDescent = std::max<qreal>(lineDescent, metrics.descent());
        }
        if (lineStart == lineEnd) {
          QFontMetricsF metrics(textFontForObject(object));
          lineAscent = std::max<qreal>(lineAscent, metrics.ascent());
          lineDescent = std::max<qreal>(lineDescent, metrics.descent());
        }
        lineW = std::max<qreal>(16.0, lineW);
      }
      maxLineWidth = std::max(maxLineWidth, lineW);
      if (firstLine) {
        firstLineAscent = lineAscent;
        firstLine = false;
      }
      lastLineAscent = lineAscent;
      cumY += lineAscent + lineDescent;
      lineStart = lineEnd + 1;
    }
    // top = -firstLineAscent, height = (cumY - lastLineAscent + firstLineAscent)
    const qreal rectHeight = (cumY - lastLineAscent + firstLineAscent) * sy;
    localRect = QRectF(0.0, -firstLineAscent * sy, maxLineWidth * sx, rectHeight);
  }

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

  if (object.vertical) {
    // Vertical multi-column: \n = 次列へ
    // Qt clockwise rotation: localX = dx*c - dy*s, localY = dx*s + dy*c
    const double localX = (dx * c - dy * s) / std::max(0.1F, object.scaleX);
    const double localY = (dx * s + dy * c) / std::max(0.1F, object.scaleY);
    const double ls = static_cast<double>(std::max(0.5f, object.lineSpacing));
    const QFont baseFont = textFontForObject(object);
    QFontMetricsF bfm(baseFont);
    const double colWidth = std::max(16.0, bfm.ascent() + bfm.descent());

    // クリック位置の列インデックスを決定
    int targetCol;
    if (object.verticalRTL) {
      // col n の中心 X = -n * colWidth
      targetCol = std::max(0, static_cast<int>((-localX + colWidth / 2.0) / colWidth));
    } else {
      targetCol = std::max(0, static_cast<int>((localX + colWidth / 2.0) / colWidth));
    }

    // 対象列の文字を順に比較
    int currentCol = 0;
    double accumY = 0.0;
    for (int i = 0; i <= text.size(); ++i) {
      if (i == text.size()) return i;
      if (text[i] == QChar('\n')) {
        if (currentCol == targetCol) return i; // 列末尾
        ++currentCol;
        accumY = 0.0;
        continue;
      }
      const TextStyleRun style = styleAtIndex(object, i);
      QFontMetricsF metrics(textFontForStyleRun(style));
      const double step = (metrics.ascent() + metrics.descent()) * ls;
      if (currentCol == targetCol && localY <= accumY + step / 2.0) return i;
      if (currentCol == targetCol) accumY += step;
    }
    return static_cast<int>(text.size());
  }

  const double localX = (dx * c + dy * s) / std::max(0.1F, object.scaleX);
  const double localY = (-dx * s + dy * c) / std::max(0.1F, object.scaleY);

  // Multi-line horizontal: find which line by localY, then find X within that line
  auto getLineH = [&](int ls, int le) -> qreal {
    qreal la = 1.0, ld = 1.0;
    if (object.styleRuns.empty()) {
      QFontMetricsF m(textFontForObject(object));
      return std::max(1.0, m.ascent()) + std::max(1.0, m.descent());
    }
    for (int i = ls; i < le; ++i) {
      QFontMetricsF m(textFontForStyleRun(styleAtIndex(object, i)));
      la = std::max(la, m.ascent());
      ld = std::max(ld, m.descent());
    }
    if (ls == le) {
      QFontMetricsF m(textFontForObject(object));
      la = std::max(la, m.ascent());
      ld = std::max(ld, m.descent());
    }
    return la + ld;
  };

  // Scan lines until the target is found by Y comparison
  int targetLineStart = 0;
  int targetLineEnd = 0;
  {
    int ls = 0;
    qreal cumY = 0.0;
    while (true) {
      int le = ls;
      while (le < text.size() && text[le] != QChar('\n')) ++le;
      const qreal lh = getLineH(ls, le);
      targetLineStart = ls;
      targetLineEnd = le;
      if (le >= text.size() || localY < cumY + lh) {
        break;
      }
      cumY += lh;
      ls = le + 1;
    }
  }

  // Find X position within the target line
  int bestIndex = targetLineStart;
  double advance = 0.0;
  double bestDistance = std::abs(localX);
  for (int i = targetLineStart; i < targetLineEnd && i < text.size(); ++i) {
    const TextStyleRun style = styleAtIndex(object, i);
    QFontMetricsF metrics(textFontForStyleRun(style));
    advance += metrics.horizontalAdvance(text.mid(i, 1));
    const double distance = std::abs(localX - advance);
    if (distance < bestDistance) {
      bestDistance = distance;
      bestIndex = i + 1;
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

bool TextEditorFeature::beginTextInput(core::Point point, const core::Color& color, int fontSize, bool vertical) {
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
    object.fontSize = std::clamp(fontSize, 8, 24);
    object.color = color;
    object.vertical = vertical;
    object.lineSpacing = vertical ? 0.8f : 1.0f;
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

bool TextEditorFeature::selectAll() noexcept {
  if (!m_editSessionActive) {
    return false;
  }
  const TextObject* object = findById(m_editId);
  if (object == nullptr) {
    return false;
  }
  const int len = static_cast<int>(toQString(object->text).size());
  m_editSelectionAnchorIndex = 0;
  m_editSelectionFocusIndex  = len;
  m_editCaretIndex           = len;
  return true;
}

bool TextEditorFeature::extendSelectionLeft() noexcept {
  if (!m_editSessionActive) return false;
  const TextObject* obj = findById(m_editId);
  if (!obj) return false;
  if (m_editCaretIndex > 0) m_editCaretIndex -= 1;
  m_editSelectionFocusIndex = m_editCaretIndex;
  return true;
}

bool TextEditorFeature::extendSelectionRight() noexcept {
  if (!m_editSessionActive) return false;
  const TextObject* obj = findById(m_editId);
  if (!obj) return false;
  const int len = static_cast<int>(toQString(obj->text).size());
  if (m_editCaretIndex < len) m_editCaretIndex += 1;
  m_editSelectionFocusIndex = m_editCaretIndex;
  return true;
}

TextEditorFeature::TextStyleRun TextEditorFeature::caretStyle() const noexcept {
  if (!m_editSessionActive) return {};
  const TextObject* obj = findById(m_editId);
  if (!obj) return {};
  const int idx = m_editCaretIndex > 0 ? m_editCaretIndex - 1 : 0;
  return styleAtIndex(*obj, idx);
}

bool TextEditorFeature::toggleVertical() noexcept {
  if (!m_selectedId.has_value()) return false;
  TextObject* obj = findById(*m_selectedId);
  if (!obj) return false;
  obj->vertical = !obj->vertical;
  obj->bounds = measureBounds(*obj);
  m_selectedBounds = obj->bounds;
  return true;
}

bool TextEditorFeature::isVertical() const noexcept {
  if (!m_selectedId.has_value()) return false;
  const TextObject* obj = findById(*m_selectedId);
  return obj ? obj->vertical : false;
}

bool TextEditorFeature::setVerticalRTL(bool rtl) noexcept {
  if (!m_selectedId.has_value()) return false;
  TextObject* obj = findById(*m_selectedId);
  if (!obj) return false;
  obj->verticalRTL = rtl;
  obj->bounds = measureBounds(*obj);
  m_selectedBounds = obj->bounds;
  return true;
}

bool TextEditorFeature::getVerticalRTL() const noexcept {
  if (!m_selectedId.has_value()) return true;
  const TextObject* obj = findById(*m_selectedId);
  return obj ? obj->verticalRTL : true;
}

bool TextEditorFeature::setLineSpacing(float spacing) {
  if (!m_selectedId.has_value()) return false;
  TextObject* obj = findById(*m_selectedId);
  if (!obj) return false;
  obj->lineSpacing = std::max(0.5f, spacing);
  obj->bounds = measureBounds(*obj);
  m_selectedBounds = obj->bounds;
  return true;
}

float TextEditorFeature::getLineSpacing() const noexcept {
  if (!m_selectedId.has_value()) return 1.0f;
  const TextObject* obj = findById(*m_selectedId);
  return obj ? obj->lineSpacing : 1.0f;
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

std::pair<int, int> TextEditorFeature::selectedRangeBounds(int textLength) const {
  const int a = std::clamp(m_editSelectionAnchorIndex, 0, std::max(0, textLength));
  const int b = std::clamp(m_editSelectionFocusIndex, 0, std::max(0, textLength));
  return {std::min(a, b), std::max(a, b)};
}

TextEditorFeature::TextStyleRun TextEditorFeature::styleAtIndex(const TextObject& object, int index) const {
  TextStyleRun style;
  style.start = std::max(0, index);
  style.length = 0;
  style.fontSize = object.fontSize;
  style.color = object.color;
  style.fontFamily = object.fontFamily;
  style.bold = object.bold;
  style.italic = object.italic;
  style.underline = object.underline;
  style.strikeOut = object.strikeOut;
  for (const TextStyleRun& run : object.styleRuns) {
    if (index >= run.start && index < run.start + run.length) {
      style.fontSize = run.fontSize;
      style.color = run.color;
      style.fontFamily = run.fontFamily;
      style.bold = run.bold;
      style.italic = run.italic;
      style.underline = run.underline;
      style.strikeOut = run.strikeOut;
    }
  }
  return style;
}

void TextEditorFeature::sanitizeStyleRuns(TextObject& object) {
  const int textLength = static_cast<int>(toQString(object.text).size());
  for (TextStyleRun& run : object.styleRuns) {
    run.start = std::clamp(run.start, 0, std::max(0, textLength));
    run.length = std::clamp(run.length, 0, std::max(0, textLength - run.start));
    run.fontSize = std::max(8, run.fontSize);
  }
  object.styleRuns.erase(
      std::remove_if(object.styleRuns.begin(), object.styleRuns.end(), [](const TextStyleRun& run) {
        return run.length <= 0;
      }),
      object.styleRuns.end());
}

void TextEditorFeature::adjustStyleRunsAfterEdit(TextObject& object, int start, int removedLength, int insertedLength) {
  start = std::max(0, start);
  removedLength = std::max(0, removedLength);
  insertedLength = std::max(0, insertedLength);

  if (removedLength > 0) {
    const int removedEnd = start + removedLength;
    for (TextStyleRun& run : object.styleRuns) {
      const int runEnd = run.start + run.length;
      if (runEnd <= start) {
        continue;
      }
      if (run.start >= removedEnd) {
        run.start -= removedLength;
        continue;
      }
      const int before = std::max(0, start - run.start);
      const int after = std::max(0, runEnd - removedEnd);
      run.start = std::min(run.start, start);
      run.length = before + after;
    }
  }

  if (insertedLength > 0) {
    for (TextStyleRun& run : object.styleRuns) {
      const int runEnd = run.start + run.length;
      if (run.start >= start) {
        run.start += insertedLength;
      } else if (runEnd > start) {
        run.length += insertedLength;
      }
    }
  }

  sanitizeStyleRuns(object);
}

void TextEditorFeature::applyStyleToSelectedTextRange(TextObject& object, const TextStyleRun& style) {
  const int textLength = static_cast<int>(toQString(object.text).size());
  const auto range = selectedRangeBounds(textLength);
  if (range.first == range.second) {
    return;
  }
  TextStyleRun run = style;
  run.start = range.first;
  run.length = range.second - range.first;
  run.fontSize = std::max(8, run.fontSize);
  object.styleRuns.push_back(run);
  sanitizeStyleRuns(object);
  object.bounds = measureBounds(object);
  m_selectedBounds = object.bounds;
}

bool TextEditorFeature::deleteSelectedTextRange(TextObject& object) {
  if (!hasSelectedTextRange()) {
    return false;
  }
  QString current = toQString(object.text);
  const auto range = selectedRangeBounds(static_cast<int>(current.size()));
  if (range.first == range.second) {
    return false;
  }
  const int removedLength = range.second - range.first;
  current.remove(range.first, removedLength);
  object.text = toUtf8String(current);
  adjustStyleRunsAfterEdit(object, range.first, removedLength, 0);
  m_editCaretIndex = range.first;
  m_editSelectionAnchorIndex = range.first;
  m_editSelectionFocusIndex = range.first;
  m_editRangeSelectionActive = false;
  object.bounds = measureBounds(object);
  m_selectedBounds = object.bounds;
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

  QString current = toQString(object->text);
  m_editCaretIndex = clampedCaretIndex(m_editCaretIndex, current);

  if (!m_editPreeditText.empty()) {
    const QString preedit = toQString(m_editPreeditText);
    const int preeditStart = clampedCaretIndex(m_editPreeditStartIndex, current);
    if (!preedit.isEmpty() && preeditStart + preedit.size() <= current.size() &&
        current.mid(preeditStart, preedit.size()) == preedit) {
      current.remove(preeditStart, preedit.size());
      object->text = toUtf8String(current);
      adjustStyleRunsAfterEdit(*object, preeditStart, preedit.size(), 0);
      object->bounds = measureBounds(*object);
      m_selectedBounds = object->bounds;
    }
    m_editCaretIndex = clampedCaretIndex(preeditStart, current);
    m_editPreeditText.clear();
    m_editPreeditStartIndex = m_editCaretIndex;
  }

  if (key == Qt::Key_Return || key == Qt::Key_Enter) {
    // Key_Return = 確定（セッション終了）。改行挿入は handleTextSessionKey(0, "\n") 経由
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
    if (hasSelectedTextRange()) {
      const auto range = selectedRangeBounds(static_cast<int>(current.size()));
      m_editCaretIndex = range.first;
      m_editSelectionAnchorIndex = m_editCaretIndex;
      m_editSelectionFocusIndex = m_editCaretIndex;
      m_editRangeSelectionActive = false;
      return true;
    }
    m_editCaretIndex = std::max(0, m_editCaretIndex - 1);
    return true;
  }
  if (key == Qt::Key_Right) {
    if (hasSelectedTextRange()) {
      const auto range = selectedRangeBounds(static_cast<int>(current.size()));
      m_editCaretIndex = range.second;
      m_editSelectionAnchorIndex = m_editCaretIndex;
      m_editSelectionFocusIndex = m_editCaretIndex;
      m_editRangeSelectionActive = false;
      return true;
    }
    const int size = static_cast<int>(current.size());
    if (m_editCaretIndex < size) {
      m_editCaretIndex += 1;
    }
    return true;
  }
  if (key == Qt::Key_Home) {
    m_editCaretIndex = 0;
    m_editSelectionAnchorIndex = m_editCaretIndex;
    m_editSelectionFocusIndex = m_editCaretIndex;
    m_editRangeSelectionActive = false;
    return true;
  }
  if (key == Qt::Key_End) {
    m_editCaretIndex = static_cast<int>(current.size());
    m_editSelectionAnchorIndex = m_editCaretIndex;
    m_editSelectionFocusIndex = m_editCaretIndex;
    m_editRangeSelectionActive = false;
    return true;
  }
  if (key == Qt::Key_Backspace) {
    if (deleteSelectedTextRange(*object)) {
      return true;
    }
    current = toQString(object->text);
    if (m_editCaretIndex > 0) {
      current.remove(m_editCaretIndex - 1, 1);
      adjustStyleRunsAfterEdit(*object, m_editCaretIndex - 1, 1, 0);
      m_editCaretIndex -= 1;
      object->text = toUtf8String(current);
      object->bounds = measureBounds(*object);
      m_selectedBounds = object->bounds;
    }
    return true;
  }
  if (key == Qt::Key_Delete) {
    if (deleteSelectedTextRange(*object)) {
      return true;
    }
    current = toQString(object->text);
    if (m_editCaretIndex < current.size()) {
      current.remove(m_editCaretIndex, 1);
      adjustStyleRunsAfterEdit(*object, m_editCaretIndex, 1, 0);
      object->text = toUtf8String(current);
      object->bounds = measureBounds(*object);
      m_selectedBounds = object->bounds;
    }
    return true;
  }

  if (!textUtf8.empty()) {
    if (deleteSelectedTextRange(*object)) {
      current = toQString(object->text);
    } else {
      current = toQString(object->text);
    }
    m_editCaretIndex = clampedCaretIndex(m_editCaretIndex, current);
    const QString insertion = toQString(textUtf8);
    const int insertAt = m_editCaretIndex;
    current.insert(insertAt, insertion);
    adjustStyleRunsAfterEdit(*object, insertAt, 0, insertion.size());
    m_editCaretIndex += insertion.size();
    m_editSelectionAnchorIndex = m_editCaretIndex;
    m_editSelectionFocusIndex = m_editCaretIndex;
    m_editRangeSelectionActive = false;
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

  if (hasSelectedTextRange() && m_editPreeditText.empty()) {
    deleteSelectedTextRange(*object);
  }

  QString current = toQString(object->text);
  const QString previousPreedit = toQString(m_editPreeditText);
  int preeditStart = clampedCaretIndex(m_editPreeditStartIndex, current);

  if (!previousPreedit.isEmpty() && preeditStart + previousPreedit.size() <= current.size() &&
      current.mid(preeditStart, previousPreedit.size()) == previousPreedit) {
    current.remove(preeditStart, previousPreedit.size());
    object->text = toUtf8String(current);
    adjustStyleRunsAfterEdit(*object, preeditStart, previousPreedit.size(), 0);
    current = toQString(object->text);
  } else {
    preeditStart = clampedCaretIndex(m_editCaretIndex, current);
  }

  const QString nextPreedit = toQString(textUtf8);
  if (!nextPreedit.isEmpty()) {
    current.insert(preeditStart, nextPreedit);
    adjustStyleRunsAfterEdit(*object, preeditStart, 0, nextPreedit.size());
    m_editCaretIndex = preeditStart + nextPreedit.size();
  } else {
    m_editCaretIndex = preeditStart;
  }

  m_editSelectionAnchorIndex = m_editCaretIndex;
  m_editSelectionFocusIndex = m_editCaretIndex;
  m_editRangeSelectionActive = false;
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
  object->styleRuns.clear();
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
  if (hasSelectedTextRange()) {
    TextStyleRun style = styleAtIndex(*object, selectedRangeBounds(static_cast<int>(toQString(object->text).size())).first);
    style.color = color;
    applyStyleToSelectedTextRange(*object, style);
    return true;
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
  if (hasSelectedTextRange()) {
    TextStyleRun style = styleAtIndex(*object, selectedRangeBounds(static_cast<int>(toQString(object->text).size())).first);
    style.fontSize = std::max(8, fontSize * 2);
    applyStyleToSelectedTextRange(*object, style);
    return true;
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
  if (hasSelectedTextRange()) {
    TextStyleRun style = styleAtIndex(*object, selectedRangeBounds(static_cast<int>(toQString(object->text).size())).first);
    style.fontFamily = fontFamily;
    applyStyleToSelectedTextRange(*object, style);
    return true;
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
  if (hasSelectedTextRange()) {
    TextStyleRun style = styleAtIndex(*object, selectedRangeBounds(static_cast<int>(toQString(object->text).size())).first);
    style.bold = enabled;
    applyStyleToSelectedTextRange(*object, style);
    return true;
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
  if (hasSelectedTextRange()) {
    TextStyleRun style = styleAtIndex(*object, selectedRangeBounds(static_cast<int>(toQString(object->text).size())).first);
    style.italic = enabled;
    applyStyleToSelectedTextRange(*object, style);
    return true;
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
  if (hasSelectedTextRange()) {
    TextStyleRun style = styleAtIndex(*object, selectedRangeBounds(static_cast<int>(toQString(object->text).size())).first);
    style.underline = enabled;
    applyStyleToSelectedTextRange(*object, style);
    return true;
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
  if (hasSelectedTextRange()) {
    TextStyleRun style = styleAtIndex(*object, selectedRangeBounds(static_cast<int>(toQString(object->text).size())).first);
    style.strikeOut = enabled;
    applyStyleToSelectedTextRange(*object, style);
    return true;
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
      const QString text = toQString(object->text);
      const int caretIndex = clampedCaretIndex(m_editCaretIndex, text);
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
      if (object->vertical) {
        // ---- VERTICAL mode (multi-column) ----
        const double ls = static_cast<double>(std::max(0.5f, object->lineSpacing));
        const QFont baseFont = textFontForObject(*object);
        QFontMetricsF bfm(baseFont);
        const qreal colWidth = std::max(16.0, bfm.ascent() + bfm.descent());

        // \n をスキップして (列, 列内Y) を返す
        auto charColAndY = [&](int idx) -> std::pair<int, qreal> {
          int col = 0;
          qreal y = 0.0;
          for (int i = 0; i < idx && i < text.size(); ++i) {
            if (text[i] == QChar('\n')) { ++col; y = 0.0; continue; }
            QFontMetricsF m(textFontForStyleRun(styleAtIndex(*object, i)));
            y += (m.ascent() + m.descent()) * ls;
          }
          return {col, y};
        };
        auto colCenterLocalX = [&](int col) -> qreal {
          return object->verticalRTL ? -(static_cast<qreal>(col) * colWidth)
                                     : +(static_cast<qreal>(col) * colWidth);
        };

        /* text-editor-range-selection-overlay (vertical) */
        if (hasSelectedTextRange()) {
          const int selStart = std::min(clampedCaretIndex(m_editSelectionAnchorIndex, text), clampedCaretIndex(m_editSelectionFocusIndex, text));
          const int selEnd   = std::max(clampedCaretIndex(m_editSelectionAnchorIndex, text), clampedCaretIndex(m_editSelectionFocusIndex, text));
          bool hasVisual = false;
          qreal minLX = 1e9, maxLX = -1e9, minLY = 1e9, maxLY = -1e9;
          for (int i = selStart; i < selEnd && i < text.size(); ++i) {
            if (text[i] == QChar('\n')) continue;
            const auto [col, localY] = charColAndY(i);
            QFontMetricsF m(textFontForStyleRun(styleAtIndex(*object, i)));
            const qreal step = (m.ascent() + m.descent()) * ls;
            const qreal cx = colCenterLocalX(col);
            minLX = std::min(minLX, cx - colWidth / 2.0);
            maxLX = std::max(maxLX, cx + colWidth / 2.0);
            minLY = std::min(minLY, localY);
            maxLY = std::max(maxLY, localY + step);
            hasVisual = true;
          }
          if (hasVisual) {
            const core::Point p1 = mapLocal(minLX * scaleX, minLY * scaleY);
            const core::Point p2 = mapLocal(maxLX * scaleX, minLY * scaleY);
            const core::Point p3 = mapLocal(maxLX * scaleX, maxLY * scaleY);
            const core::Point p4 = mapLocal(minLX * scaleX, maxLY * scaleY);
            const int minX = std::min(std::min(p1.x, p2.x), std::min(p3.x, p4.x));
            const int minY = std::min(std::min(p1.y, p2.y), std::min(p3.y, p4.y));
            const int maxX = std::max(std::max(p1.x, p2.x), std::max(p3.x, p4.x));
            const int maxY = std::max(std::max(p1.y, p2.y), std::max(p3.y, p4.y));
            OverlayPrimitive selection;
            selection.kind = OverlayPrimitive::Kind::Rect;
            selection.rect = core::Rect {minX, minY, std::max(1, maxX - minX), std::max(1, maxY - minY)};
            out.primitives.push_back(selection);
          }
        }
        // Caret: 列対応 横線
        const auto [caretCol, caretLocalY] = charColAndY(caretIndex);
        const qreal cx = colCenterLocalX(caretCol) * scaleX;
        const qreal cy = caretLocalY * scaleY;
        const qreal halfW = colWidth / 2.0 * scaleX;
        OverlayPrimitive caret;
        caret.kind = OverlayPrimitive::Kind::Line;
        caret.p1 = mapLocal(cx - halfW, cy);
        caret.p2 = mapLocal(cx + halfW, cy);
        out.primitives.push_back(caret);
      } else {
        // ---- HORIZONTAL mode (multi-line) ----
        struct HLineInfo { int start; int end; qreal baseline; qreal ascent; qreal descent; };
        constexpr int kMaxLines = 64;
        HLineInfo lines[kMaxLines];
        int lineCount = 0;
        {
          int ls = 0;
          qreal cumY = 0.0;
          while (ls <= text.size() && lineCount < kMaxLines) {
            int le = ls;
            while (le < text.size() && text[le] != QChar('\n')) ++le;
            qreal la = 1.0, ld = 1.0;
            if (object->styleRuns.empty()) {
              QFontMetricsF m(textFontForObject(*object));
              la = m.ascent(); ld = m.descent();
            } else {
              for (int i = ls; i < le; ++i) {
                QFontMetricsF m(textFontForStyleRun(styleAtIndex(*object, i)));
                la = std::max(la, m.ascent());
                ld = std::max(ld, m.descent());
              }
              if (ls == le) {
                QFontMetricsF m(textFontForObject(*object));
                la = std::max(la, m.ascent());
                ld = std::max(ld, m.descent());
              }
            }
            lines[lineCount++] = {ls, le, cumY, la, ld};
            cumY += la + ld;
            ls = le + 1;
          }
        }
        if (lineCount == 0) {
          QFontMetricsF m(textFontForObject(*object));
          lines[0] = {0, 0, 0.0, m.ascent(), m.descent()};
          lineCount = 1;
        }

        // Get (lineIdx, xAdvance within that line) for a given char index
        auto posForChar = [&](int idx) -> std::pair<int, qreal> {
          for (int li = 0; li < lineCount; ++li) {
            if (idx >= lines[li].start && (idx <= lines[li].end || li == lineCount - 1)) {
              qreal adv = 0.0;
              for (int i = lines[li].start; i < idx && i < lines[li].end; ++i) {
                QFontMetricsF m(textFontForStyleRun(styleAtIndex(*object, i)));
                adv += m.horizontalAdvance(text.mid(i, 1));
              }
              return {li, adv};
            }
          }
          return {lineCount - 1, 0.0};
        };

        // Full line width (for spanning-line selection)
        auto fullLineWidth = [&](int li) -> qreal {
          qreal adv = 0.0;
          for (int i = lines[li].start; i < lines[li].end; ++i) {
            QFontMetricsF m(textFontForStyleRun(styleAtIndex(*object, i)));
            adv += m.horizontalAdvance(text.mid(i, 1));
          }
          return adv + 2.0;
        };

        /* text-editor-range-selection-overlay */
        if (hasSelectedTextRange()) {
          const int selStart = std::min(clampedCaretIndex(m_editSelectionAnchorIndex, text), clampedCaretIndex(m_editSelectionFocusIndex, text));
          const int selEnd = std::max(clampedCaretIndex(m_editSelectionAnchorIndex, text), clampedCaretIndex(m_editSelectionFocusIndex, text));
          const auto [startLine, startX] = posForChar(selStart);
          const auto [endLine, endX] = posForChar(selEnd);
          for (int li = startLine; li <= endLine && li < lineCount; ++li) {
            const qreal xL = (li == startLine ? startX : 0.0) * scaleX;
            const qreal xR = (li == endLine ? endX : fullLineWidth(li)) * scaleX;
            const qreal yT = (lines[li].baseline - lines[li].ascent) * scaleY;
            const qreal yB = (lines[li].baseline + lines[li].descent) * scaleY;
            const core::Point p1 = mapLocal(xL, yT);
            const core::Point p2 = mapLocal(xR, yT);
            const core::Point p3 = mapLocal(xR, yB);
            const core::Point p4 = mapLocal(xL, yB);
            const int minX = std::min(std::min(p1.x, p2.x), std::min(p3.x, p4.x));
            const int minY = std::min(std::min(p1.y, p2.y), std::min(p3.y, p4.y));
            const int maxX = std::max(std::max(p1.x, p2.x), std::max(p3.x, p4.x));
            const int maxY = std::max(std::max(p1.y, p2.y), std::max(p3.y, p4.y));
            OverlayPrimitive selection;
            selection.kind = OverlayPrimitive::Kind::Rect;
            selection.rect = core::Rect {minX, minY, std::max(1, maxX - minX), std::max(1, maxY - minY)};
            out.primitives.push_back(selection);
          }
        }
        // Caret
        const auto [caretLine, caretXAdv] = posForChar(caretIndex);
        const HLineInfo& cl = lines[std::min(caretLine, lineCount - 1)];
        const int caretFontIdx = caretIndex < text.size() ? caretIndex : std::max(0, caretIndex - 1);
        const QFont caretFont = text.isEmpty()
                                ? textFontForObject(*object)
                                : textFontForStyleRun(styleAtIndex(*object, caretFontIdx));
        QFontMetricsF caretM(caretFont);
        OverlayPrimitive caret;
        caret.kind = OverlayPrimitive::Kind::Line;
        const double caretX = caretXAdv * scaleX;
        const double caretTop = (cl.baseline - std::max<qreal>(1.0, caretM.ascent())) * scaleY;
        const double caretBottom = (cl.baseline + std::max<qreal>(1.0, caretM.descent())) * scaleY;
        caret.p1 = mapLocal(caretX, caretTop);
        caret.p2 = mapLocal(caretX, caretBottom);
        out.primitives.push_back(caret);
      }
    }
  }
  return out;
}

} // namespace features::object_editing::text_editor

