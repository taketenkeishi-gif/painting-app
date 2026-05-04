#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/common/Point.h"
#include "core/common/Rect.h"
#include "core/tools/ToolTypes.h"
#include "features/object_editing/ObjectOverlayRenderer.h"

namespace features::object_editing::text_editor {

class TextEditorFeature {
public:
  struct TextObject {
    std::string id;
    std::string text;
    core::Point position {0, 0}; // baseline origin
    int fontSize {16};
    core::Color color {0, 0, 0, 255};
    float rotationDeg {0.0F};
    float scaleX {1.0F};
    float scaleY {1.0F};
    core::Rect bounds {0, 0, 1, 1};
    bool visible {true};
    bool locked {false};
  };

  bool beginTextInput(core::Point point, const core::Color& color, int fontSize);
  bool handleKeyPress(int key, const std::string& textUtf8);
  bool hasActiveTextSession() const noexcept { return m_editSessionActive; }

  std::optional<std::string> hitTextIdAt(core::Point point) const;
  std::optional<std::string> textForId(const std::string& id) const;
  std::optional<core::Rect> boundsForId(const std::string& id) const;
  bool setTextForId(const std::string& id, const std::string& text);
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

  std::vector<TextObject> m_objects;
  std::optional<std::string> m_selectedId;
  std::optional<core::Rect> m_selectedBounds;

  bool m_editSessionActive {false};
  bool m_editCreatedNow {false};
  std::string m_editId;
  std::string m_editOriginalText;
  int m_editCaretIndex {0};

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
