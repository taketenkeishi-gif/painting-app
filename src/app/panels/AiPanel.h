#pragma once

#include <QElapsedTimer>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QPixmap>
#include <QWidget>

#include "app/panels/WorkflowBindingDialog.h"

class QButtonGroup;
class QComboBox;
class QDoubleSpinBox;
class QFrame;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QScrollArea;
class QSpinBox;
class QTabWidget;
class QTextEdit;
class QToolButton;
class QVBoxLayout;

namespace app::bridge {
class AppController;
}

namespace app::panels {

// ─────────────────────────────────────────────────────────────────────────────
// AiPanel  — ComfyUI 連携パネル
//
//  ● 接続: URL / モデル
//  ● ワークフロー: カスタム JSON ファイル or 内蔵
//  ● パラメーター: prompt / negative / steps / CFG / seed
//  ● タブ:
//      テキスト生成: 幅・高さ・生成枚数 [1][2][4]
//      インペイント: denoise 強度・生成枚数 [1][2][4]
//  ● ライブプレビュー: KSampler 中間フレーム表示
//  ● 進捗: ノード名・step/total・経過時間・ETA
//  ● 生成結果: サムネイルグリッド + [Apply] でレイヤー適用
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
  void onBrowseWorkflow();
  void onBindingClicked();
  void onWorkflowSelected(int index);
  void onApplyCandidate();
  void onComfyStateChanged(bool connected);
  void onModelsLoaded(const QStringList& models);
  void onProgressUpdate(int step, int total, const QString& nodeId);
  void onPreviewReceived(const QPixmap& px);
  void onGenerationComplete(const QString& opType);
  void onBatchCandidatesReady(const QList<QPixmap>& candidates);
  void onGenerationError(const QString& message);
  void onSelectionMissing();
  void onElapsedTick();

private:
  void setupUi();
  void setGenerating(bool generating);
  void updateConnectionStatus(bool connected);
  void populateWorkflowCombo();
  void addRecentWorkflow(const QString& path);
  QString currentCheckpoint() const;
  int batchCount() const;
  QJsonObject currentWorkflow() const;   ///< loaded or {} = use built-in
  void showCandidates(const QList<QPixmap>& pixmaps);
  void clearCandidates();

  app::bridge::AppController* m_controller {nullptr};

  // ── Connection UI ──────────────────────────────────────────────────────────
  QLineEdit*   m_urlEdit          {nullptr};
  QPushButton* m_connectButton    {nullptr};
  QLabel*      m_statusLabel      {nullptr};
  QComboBox*   m_modelCombo       {nullptr};
  QPushButton* m_refreshModels    {nullptr};

  // ── Workflow ──────────────────────────────────────────────────────────────
  QComboBox*   m_workflowCombo    {nullptr};
  QPushButton* m_workflowBrowse   {nullptr};
  QPushButton* m_bindingButton    {nullptr};
  QJsonObject  m_loadedWorkflow;              ///< {} = use built-in
  QStringList  m_recentWorkflows;
  // ノードバインド設定: workflowPath → config
  QMap<QString, WorkflowBindingConfig> m_bindingConfigs;

  // ── Helpers ──────────────────────────────────────────────────────────────
  void saveBindingConfig(const QString& path, const WorkflowBindingConfig& cfg);
  WorkflowBindingConfig loadBindingConfig(const QString& path) const;
  WorkflowBindingConfig currentBindingConfig() const;
  QString currentWorkflowPath() const;

  // ── Shared params ─────────────────────────────────────────────────────────
  QTextEdit*      m_promptEdit       {nullptr};
  QTextEdit*      m_negEdit          {nullptr};
  QSpinBox*       m_stepsSpinShared  {nullptr};
  QDoubleSpinBox* m_cfgSpinShared    {nullptr};
  QSpinBox*       m_seedSpinShared   {nullptr};

  // ── Generate tab ─────────────────────────────────────────────────────────
  QSpinBox*    m_widthSpin        {nullptr};
  QSpinBox*    m_heightSpin       {nullptr};
  QButtonGroup* m_genBatchGroup   {nullptr};
  QPushButton* m_generateButton   {nullptr};

  // ── Inpaint tab ──────────────────────────────────────────────────────────
  QDoubleSpinBox* m_denoiseSpin   {nullptr};
  QButtonGroup*   m_inpBatchGroup {nullptr};
  QPushButton*    m_inpaintButton {nullptr};

  QTabWidget*  m_tabs             {nullptr};

  // ── Live preview ─────────────────────────────────────────────────────────
  QLabel*       m_previewLabel    {nullptr};

  // ── Progress ─────────────────────────────────────────────────────────────
  QLabel*       m_nodeLabel       {nullptr};
  QLabel*       m_stepLabel       {nullptr};
  QLabel*       m_elapsedLabel    {nullptr};
  QLabel*       m_etaLabel        {nullptr};
  QProgressBar* m_progressBar     {nullptr};
  QPushButton*  m_cancelButton    {nullptr};
  QWidget*      m_progressWidget  {nullptr};
  QElapsedTimer m_genTimer;
  QTimer*       m_elapsedTimer    {nullptr};
  int           m_lastStep        {0};
  int           m_lastTotal       {0};

  // ── Candidate grid ────────────────────────────────────────────────────────
  QScrollArea*  m_candidateScroll {nullptr};
  QWidget*      m_candidateHost   {nullptr};
  QHBoxLayout*  m_candidateLayout {nullptr};
  QList<QToolButton*> m_candidateBtns;
  QList<QPixmap>      m_candidatePixmaps;
  int           m_selectedCandidate {-1};
  QPushButton*  m_applyButton     {nullptr};
  QWidget*      m_candidateWidget {nullptr};

  // ── Result label ─────────────────────────────────────────────────────────
  QLabel*       m_resultLabel     {nullptr};
};

}  // namespace app::panels
