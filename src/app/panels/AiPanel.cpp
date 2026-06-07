#include "app/panels/AiPanel.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "app/bridge/AppController.h"

namespace app::panels {

namespace {

constexpr int kPreviewSize   = 160;
constexpr int kThumbSize     = 96;
constexpr int kMaxRecent     = 8;
const char kSettingsKey[]    = "ai/recentWorkflows";

QPushButton* makeBatchBtn(const QString& label, QWidget* parent) {
  auto* btn = new QPushButton(label, parent);
  btn->setCheckable(true);
  btn->setFixedWidth(40);
  btn->setStyleSheet(
      "QPushButton { background:#252838; color:#9095ab; border:1px solid #353a50;"
      " border-radius:3px; font-size:11px; }"
      "QPushButton:checked { background:#2a3a5e; color:#c5d0e8;"
      " border-color:#4e8ef7; font-weight:600; }"
      "QPushButton:hover { background:#2d3248; }");
  return btn;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// ctor
// ─────────────────────────────────────────────────────────────────────────────
AiPanel::AiPanel(QWidget* parent) : QWidget(parent) {
  setupUi();
  m_elapsedTimer = new QTimer(this);
  m_elapsedTimer->setInterval(500);
  connect(m_elapsedTimer, &QTimer::timeout, this, &AiPanel::onElapsedTick);
}

// ─────────────────────────────────────────────────────────────────────────────
// UI 構築
// ─────────────────────────────────────────────────────────────────────────────
void AiPanel::setupUi() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(6, 6, 6, 6);
  root->setSpacing(6);

  // ── 接続バー ─────────────────────────────────────────────────────────────
  {
    auto* grp = new QGroupBox(QStringLiteral("ComfyUI 接続"), this);
    auto* lay = new QVBoxLayout(grp);
    lay->setContentsMargins(6, 16, 6, 6);
    lay->setSpacing(4);

    auto* urlRow = new QHBoxLayout();
    m_urlEdit = new QLineEdit(QStringLiteral("http://localhost:8188"), grp);
    m_connectButton = new QPushButton(QStringLiteral("接続"), grp);
    m_connectButton->setFixedWidth(56);
    urlRow->addWidget(m_urlEdit, 1);
    urlRow->addWidget(m_connectButton);

    m_statusLabel = new QLabel(QStringLiteral("● 未接続"), grp);
    m_statusLabel->setStyleSheet("color: #e05555;");

    auto* modelRow = new QHBoxLayout();
    m_modelCombo = new QComboBox(grp);
    m_modelCombo->addItem(QStringLiteral("v1-5-pruned-emaonly.ckpt"));
    m_modelCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_refreshModels = new QPushButton(QStringLiteral("↻"), grp);
    m_refreshModels->setFixedWidth(28);
    m_refreshModels->setToolTip(QStringLiteral("モデル一覧を再取得"));
    modelRow->addWidget(m_modelCombo, 1);
    modelRow->addWidget(m_refreshModels);

    lay->addLayout(urlRow);
    lay->addWidget(m_statusLabel);
    lay->addLayout(modelRow);
    root->addWidget(grp);
  }

  // ── ワークフロー ──────────────────────────────────────────────────────────
  {
    auto* grp = new QGroupBox(QStringLiteral("ワークフロー"), this);
    auto* lay = new QHBoxLayout(grp);
    lay->setContentsMargins(6, 16, 6, 6);
    lay->setSpacing(4);

    m_workflowCombo = new QComboBox(grp);
    m_workflowCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_workflowBrowse = new QPushButton(QStringLiteral("参照"), grp);
    m_workflowBrowse->setFixedWidth(48);

    lay->addWidget(m_workflowCombo, 1);
    lay->addWidget(m_workflowBrowse);
    root->addWidget(grp);

    // 最近使ったワークフローを読み込む
    QSettings settings;
    m_recentWorkflows = settings.value(kSettingsKey).toStringList();
    populateWorkflowCombo();
  }

  // ── 共通パラメーター ──────────────────────────────────────────────────────
  {
    auto* grp = new QGroupBox(QStringLiteral("パラメーター"), this);
    auto* lay = new QFormLayout(grp);
    lay->setContentsMargins(6, 16, 6, 6);
    lay->setSpacing(4);
    lay->setLabelAlignment(Qt::AlignRight);

    m_promptEdit = new QTextEdit(grp);
    m_promptEdit->setPlaceholderText(QStringLiteral("プロンプト (例: a cat, high quality)"));
    m_promptEdit->setMaximumHeight(56);
    m_negEdit = new QTextEdit(grp);
    m_negEdit->setPlaceholderText(QStringLiteral("ネガティブ (例: blurry, low quality)"));
    m_negEdit->setMaximumHeight(40);

    m_stepsSpinShared = new QSpinBox(grp);
    m_stepsSpinShared->setRange(1, 150);
    m_stepsSpinShared->setValue(20);

    m_cfgSpinShared = new QDoubleSpinBox(grp);
    m_cfgSpinShared->setRange(1.0, 30.0);
    m_cfgSpinShared->setSingleStep(0.5);
    m_cfgSpinShared->setValue(7.5);

    m_seedSpinShared = new QSpinBox(grp);
    m_seedSpinShared->setRange(-1, 2147483647);
    m_seedSpinShared->setValue(-1);
    m_seedSpinShared->setSpecialValueText(QStringLiteral("ランダム"));

    lay->addRow(QStringLiteral("プロンプト:"), m_promptEdit);
    lay->addRow(QStringLiteral("ネガティブ:"), m_negEdit);
    lay->addRow(QStringLiteral("ステップ:"), m_stepsSpinShared);
    lay->addRow(QStringLiteral("CFG:"), m_cfgSpinShared);
    lay->addRow(QStringLiteral("シード:"), m_seedSpinShared);
    root->addWidget(grp);
  }

  // ── タブ: テキスト生成 / インペイント ─────────────────────────────────────
  m_tabs = new QTabWidget(this);

  // テキスト生成タブ
  {
    auto* tab = new QWidget();
    auto* lay = new QFormLayout(tab);
    lay->setContentsMargins(6, 8, 6, 8);
    lay->setSpacing(4);

    m_widthSpin = new QSpinBox(tab);
    m_widthSpin->setRange(64, 2048);
    m_widthSpin->setSingleStep(64);
    m_widthSpin->setValue(512);
    m_heightSpin = new QSpinBox(tab);
    m_heightSpin->setRange(64, 2048);
    m_heightSpin->setSingleStep(64);
    m_heightSpin->setValue(512);

    // 生成枚数
    auto* batchRow = new QHBoxLayout();
    batchRow->setSpacing(4);
    auto* b1 = makeBatchBtn("1", tab);
    auto* b2 = makeBatchBtn("2", tab);
    auto* b4 = makeBatchBtn("4", tab);
    b1->setChecked(true);
    m_genBatchGroup = new QButtonGroup(this);
    m_genBatchGroup->addButton(b1, 1);
    m_genBatchGroup->addButton(b2, 2);
    m_genBatchGroup->addButton(b4, 4);
    batchRow->addWidget(b1);
    batchRow->addWidget(b2);
    batchRow->addWidget(b4);
    batchRow->addStretch();

    m_generateButton = new QPushButton(QStringLiteral("✦ 生成"), tab);
    m_generateButton->setMinimumHeight(30);
    m_generateButton->setStyleSheet(
        "QPushButton { background: #1d4e8a; color: #edf0f9; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2560a8; }"
        "QPushButton:disabled { background: #252a38; color: #555; }");

    lay->addRow(QStringLiteral("幅:"), m_widthSpin);
    lay->addRow(QStringLiteral("高さ:"), m_heightSpin);
    lay->addRow(QStringLiteral("枚数:"), batchRow);
    lay->addRow(m_generateButton);
    m_tabs->addTab(tab, QStringLiteral("テキスト生成"));
  }

  // インペイントタブ
  {
    auto* tab = new QWidget();
    auto* lay = new QFormLayout(tab);
    lay->setContentsMargins(6, 8, 6, 8);
    lay->setSpacing(4);

    m_denoiseSpin = new QDoubleSpinBox(tab);
    m_denoiseSpin->setRange(0.01, 1.0);
    m_denoiseSpin->setSingleStep(0.05);
    m_denoiseSpin->setValue(0.75);

    // 生成枚数
    auto* batchRow = new QHBoxLayout();
    batchRow->setSpacing(4);
    auto* b1 = makeBatchBtn("1", tab);
    auto* b2 = makeBatchBtn("2", tab);
    auto* b4 = makeBatchBtn("4", tab);
    b1->setChecked(true);
    m_inpBatchGroup = new QButtonGroup(this);
    m_inpBatchGroup->addButton(b1, 1);
    m_inpBatchGroup->addButton(b2, 2);
    m_inpBatchGroup->addButton(b4, 4);
    batchRow->addWidget(b1);
    batchRow->addWidget(b2);
    batchRow->addWidget(b4);
    batchRow->addStretch();

    m_inpaintButton = new QPushButton(QStringLiteral("✦ インペイント"), tab);
    m_inpaintButton->setMinimumHeight(30);
    m_inpaintButton->setStyleSheet(
        "QPushButton { background: #2d6e48; color: #edf0f9; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #357a55; }"
        "QPushButton:disabled { background: #252a38; color: #555; }");

    lay->addRow(QStringLiteral("デノイズ:"), m_denoiseSpin);
    lay->addRow(QStringLiteral("枚数:"), batchRow);
    lay->addRow(m_inpaintButton);
    m_tabs->addTab(tab, QStringLiteral("インペイント"));
  }

  root->addWidget(m_tabs);

  // ── 生成中 UI: プレビュー + 進捗 ─────────────────────────────────────────
  m_progressWidget = new QWidget(this);
  m_progressWidget->setVisible(false);
  {
    auto* pLay = new QVBoxLayout(m_progressWidget);
    pLay->setContentsMargins(0, 0, 0, 0);
    pLay->setSpacing(4);

    // ライブプレビュー
    m_previewLabel = new QLabel(m_progressWidget);
    m_previewLabel->setFixedSize(kPreviewSize, kPreviewSize);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet(
        "QLabel { background: #141720; border: 1px solid #2a2e3e; border-radius:3px; }");
    m_previewLabel->setText(QStringLiteral("生成中…"));

    auto* previewRow = new QHBoxLayout();
    previewRow->addStretch();
    previewRow->addWidget(m_previewLabel);
    previewRow->addStretch();
    pLay->addLayout(previewRow);

    // ノード名 / ステップ / 時間
    auto* infoRow = new QHBoxLayout();
    m_nodeLabel = new QLabel(m_progressWidget);
    m_nodeLabel->setStyleSheet("color:#8090b0; font-size:10px;");
    m_stepLabel = new QLabel(m_progressWidget);
    m_stepLabel->setStyleSheet("color:#a0b0cc; font-size:10px; font-weight:600;");
    m_elapsedLabel = new QLabel(m_progressWidget);
    m_elapsedLabel->setStyleSheet("color:#6080a0; font-size:10px;");
    m_etaLabel = new QLabel(m_progressWidget);
    m_etaLabel->setStyleSheet("color:#6080a0; font-size:10px;");
    infoRow->addWidget(m_nodeLabel);
    infoRow->addStretch();
    infoRow->addWidget(m_stepLabel);
    infoRow->addWidget(m_elapsedLabel);
    infoRow->addWidget(m_etaLabel);
    pLay->addLayout(infoRow);

    m_progressBar = new QProgressBar(m_progressWidget);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFixedHeight(8);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar { background:#1c2030; border-radius:4px; }"
        "QProgressBar::chunk { background:#4e8ef7; border-radius:4px; }");
    pLay->addWidget(m_progressBar);

    m_cancelButton = new QPushButton(QStringLiteral("キャンセル"), m_progressWidget);
    m_cancelButton->setFixedHeight(24);
    pLay->addWidget(m_cancelButton);
  }
  root->addWidget(m_progressWidget);

  // ── 候補グリッド ──────────────────────────────────────────────────────────
  m_candidateWidget = new QWidget(this);
  m_candidateWidget->setVisible(false);
  {
    auto* cLay = new QVBoxLayout(m_candidateWidget);
    cLay->setContentsMargins(0, 4, 0, 0);
    cLay->setSpacing(4);

    auto* header = new QLabel(QStringLiteral("生成結果"), m_candidateWidget);
    header->setStyleSheet("color:#8090b0; font-size:10px; font-weight:600;");
    cLay->addWidget(header);

    m_candidateScroll = new QScrollArea(m_candidateWidget);
    m_candidateScroll->setFrameShape(QFrame::NoFrame);
    m_candidateScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_candidateScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_candidateScroll->setFixedHeight(kThumbSize + 8);
    m_candidateScroll->setWidgetResizable(false);

    m_candidateHost = new QWidget();
    m_candidateLayout = new QHBoxLayout(m_candidateHost);
    m_candidateLayout->setContentsMargins(2, 2, 2, 2);
    m_candidateLayout->setSpacing(4);
    m_candidateLayout->addStretch();
    m_candidateScroll->setWidget(m_candidateHost);
    cLay->addWidget(m_candidateScroll);

    m_applyButton = new QPushButton(QStringLiteral("Apply"), m_candidateWidget);
    m_applyButton->setEnabled(false);
    m_applyButton->setStyleSheet(
        "QPushButton { background:#1d4e8a; color:#edf0f9; border-radius:4px;"
        " font-weight:bold; min-height:26px; }"
        "QPushButton:hover { background:#2560a8; }"
        "QPushButton:disabled { background:#252a38; color:#555; }");
    cLay->addWidget(m_applyButton);
  }
  root->addWidget(m_candidateWidget);

  // ── 結果ラベル ────────────────────────────────────────────────────────────
  m_resultLabel = new QLabel(this);
  m_resultLabel->setWordWrap(true);
  m_resultLabel->setStyleSheet("color: #8af; font-size:10px;");
  root->addWidget(m_resultLabel);
  root->addStretch();

  setLayout(root);

  // ── シグナル接続 ──────────────────────────────────────────────────────────
  connect(m_connectButton,  &QPushButton::clicked, this, &AiPanel::onConnectClicked);
  connect(m_generateButton, &QPushButton::clicked, this, &AiPanel::onGenerateClicked);
  connect(m_inpaintButton,  &QPushButton::clicked, this, &AiPanel::onInpaintClicked);
  connect(m_cancelButton,   &QPushButton::clicked, this, &AiPanel::onCancelClicked);
  connect(m_workflowBrowse, &QPushButton::clicked, this, &AiPanel::onBrowseWorkflow);
  connect(m_workflowCombo,  QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &AiPanel::onWorkflowSelected);
  connect(m_applyButton,    &QPushButton::clicked, this, &AiPanel::onApplyCandidate);
  connect(m_refreshModels,  &QPushButton::clicked, this, [this]() {
    if (m_controller != nullptr) m_controller->fetchAiModels();
  });

  updateConnectionStatus(false);
}

// ─────────────────────────────────────────────────────────────────────────────
// setController
// ─────────────────────────────────────────────────────────────────────────────
void AiPanel::setController(app::bridge::AppController* controller) {
  if (m_controller != nullptr) {
    disconnect(m_controller, nullptr, this, nullptr);
  }
  m_controller = controller;
  if (m_controller == nullptr) return;

  connect(m_controller, &app::bridge::AppController::comfyUiStateChanged,
          this, &AiPanel::onComfyStateChanged);
  connect(m_controller, &app::bridge::AppController::aiModelsLoaded,
          this, &AiPanel::onModelsLoaded);
  connect(m_controller, &app::bridge::AppController::aiProgressUpdate,
          this, &AiPanel::onProgressUpdate);
  connect(m_controller, &app::bridge::AppController::aiPreviewReceived,
          this, &AiPanel::onPreviewReceived);
  connect(m_controller, &app::bridge::AppController::aiGenerationComplete,
          this, &AiPanel::onGenerationComplete);
  connect(m_controller, &app::bridge::AppController::aiBatchCandidatesReady,
          this, &AiPanel::onBatchCandidatesReady);
  connect(m_controller, &app::bridge::AppController::aiGenerationError,
          this, &AiPanel::onGenerationError);
  connect(m_controller, &app::bridge::AppController::selectionMissing,
          this, &AiPanel::onSelectionMissing);

  onComfyStateChanged(m_controller->isComfyUiConnected());
}

// ─────────────────────────────────────────────────────────────────────────────
// スロット
// ─────────────────────────────────────────────────────────────────────────────
void AiPanel::onConnectClicked() {
  if (m_controller == nullptr) return;
  const QString url = m_urlEdit->text().trimmed();
  m_controller->connectComfyUi(url.isEmpty() ? "http://localhost:8188" : url);
  m_statusLabel->setText(QStringLiteral("● 接続中..."));
  m_statusLabel->setStyleSheet("color: #e0c84a;");
  m_connectButton->setEnabled(false);
}

void AiPanel::onGenerateClicked() {
  if (m_controller == nullptr) {
    m_resultLabel->setText(QStringLiteral("❌ エラー: コントローラが未初期化"));
    return;
  }
  if (!m_controller->isComfyUiConnected()) {
    m_resultLabel->setText(QStringLiteral("❌ エラー: ComfyUI に接続されていません"));
    return;
  }
  m_resultLabel->setText(QString());
  clearCandidates();
  setGenerating(true);

  const int bc = batchCount();
  const QJsonObject wf = currentWorkflow();

  if (wf.isEmpty()) {
    app::bridge::AppController::Txt2ImgParams p;
    p.prompt         = m_promptEdit->toPlainText().trimmed();
    p.negativePrompt = m_negEdit->toPlainText().trimmed();
    p.checkpoint     = currentCheckpoint();
    p.steps          = m_stepsSpinShared->value();
    p.cfg            = static_cast<float>(m_cfgSpinShared->value());
    p.seed           = m_seedSpinShared->value();
    p.width          = m_widthSpin->value();
    p.height         = m_heightSpin->value();
    m_resultLabel->setText(QStringLiteral("⏳ txt2img を送信中..."));
    m_controller->runTextToImage(p, bc);
  } else {
    m_resultLabel->setText(QStringLiteral("⏳ カスタムワークフローを送信中..."));
    m_controller->runWorkflow(
        wf,
        m_promptEdit->toPlainText().trimmed(),
        m_negEdit->toPlainText().trimmed(),
        m_seedSpinShared->value(),
        currentCheckpoint(),
        bc);
  }
}

void AiPanel::onInpaintClicked() {
  if (m_controller == nullptr || !m_controller->isComfyUiConnected()) return;
  m_resultLabel->setText(QString());
  clearCandidates();

  // 選択範囲チェックは AppController が行い、なければ selectionMissing() を emit する
  // → onSelectionMissing() でダイアログ表示
  setGenerating(true);

  const int bc = batchCount();
  const QJsonObject wf = currentWorkflow();

  if (wf.isEmpty()) {
    app::bridge::AppController::InpaintParams p;
    p.prompt         = m_promptEdit->toPlainText().trimmed();
    p.negativePrompt = m_negEdit->toPlainText().trimmed();
    p.checkpoint     = currentCheckpoint();
    p.steps          = m_stepsSpinShared->value();
    p.cfg            = static_cast<float>(m_cfgSpinShared->value());
    p.seed           = m_seedSpinShared->value();
    p.denoise        = static_cast<float>(m_denoiseSpin->value());
    m_controller->runInpaint(p, bc);
  } else {
    m_controller->runWorkflow(
        wf,
        m_promptEdit->toPlainText().trimmed(),
        m_negEdit->toPlainText().trimmed(),
        m_seedSpinShared->value(),
        currentCheckpoint(),
        bc);
  }
}

void AiPanel::onCancelClicked() {
  if (m_controller != nullptr) m_controller->cancelAiGeneration();
  setGenerating(false);
  m_resultLabel->setStyleSheet("color: #8090a0; font-size:10px;");
  m_resultLabel->setText(QStringLiteral("キャンセルしました"));
}

void AiPanel::onBrowseWorkflow() {
  const QString path = QFileDialog::getOpenFileName(
      this,
      QStringLiteral("ComfyUI ワークフロー (API 形式) を開く"),
      QString(),
      QStringLiteral("JSON ファイル (*.json)"));
  if (path.isEmpty()) return;

  // ファイル読み込み
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(this, QStringLiteral("エラー"),
        QStringLiteral("ファイルを開けませんでした:\n") + path);
    return;
  }
  const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
  if (doc.isNull() || !doc.isObject()) {
    QMessageBox::warning(this, QStringLiteral("エラー"),
        QStringLiteral("有効な JSON ファイルではありません"));
    return;
  }

