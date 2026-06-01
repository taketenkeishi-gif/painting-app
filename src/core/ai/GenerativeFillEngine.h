#pragma once

#include <QObject>
#include <QString>

#include "core/buffer/PixelBuffer.h"
#include "core/selection/SelectionMask.h"

namespace core::ai {

// ─────────────────────────────────────────────────────────────────────────────
// GenerativeFillEngine
//
// AI 生成塗りつぶし基盤。
// 現在は Stub バックエンドのみ実装（選択範囲にノイズ混合を生成）。
// LocalOnnx / ApiRemote は将来のプレースホルダ。
//
// 使い方:
//   auto* engine = new GenerativeFillEngine(this);
//   connect(engine, &GenerativeFillEngine::progressChanged, ...);
//   connect(engine, &GenerativeFillEngine::resultReady, ...);
//   engine->generate(canvas, mask, settings);
// ─────────────────────────────────────────────────────────────────────────────
class GenerativeFillEngine : public QObject {
  Q_OBJECT

public:
  enum class Backend {
    Stub,       ///< テスト用スタブ（選択範囲をトーン変換して返す）
    LocalOnnx,  ///< ローカル ONNX モデル（未実装）
    ApiRemote,  ///< リモート REST API（未実装）
  };

  struct Settings {
    QString prompt;
    QString negativePrompt;
    int     steps    {20};
    float   guidance {7.5f};    ///< CFG スケール
    float   strength {0.80f};   ///< インペイント強度 0.0–1.0
    int     seed     {-1};      ///< -1 = ランダム
    Backend backend  {Backend::Stub};
  };

  explicit GenerativeFillEngine(QObject* parent = nullptr);

  /// 非同期生成を開始。完了時に resultReady / errorOccurred を emit。
  void generate(const core::PixelBuffer& canvas,
                const core::SelectionMask& mask,
                const Settings& settings);

  /// 処理をキャンセル（Stub の場合は即時停止）。
  void cancel();

  bool isRunning() const noexcept { return m_running; }

signals:
  void progressChanged(int percent);          ///< 0–100
  void resultReady(core::PixelBuffer result); ///< 生成完了
  void errorOccurred(QString message);        ///< エラー

private:
  void runStubAsync(core::PixelBuffer canvas,
                    core::SelectionMask mask,
                    Settings settings);

  bool m_running         {false};
  bool m_cancelRequested {false};
};

} // namespace core::ai
