#pragma once

#include <functional>
#include <string>

#include "core/color/Color.h"
#include "core/common/Point.h"
#include "core/tools/ITool.h"

namespace core {

// ─────────────────────────────────────────────────────────────────────────────
// TextTool — テキストツール (PS「横書き文字ツール」T 相当)
//
// 操作:
//   クリック      → テキスト入力開始（キャレット表示）
//   Enter         → 改行
//   Escape        → 確定（ラスタライズしてレイヤーに書き込み）
//   Ctrl+Enter    → 確定（Escape と同じ）
//   外部クリック  → 確定
//
// レンダリング:
//   ラスタライズは Qt の QPainter でアプリ側が担当する。
//   Tool はテキスト状態（文字列・位置・フォント設定）を保持し、
//   CommitCallback でアプリ側に渡す。
// ─────────────────────────────────────────────────────────────────────────────
class TextTool : public ITool {
public:
  struct TextSettings {
    std::string fontFamily {"Arial"};
    int         fontSize   {24};     // pt
    bool        bold       {false};
    bool        italic     {false};
    Color       color      {0, 0, 0, 255};
    bool        antiAlias  {true};
  };

  // 確定時コールバック: テキスト文字列・位置・設定を渡す
  using CommitCallback = std::function<void(const std::string& text,
                                            Point position,
                                            const TextSettings& settings)>;

  ToolKind         kind()        const noexcept override { return ToolKind::Text; }
  std::string_view displayName() const noexcept override { return "Text"; }

  ToolResult onPointerPress  (ToolContext& ctx, const ToolPointerEvent& e) override;
  ToolResult onPointerMove   (ToolContext& ctx, const ToolPointerEvent& e) override;
  ToolResult onPointerRelease(ToolContext& ctx, const ToolPointerEvent& e) override;
  ToolResult onCancel        (ToolContext& ctx) override;
  ToolResult onWheel         (ToolContext& ctx, int delta, const ToolPointerEvent& e) override;
  ToolOverlayState overlay() const override;

  // テキスト入力（AppController / Qt キーイベントから呼ぶ）
  void inputText(const std::string& text);
  void inputBackspace();
  void inputNewline();
  void commitText(ToolContext& ctx);  ///< Escape / Ctrl+Enter で確定

  bool isEditing() const noexcept { return m_editing; }
  const std::string& currentText() const noexcept { return m_text; }
  Point textOrigin() const noexcept { return m_origin; }

  // 設定
  TextSettings& settings() noexcept { return m_settings; }
  const TextSettings& settings() const noexcept { return m_settings; }

  void setCommitCallback(CommitCallback cb) { m_commitCb = std::move(cb); }

private:
  bool         m_editing {false};
  std::string  m_text;
  Point        m_origin  {0, 0};
  TextSettings m_settings;
  CommitCallback m_commitCb;

  // カーソル点滅用カウンタ（overlay で使用）
  mutable int m_blinkCounter {0};
};

} // namespace core