  m_loadedWorkflow = doc.object();
  addRecentWorkflow(path);
}

void AiPanel::onWorkflowSelected(int index) {
  // index 0 = "(内蔵ワークフロー)"
  if (index <= 0) {
    m_loadedWorkflow = {};
    return;
  }
  const QString path = m_workflowCombo->itemData(index).toString();
  if (path.isEmpty()) { m_loadedWorkflow = {}; return; }

  QFile f(path);
  if (!f.open(QIODevice::ReadOnly)) { m_loadedWorkflow = {}; return; }
  const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
  m_loadedWorkflow = (doc.isNull() || !doc.isObject()) ? QJsonObject{} : doc.object();
}

void AiPanel::onApplyCandidate() {
  if (m_controller == nullptr) return;
  if (m_selectedCandidate < 0
   || m_selectedCandidate >= m_candidatePixmaps.size()) return;

  const QPixmap& px = m_candidatePixmaps.at(m_selectedCandidate);
  m_controller->applyBatchCandidate(px);
  clearCandidates();
}

void AiPanel::onComfyStateChanged(bool connected) {
  updateConnectionStatus(connected);
  m_connectButton->setEnabled(true);
  if (connected && m_controller != nullptr) {
    m_controller->fetchAiModels();
  }
}

void AiPanel::onModelsLoaded(const QStringList& models) {
  const QString current = m_modelCombo->currentText();
  m_modelCombo->clear();
  if (models.isEmpty()) {
    m_modelCombo->addItem(QStringLiteral("v1-5-pruned-emaonly.ckpt"));
  } else {
    for (const QString& m : models) m_modelCombo->addItem(m);
    const int idx = m_modelCombo->findText(current);
    if (idx >= 0) m_modelCombo->setCurrentIndex(idx);
  }
}

void AiPanel::onProgressUpdate(int step, int total, const QString& nodeId) {
  m_lastStep  = step;
  m_lastTotal = total;

  // ノード名 (class_type が取れれば好ましいが ID だけでも可)
  m_nodeLabel->setText(nodeId.isEmpty() ? QString() : nodeId);

  if (total > 0) {
    m_progressBar->setRange(0, total);
    m_progressBar->setValue(step);
    m_stepLabel->setText(QString("Step %1 / %2").arg(step).arg(total));

    // ETA 計算
    const qint64 elapsed = m_genTimer.elapsed();
    if (step > 0) {
      const qint64 etaMs = static_cast<qint64>(
          (static_cast<double>(elapsed) / step) * (total - step));
      const int etaSec = static_cast<int>(etaMs / 1000);
      m_etaLabel->setText(QString(" ETA %1:%2")
          .arg(etaSec / 60, 2, 10, QChar('0'))
          .arg(etaSec % 60, 2, 10, QChar('0')));
    }
  } else {
    m_progressBar->setRange(0, 0);
    m_stepLabel->setText(QString());
    m_etaLabel->setText(QString());
  }
}

void AiPanel::onPreviewReceived(const QPixmap& px) {
  if (!px.isNull()) {
    m_previewLabel->setPixmap(
        px.scaled(kPreviewSize, kPreviewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  }
}

void AiPanel::onGenerationComplete(const QString& /*opType*/) {
  setGenerating(false);
  m_resultLabel->setStyleSheet("color: #55e0a0; font-size:10px;");
  m_resultLabel->setText(QStringLiteral("✓ 生成完了 — 新規レイヤーに配置されました"));
}

void AiPanel::onBatchCandidatesReady(const QList<QPixmap>& candidates) {
  setGenerating(false);
  m_resultLabel->setStyleSheet("color: #8090b0; font-size:10px;");
  m_resultLabel->setText(
      QString(QStringLiteral("%1 枚生成されました。選択して Apply。")).arg(candidates.size()));
  showCandidates(candidates);
}

void AiPanel::onGenerationError(const QString& message) {
  setGenerating(false);
  m_resultLabel->setStyleSheet("color: #e05555; font-size:10px;");
  m_resultLabel->setText(QStringLiteral("✗ ") + message);
}

void AiPanel::onSelectionMissing() {
  setGenerating(false);

  const auto ans = QMessageBox::question(
      this,
      QStringLiteral("選択範囲なし"),
      QStringLiteral("選択範囲がありません。\n全体生成しますか？"),
      QMessageBox::Yes | QMessageBox::No,
      QMessageBox::No);

  if (ans == QMessageBox::Yes) {
    // 全体を選択してから再実行 (AiPanel は直接 runInpaint を呼ばない — 選択操作が必要)
    m_resultLabel->setStyleSheet("color: #e0c84a; font-size:10px;");
    m_resultLabel->setText(
        QStringLiteral("先にキャンバス全体を選択 (Ctrl+A) してから実行してください。"));
  }
  // No を選択 or 案内表示のみ — 実行しない
}

void AiPanel::onElapsedTick() {
  const qint64 elapsedMs = m_genTimer.elapsed();
  const int sec = static_cast<int>(elapsedMs / 1000);
  m_elapsedLabel->setText(
      QString(" %1:%2")
          .arg(sec / 60, 2, 10, QChar('0'))
          .arg(sec % 60, 2, 10, QChar('0')));
}

// ─────────────────────────────────────────────────────────────────────────────
// プライベート
// ─────────────────────────────────────────────────────────────────────────────
void AiPanel::setGenerating(bool generating) {
  m_generateButton->setEnabled(!generating);
  m_inpaintButton->setEnabled(!generating);
  m_progressWidget->setVisible(generating);

  if (generating) {
    m_progressBar->setRange(0, 0);
    m_progressBar->setValue(0);
    m_nodeLabel->setText(QString());
    m_stepLabel->setText(QString());
    m_elapsedLabel->setText(QStringLiteral(" 00:00"));
    m_etaLabel->setText(QString());
    m_previewLabel->setPixmap(QPixmap());
    m_previewLabel->setText(QStringLiteral("生成中…"));
    m_lastStep = 0; m_lastTotal = 0;
    m_genTimer.start();
    m_elapsedTimer->start();
  } else {
    m_elapsedTimer->stop();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
  }
}

void AiPanel::updateConnectionStatus(bool connected) {
  if (connected) {
    m_statusLabel->setText(QStringLiteral("● 接続済み"));
    m_statusLabel->setStyleSheet("color: #55e0a0;");
    m_generateButton->setEnabled(true);
    m_inpaintButton->setEnabled(true);
  } else {
    m_statusLabel->setText(QStringLiteral("● 未接続"));
    m_statusLabel->setStyleSheet("color: #e05555;");
    m_generateButton->setEnabled(false);
    m_inpaintButton->setEnabled(false);
  }
}

void AiPanel::populateWorkflowCombo() {
  const QSignalBlocker blocker(m_workflowCombo);
  m_workflowCombo->clear();
  m_workflowCombo->addItem(QStringLiteral("(内蔵ワークフロー)"), QString());
  for (const QString& path : m_recentWorkflows) {
    m_workflowCombo->addItem(QFileInfo(path).fileName(), path);
  }
}

void AiPanel::addRecentWorkflow(const QString& path) {
  m_recentWorkflows.removeAll(path);
  m_recentWorkflows.prepend(path);
  while (m_recentWorkflows.size() > kMaxRecent)
    m_recentWorkflows.removeLast();

  QSettings settings;
  settings.setValue(kSettingsKey, m_recentWorkflows);
  populateWorkflowCombo();

  // 追加したファイルを選択
  const int idx = m_workflowCombo->findData(path);
  if (idx >= 0) m_workflowCombo->setCurrentIndex(idx);
}

QString AiPanel::currentCheckpoint() const {
  return m_modelCombo->currentText().trimmed().isEmpty()
      ? QStringLiteral("v1-5-pruned-emaonly.ckpt")
      : m_modelCombo->currentText().trimmed();
}

int AiPanel::batchCount() const {
  // テキスト生成タブが前面なら genBatchGroup, それ以外は inpBatchGroup
  const QButtonGroup* grp = (m_tabs->currentIndex() == 0)
      ? m_genBatchGroup : m_inpBatchGroup;
  const int id = grp->checkedId();
  return (id == 1 || id == 2 || id == 4) ? id : 1;
}

QJsonObject AiPanel::currentWorkflow() const {
  return m_loadedWorkflow;
}

void AiPanel::showCandidates(const QList<QPixmap>& pixmaps) {
  clearCandidates();
  m_candidatePixmaps = pixmaps;
  m_candidateWidget->setVisible(true);

  for (int i = 0; i < pixmaps.size(); ++i) {
    auto* btn = new QToolButton(m_candidateHost);
    btn->setFixedSize(kThumbSize, kThumbSize);
    btn->setCheckable(true);
    btn->setAutoExclusive(true);
    btn->setIcon(QIcon(pixmaps[i].scaled(
        kThumbSize, kThumbSize, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    btn->setIconSize(QSize(kThumbSize - 4, kThumbSize - 4));
    btn->setStyleSheet(
        "QToolButton { border: 2px solid transparent; border-radius:3px; background:#1c2030; }"
        "QToolButton:checked { border-color: #4e8ef7; }"
        "QToolButton:hover { border-color:#3a4a6a; }");

    const int idx = i;
    connect(btn, &QToolButton::toggled, this, [this, idx](bool checked) {
      if (checked) {
        m_selectedCandidate = idx;
        m_applyButton->setEnabled(true);
      }
    });

    // stretch の前に挿入
    m_candidateLayout->insertWidget(i, btn);
    m_candidateBtns.append(btn);
  }

  // 先頭を選択
  if (!m_candidateBtns.isEmpty()) {
    m_candidateBtns.first()->setChecked(true);
  }
}

void AiPanel::clearCandidates() {
  for (auto* btn : m_candidateBtns) {
    m_candidateLayout->removeWidget(btn);
    btn->deleteLater();
  }
  m_candidateBtns.clear();
  m_candidatePixmaps.clear();
  m_selectedCandidate = -1;
  m_applyButton->setEnabled(false);
  m_candidateWidget->setVisible(false);
}

}  // namespace app::panels
