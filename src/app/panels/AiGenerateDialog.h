#pragma once

#include <QDialog>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QDoubleSpinBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QProgressBar)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QSlider)
QT_FORWARD_DECLARE_CLASS(QSpinBox)
QT_FORWARD_DECLARE_CLASS(QTextEdit)

namespace app::bridge { class AppController; }

namespace app::panels {

// ─────────────────────────────────────────────────────────────────────────────
// AiGenerateDialog  — AI メニュー各項目から開くコンパクトダイアログ
//
// Mode に応じて必要なコントロールのみ表示する。
// モーダルレスなのでキャンバスを見ながらパラメータ調整できる。
// ─────────────────────────────────────────────────────────────────────────────
class AiGenerateDialog : public QDialog {
  Q_OBJECT

public:
  enum class Mode {
    Img2Img,   ///< 通常 img2img（ControlNet なし）
    Inpaint,   ///< 選択範囲インペイント
    Scribble,  ///< ControlNet Scribble (HED)
    Canny,     ///< ControlNet Canny
    Lineart,   ///< ControlNet Lineart
  };

  explicit AiGenerateDialog(Mode mode,
                             app::bridge::AppController* controller,
                             QWidget* parent = nullptr);

  // AppController からの進捗通知を受け取る
  void onProgress(int step, int total, const QString& nodeId);
  void onComplete();
  void onError(const QString& msg);

private:
  void setupUi();
  void setupConnections();
  void onGenerate();

  QString modeName() const;
  QString workflowResource() const;
  bool    needsCnControls()  const;
  bool    needsMask()        const;

  Mode                       m_mode;
  app::bridge::AppController* m_controller {nullptr};
  bool                       m_generating  {false};

  // UI widgets
  QComboBox*      m_checkpointCombo   {nullptr};
  QComboBox*      m_sourceCombo       {nullptr};  ///< アクティブ / 合成
  QTextEdit*      m_promptEdit        {nullptr};
  QTextEdit*      m_negativeEdit      {nullptr};
  QSlider*        m_denoiseSlider     {nullptr};
  QLabel*         m_denoiseLabel      {nullptr};
  QSlider*        m_cnStrengthSlider  {nullptr};
  QLabel*         m_cnStrengthLabel   {nullptr};
  QLineEdit*      m_cnModelEdit       {nullptr};
  QLabel*         m_cnModelLabel      {nullptr};
  QSpinBox*       m_stepsSpinBox      {nullptr};
  QDoubleSpinBox* m_cfgSpinBox        {nullptr};
  QSpinBox*       m_seedSpinBox       {nullptr};
  QComboBox*      m_batchCombo        {nullptr};
  QProgressBar*   m_progressBar       {nullptr};
  QLabel*         m_statusLabel       {nullptr};
  QPushButton*    m_generateBtn       {nullptr};
  QPushButton*    m_cancelBtn         {nullptr};
};

} // namespace app::panels
