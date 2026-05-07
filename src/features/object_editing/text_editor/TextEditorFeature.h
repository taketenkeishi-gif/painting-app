#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "core/common/Point.h"
#include "core/common/Rect.h"
#include "core/tools/ToolTypes.h"
#include "features/object_editing/ObjectOverlayRenderer.h"

namespace features::object_editing::text_editor {

class TextEditorFeature {
public:
  struct TextStyleRun {
    int start {0};
    int length {0};
    int fontSize {16};
    core::Color color {0, 0, 0, 255};
    std::string fontFamily;
    bool bold {false};
    bool italic {false};
    bool underline {false};
    bool strikeOut {false};
  };

  struct TextObject {
    std::string id;
    std::string text;
    core::Point position {0, 0}; // baseline origin
    int fontSize {16};
    core::Color color {0, 0, 0, 255};
    std::string fontFamily;
    bool bold {false};
    bool italic {false};
    bool underline {false};
    bool strikeOut {false};
    std::vector<TextStyleRun> styleRuns;
    float rotationDeg {0.0F};
    float scaleX {1.0F};
    float scaleY {1.0F};
    core::Rect bounds {0, 0, 1, 1};
    bool visible {true};
    bool locked {false};
  };

  bool beginTextInput(core::Point point, const core::Color& color, int fontSize);
  bool handleKeyPress(int key, const std::string& textUtf8);
  bool handlePreeditText(const std::string& textUtf8);
  bool beginTextRangeSelectionAt(core::Point point);
  bool updateTextRangeSelectionAt(core::Point point);
  bool endTextRangeSelection();
  bool hasActiveTextRangeSelection() const noexcept { return m_editRangeSelectionActive; }
  bool hasSelectedTextRange() const noexcept;
  std::optional<core::Rect> selectedTextRangeRect() const;
  bool hasActiveTextSession() const noexcept { return m_editSessionActive; }

  std::optional<std::string> hitTextIdAt(core::Point point) const;
  std::optional<std::string> textForId(const std::string& id) const;
  std::optional<core::Rect> boundsForId(const std::string& id) const;
  bool setTextForId(const std::string& id, const std::string& text);
  bool setSelectedTextColor(const core::Color& color);
  bool setSelectedTextFontSize(int fontSize);
  bool setSelectedTextFontFamily(const std::string& fontFamily);
  bool setSelectedTextBold(bool enabled);
  bool setSelectedTextItalic(bool enabled);
  bool setSelectedTextUnderline(bool enabled);
  bool setSelectedTextStrikeOut(bool enabled);
  bool removeById(const std::string& id);

  bool beginOperation(core::Point point);
  bool updateOperation(core::Point point);
  bool endOperation();
  bool hasActiveOperation() const noexcept { return m_operationActive; }

  const std::vector<TextObject>& objects() const noexcept { return m_objects; }
  std::optional<core::Rect> selectedBounds() const noexcept { return m_selectedBounds; }
  std::optional<std::string> selectedId() const noexcept { return m_selectedId; }
  ObjectOverlayModel selectionOverlay() const;

private:
  enum class Handle { None, Move, TL, T, TR, L, R, BL, B, BR, Rotate };

  TextObject* findById(const std::string& id);
  const TextObject* findById(const std::string& id) const;
  core::Rect measureBounds(const TextObject& object) const;
  int caretIndexAtPoint(const TextObject& object, core::Point point) const;
  core::Point boundsAnchorPoint(const core::Rect& bounds, Handle handle) const;
  Handle hitHandle(const TextObject& object, core::Point point) const;
  std::pair<int, int> selectedRangeBounds(int textLength) const;
  TextStyleRun styleAtIndex(const TextObject& object, int index) const;
  void applyStyleToSelectedTextRange(TextObject& object, const TextStyleRun& style);
  bool deleteSelectedTextRange(TextObject& object);
  void adjustStyleRunsAfterEdit(TextObject& object, int start, int removedLength, int insertedLength);
  void sanitizeStyleRuns(TextObject& object);

  std::vector<TextObject> m_objects;
  std::optional<std::string> m_selectedId;
  std::optional<core::Rect> m_selectedBounds;

  bool m_editSessionActive {false};
  bool m_editCreatedNow {false};
  std::string m_editId;
  std::string m_editOriginalText;
  int m_editCaretIndex {0};
  std::string m_editPreeditText;
  int m_editPreeditStartIndex {0};
  bool m_editRangeSelectionActive {false};
  int m_editSelectionAnchorIndex {0};
  int m_editSelectionFocusIndex {0};

  bool m_operationActive {false};
  Handle m_activeHandle {Handle::None};
  core::Point m_lastPoint {0, 0};
  core::Point m_operationCenter {0, 0};
  core::Point m_operationStartPoint {0, 0};
  core::Point m_operationFixedAnchor {0, 0};
  int m_operationStartFontSize {16};
  float m_operationStartScaleX {1.0F};
  float m_operationStartScaleY {1.0F};
  core::Rect m_operationStartBounds {0, 0, 1, 1};
  float m_operationStartRotationDeg {0.0F};
  double m_operationStartPointerAngleDeg {0.0};
};

} // namespace features::object_editing::text_editor
