#pragma once

#include <functional>
#include <string>
#include <string_view>

#include "core/selection/providers/ISelectionProvider.h"

namespace core {

// ─────────────────────────────────────────────────────────────────────────────
// AIProvider
//
// AI バックエンド (SAM2 / BiRefNet / MODNet 等) へのブリッジ。
// 実際の推論は Callback に委譲するため、Qt や ComfyUI への依存をコアに持ち込まない。
//
// 使い方:
//   auto provider = std::make_unique<AIProvider>();
//   provider->setInferenceCallback([](const SelectionRequest& req,
//                                     const PixelBuffer& ref,
//                                     int w, int h) -> SelectionResult {
//     // ここで SAM2 / HTTP / WebSocket 等を呼ぶ
//     return result;
//   });
//   engine.setProvider(std::move(provider));
//
// Callback が未設定の場合は空マスクを返す (graceful degradation)。
// ─────────────────────────────────────────────────────────────────────────────
class AIProvider final : public ISelectionProvider {
public:
  using InferenceCallback =
      std::function<SelectionResult(const SelectionRequest&,
                                    const PixelBuffer&,
                                    int w, int h)>;

  AIProvider() = default;
  ~AIProvider() override = default;

  void setInferenceCallback(InferenceCallback cb) { m_callback = std::move(cb); }
  bool hasCallback() const noexcept { return static_cast<bool>(m_callback); }

  bool canHandle(SelectionRequest::Type type) const noexcept override {
    return type == SelectionRequest::Type::Object ||
           type == SelectionRequest::Type::Subject ||
           type == SelectionRequest::Type::Semantic ||
           type == SelectionRequest::Type::AI;
  }

  SelectionResult execute(const SelectionRequest& request,
                           const PixelBuffer&      reference,
                           int canvasW,
                           int canvasH) const override {
    if (m_callback) {
      return m_callback(request, reference, canvasW, canvasH);
    }
    // Graceful fallback: empty mask
    SelectionResult result;
    result.mask        = SelectionMask(canvasW, canvasH);
    result.fromAI      = true;
    result.providerName = "AI(stub)";
    return result;
  }

  std::string_view name() const noexcept override { return m_name; }
  void setName(std::string name) { m_name = std::move(name); }

private:
  InferenceCallback m_callback;
  std::string       m_name {"AI"};
};

} // namespace core
