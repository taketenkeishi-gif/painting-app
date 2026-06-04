#pragma once

#include <QDialog>

#include "core/buffer/PixelBuffer.h"
#include "core/selection/SelectionMask.h"
#include "core/ai/GenerativeFillEngine.h"

class QComboBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QSlider;
class QSpinBox;
class QTextEdit;

namespace app::panels {

// ─────────────────────────────────────────────────────────────────────────────
// GenerativeFillDialog
//
// AI 生成塗りつぶしの設定・実行ダイアログ。
// ・プロンプト / ネガプロンプト入力
// ・ステップ数・CFG スケール・インペイント強度・シード設定
// ・バックエンド選択（現在はスタブのみ）
// ・生成プレビュー（ビフォー / アフター）
// ・進捗バー付き非同期生成
// ─────────────────────────────────────────────────────────────────────────────
class GenerativeFillDialog : public QDialog {
  Q_OBJECT

public:
  explicit GenerativeFillDialog(
      const core::PixelBuffer&    canvas,
      const core::SelectionMask&  mask,
      QWidget*                    parent = nullptr);

  /// 承認された生成結果バッファを返す（Accept した場合のみ有効）
  const core::PixelBuffer& result() const noexcept { return m_result; }

  /// 生成結果が有効かどうか
  bool hasResult() const noexcept { return m_hasResult; }

private slots:
  void onGenerateClicked();
  void onCancelGeneration();
  void onProgressChanged(int percent);
  void onResultReady(core::PixelBuffer result);
  void onErrorOccurred(const QString& message);

private:
  void setupUi();
  void updatePreviewThumbnails();
  void setGenerating(bool generating);
  static QPixmap bufferToPixmap(const core::PixelBuffer& buf, const QSize& size);

  // ── 入力 ────────────────────────────────────────────────
  const core::PixelBuffer&   m_canvas;
  const core::SelectionMask& m_mask;

  // ── ウィジェット ─────────────────────────────────────────
  QLabel*            m_beforeThumb      {nullptr};
  QLabel*            m_afterThumb       {nullptr};
  QTextEdit*         m_promptEdit       {nullptr};
  QTextEdit*         m_negPromptEdit    {nullptr};
  QSpinBox*          m_stepsSpin        {nullptr};
  QDoubleSpinBox*    m_guidanceSpin     {nullptr};
  QSlider*           m_strengthSlider   {nullptr};
  QLabel*            m_strengthLabel    {nullptr};
  QSpinBox*          m_seedSpin         {nullptr};
  QComboBox*         m_backendCombo     {nullptr};
  QPushButton*       m_generateBtn      {nullptr};
  QPushButton*       m_cancelGenBtn     {nullptr};
  QProgressBar*      m_progressBar      {nullptr};
  QLabel*            m_statusLabel      {nullptr};
  QDialogButtonBox*  m_dialogButtons    {nullptr};

  // ── エンジン ─────────────────────────────────────────────
  core::ai::GenerativeFillEngine* m_engine {nullptr};

  // ── 結果 ─────────────────────────────────────────────────
  core::PixelBuffer  m_result;
  bool               m_hasResult {false};
};

} // namespace app::panels
