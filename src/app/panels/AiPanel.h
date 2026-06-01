#pragma once

#include <QWidget>

class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QTabWidget;
class QTextEdit;
class QVBoxLayout;

namespace app::bridge {
class AppController;
}

namespace app::panels {

// ─────────────────────────────────────────────────────────────────────────────
// AiPanel
//
// Krita AI Diffusion 相当の ComfyUI 連携パネル。
//   ● Generate タブ: テキスト→画像生成
//   ● Inpaint タブ:  選択範囲インペイント
//   ● Connection:    サーバー URL・接続状態・モデル選択
// ─────────────────────────────────────────────────────────────────────────────
class AiPanel : public QWidget {
  Q_OBJECT

 public:
  explicit AiPanel(QWidget* parent = nullptr);
  void setController(app::bridge::AppController* controller);

 private slots:
  void onConnectClicked();
  void onGenerateClicked();
  void onInpaintClicked();
  void onCancelClicked();
  void onComfyStateChanged(bool connected);
  void onModelsLoaded(const QStringList& models);
  void onProgressUpdate(int step, int total);
  void onGenerationComplete(const QString& opType);
  void onGenerationError(const QString& message);

 private:
  void setupUi();
  void setGenerating(bool generating);
  void updateConnectionStatus(bool connected);
  QString currentCheckpoint() const;

  app::bridge::AppController* m_controller {nullptr};

  // ── Connection UI ──────────────────────────────────────────────────────────
  QLineEdit*   m_urlEdit          {nullptr};
  QPushButton* m_connectButton    {nullptr};
  QLabel*      m_statusLabel      {nullptr};
  QComboBox*   m_modelCombo       {nullptr};
  QPushButton* m_refreshModels    {nullptr};

  // ── Shared params ──────────────────────────────────────────────────────────
  QTextEdit*      m_promptEdit    {nullptr};
  QTextEdit*      m_negEdit       {nullptr};
  QSpinBox*       m_stepsSpinShared  {nullptr};
  QDoubleSpinBox* m_cfgSpinShared    {nullptr};
  QSpinBox*       m_seedSpinShared   {nullptr};

  // ── Generate tab ──────────────────────────────────────────────────────────
  QSpinBox*    m_widthSpin        {nullptr};
  QSpinBox*    m_heightSpin       {nullptr};
  QPushButton* m_generateButton   {nullptr};

  // ── Inpaint tab ───────────────────────────────────────────────────────────
  QDoubleSpinBox* m_denoiseSpin   {nullptr};
  QPushButton*    m_inpaintButton {nullptr};

  // ── Progress / Cancel ─────────────────────────────────────────────────────
  QProgressBar* m_progressBar     {nullptr};
  QPushButton*  m_cancelButton    {nullptr};
  QLabel*       m_resultLabel     {nullptr};

  QTabWidget*   m_tabs            {nullptr};
};

}  // namespace app::panels
