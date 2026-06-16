#include "core/tools/TextTool.h"

#include "core/document/Document.h"
#include "core/tools/ToolContext.h"

namespace core {

ToolResult TextTool::onPointerPress(ToolContext& ctx, const ToolPointerEvent& e) {
  if (m_editing) {
    // 別の場所をクリック → 確定して新しい位置で開始
    commitText(ctx);
  }
  m_text.clear();
  m_origin  = e.point;
  m_editing = true;
  m_blinkCounter = 0;
  ToolResult r; r.viewportChanged = true; return r;
}

ToolResult TextTool::onPointerMove(ToolContext& ctx, const ToolPointerEvent& e) {
  static_cast<void>(ctx); static_cast<void>(e); return {};
}

ToolResult TextTool::onPointerRelease(ToolContext& ctx, const ToolPointerEvent& e) {
  static_cast<void>(ctx); static_cast<void>(e); return {};
}

ToolResult TextTool::onCancel(ToolContext& ctx) {
  if (m_editing) commitText(ctx);
  return {};
}

ToolResult TextTool::onWheel(ToolContext& ctx, int delta, const ToolPointerEvent& e) {
  static_cast<void>(ctx); static_cast<void>(delta); static_cast<void>(e);
  return {};
}

void TextTool::inputText(const std::string& text) {
  if (!m_editing) return;
  m_text += text;
  ++m_blinkCounter;
}

void TextTool::inputBackspace() {
  if (!m_editing || m_text.empty()) return;
  // UTF-8 の末尾 1 文字を削除（マルチバイト対応）
  while (!m_text.empty() && (m_text.back() & 0xC0) == 0x80)
    m_text.pop_back();
  if (!m_text.empty()) m_text.pop_back();
  ++m_blinkCounter;
}

void TextTool::inputNewline() {
  if (!m_editing) return;
  m_text += '\n';
  ++m_blinkCounter;
}

void TextTool::commitText(ToolContext& ctx) {
  static_cast<void>(ctx);
  if (!m_editing) return;
  m_editing = false;
  if (!m_text.empty() && m_commitCb) {
    m_commitCb(m_text, m_origin, m_settings);
  }
  m_text.clear();
}

ToolOverlayState TextTool::overlay() const {
  ToolOverlayState state;
  if (!m_editing) return state;

  state.hasTextEdit    = true;
  state.textEditOrigin = m_origin;
  state.textEditContent = m_text;
  return state;
}

} // namespace core
