#include "app/panels/AiPanel.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>

#include "app/bridge/AppController.h"

namespace app::panels {

AiPanel::AiPanel(QWidget* parent) : QWidget(parent) {
  setupUi();
}

void AiPanel::setupUi() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(6, 6, 6, 6);
  root->setSpacing(6);

  // ── 接続バー ───────────────────────────────────────────────────────────────
  auto* connGroup = new QGroupBox(QStringLiteral("ComfyUI 接続"), this);
  auto* connLayout = new QVBoxLayout(connGroup);
  connLayout->setContentsMargins(6, 16, 6, 6);
  connLayout->setSpacing(4);

  auto* urlRow = new QHBoxLayout();
  m_urlEdit = new QLineEdit(QStringLiteral("http://localhost:8188"), connGroup);
  m_connectButton = new QPushButton(QStringLiteral("接続"), connGroup);
  m_connectButton->setFixedWidth(56);
  urlRow->addWidget(m_urlEdit, 1);
  urlRow->addWidget(m_connectButton);

  m_statusLabel = new QLabel(QStringLiteral("● 未接続"), connGroup);
  m_statusLabel->setStyleSheet("color: #e05555;");

  auto* modelRow = new QHBoxLayout();
  m_modelCombo = new QComboBox(connGroup);
  m_modelCombo->addItem(QStringLiteral("v1-5-pruned-emaonly.ckpt"));
  m_modelCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_refreshModels = new QPushButton(QStringLiteral("↻"), connGroup);
  m_refreshModels->setFixedWidth(28);
  m_refreshModels->setToolTip(QStringLiteral("モデル一覧を再取得"));
  modelRow->addWidget(m_modelCombo, 1);
  modelRow->addWidget(m_refreshModels);

  connLayout->addLayout(urlRow);
  connLayout->addWidget(m_statusLabel);
  connLayout->addLayout(modelRow);
  root->addWidget(connGroup);

  // ── 共通パラメーター ───────────────────────────────────────────────────────
  auto* paramGroup = new QGroupBox(QStringLiteral("パラメーター"), this);
  auto* paramLayout = new QFormLayout(paramGroup);
  paramLayout->setContentsMargins(6, 16, 6, 6);
  paramLayout->setSpacing(4);
  paramLayout->setLabelAlignment(Qt::AlignRight);

  m_promptEdit = new QTextEdit(paramGroup);
  m_promptEdit->setPlaceholderText(QStringLiteral("プロンプト (例: a cat, high quality)"));
  m_promptEdit->setMaximumHeight(64);
  m_negEdit = new QTextEdit(paramGroup);
  m_negEdit->setPlaceholderText(QStringLiteral("ネガティブプロンプト (例: blurry, low quality)"));
  m_negEdit->setMaximumHeight(48);

  m_stepsSpinShared = new QSpinBox(paramGroup);
  m_stepsSpinShared->setRange(1, 150);
  m_stepsSpinShared->setValue(20);

  m_cfgSpinShared = new QDoubleSpinBox(paramGroup);
  m_cfgSpinShared->setRange(1.0, 30.0);
  m_cfgSpinShared->setSingleStep(0.5);
  m_cfgSpinShared->setValue(7.5);

  m_seedSpinShared = new QSpinBox(paramGroup);
  m_seedSpinShared->setRange(-1, 2147483647);
  m_seedSpinShared->setValue(-1);
  m_seedSpinShared->setSpecialValueText(QStringLiteral("ランダム"));

  paramLayout->addRow(QStringLiteral("プロンプト:"), m_promptEdit);
  paramLayout->addRow(QStringLiteral("ネガティブ:"), m_negEdit);
  paramLayout->addRow(QStringLiteral("ステップ数:"), m_stepsSpinShared);
  paramLayout->addRow(QStringLiteral("CFG スケール:"), m_cfgSpinShared);
  paramLayout->addRow(QStringLiteral("シード:"), m_seedSpinShared);
  root->addWidget(paramGroup);

  // ── タブ: Generate / Inpaint ───────────────────────────────────────────────
  m_tabs = new QTabWidget(this);

  // Generate タブ
  auto* genTab = new QWidget();
  auto* genLayout = new QFormLayout(genTab);
  genLayout->setContentsMargins(6, 8, 6, 8);
  genLayout->setSpacing(4);
  m_widthSpin = new QSpinBox(genTab);
  m_widthSpin->setRange(64, 2048);
  m_widthSpin->setSingleStep(64);
  m_widthSpin->setValue(512);
  m_heightSpin = new QSpinBox(genTab);
  m_heightSpin->setRange(64, 2048);
  m_heightSpin->setSingleStep(64);
  m_heightSpin->setValue(512);
  m_generateButton = new QPushButton(QStringLiteral("✦ 生成"), genTab);
  m_generateButton->setMinimumHeight(32);
  m_generateButton->setStyleSheet(
      "QPushButton { background: #1d4e8a; color: #edf0f9; border-radius: 4px; font-weight: bold; }"
      "QPushButton:hover { background: #2560a8; }"
      "QPushButton:disabled { background: #252a38; color: #555; }");
  genLayout->addRow(QStringLiteral("幅:"), m_widthSpin);
  genLayout->addRow(QStringLiteral("高さ:"), m_heightSpin);
  genLayout->addRow(m_generateButton);
  m_tabs->addTab(genTab, QStringLiteral("テキスト生成"));

  // Inpaint タブ
  auto* inpTab = new QWidget();
  auto* inpLayout = new QFormLayout(inpTab);
  inpLayout->setContentsMargins(6, 8, 6, 8);
  inpLayout->setSpacing(4);
  m_denoiseSpin = new QDoubleSpinBox(inpTab);
  m_denoiseSpin->setRange(0.01, 1.0);
  m_denoiseSpin->setSingleStep(0.05);
  m_denoiseSpin->setValue(0.75);
  m_inpaintButton = new QPushButton(QStringLiteral("✦ インペイント"), inpTab);
  m_inpaintButton->setMinimumHeight(32);
  m_inpaintButton->setStyleSheet(
      "QPushButton { background: #2d6e48; color: #edf0f9; border-radius: 4px; font-weight: bold; }"
      "QPushButton:hover { background: #357a55; }"
      "QPushButton:disabled { background: #252a38; color: #555; }");
  inpLayout->addRow(QStringLiteral("デノイズ強度:"), m_denoiseSpin);
  inpLayout->addRow(QStringLiteral("ヒント:"), new QLabel(QStringLiteral("選択範囲=マスク\n（なし=全体）"), inpTab));
  inpLayout->addRow(m_inpaintButton);
  m_tabs->addTab(inpTab, QStringLiteral("インペイント"));

  root->addWidget(m_tabs);

  // ── 進捗 / キャンセル ──────────────────────────────────────────────────────
  m_progressBar = new QProgressBar(this);
  m_progressBar->setRange(0, 100);
  m_progressBar->setValue(0);
  m_progressBar->setVisible(false);

  m_cancelButton = new QPushButton(QStringLiteral("キャンセル"), this);
  m_cancelButton->setVisible(false);

  m_resultLabel = new QLabel(this);
  m_resultLabel->setWordWrap(true);
  m_resultLabel->setStyleSheet("color: #8af;");

  root->addWidget(m_progressBar);
  root->addWidget(m_cancelButton);
  root->addWidget(m_resultLabel);
  root->addStretch();

  setLayout(root);

  // ── シグナル接続 ───────────────────────────────────────────────────────────
  connect(m_connectButton, &QPushButton::clicked, this, &AiPanel::onConnectClicked);
  connect(m_generateButton, &QPushButton::clicked, this, &AiPanel::onGenerateClicked);
  connect(m_inpaintButton, &QPushButton::clicked, this, &AiPanel::onInpaintClicked);
  connect(m_cancelButton, &QPushButton::clicked, this, &AiPanel::onCancelClicked);
  connect(m_refreshModels, &QPushButton::clicked, this, [this]() {
    if (m_controller != nullptr) m_controller->fetchAiModels();
  });

  // 初期状態: 未接続
  updateConnectionStatus(false);
}

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
  connect(m_controller, &app::bridge::AppController::aiGenerationComplete,
          this, &AiPanel::onGenerationComplete);
  connect(m_controller, &app::bridge::AppController::aiGenerationError,
          this, &AiPanel::onGenerationError);

  // 既に接続済みなら状態を反映
  onComfyStateChanged(m_controller->isComfyUiConnected());
}

// ── スロット ────────────────────────────────────────────────────────────────

void AiPanel::onConnectClicked() {
  if (m_controller == nullptr) return;
  const QString url = m_urlEdit->text().trimmed();
  m_controller->connectComfyUi(url.isEmpty() ? "http://localhost:8188" : url);
  m_statusLabel->setText(QStringLiteral("● 接続中..."));
  m_statusLabel->setStyleSheet("color: #e0c84a;");
  m_connectButton->setEnabled(false);
}

void AiPanel::onGenerateClicked() {
  if (m_controller == nullptr || !m_controller->isComfyUiConnected()) return;
  app::bridge::AppController::Txt2ImgParams p;
  p.prompt         = m_promptEdit->toPlainText().trimmed();
  p.negativePrompt = m_negEdit->toPlainText().trimmed();
  p.checkpoint     = currentCheckpoint();
  p.steps          = m_stepsSpinShared->value();
  p.cfg            = static_cast<float>(m_cfgSpinShared->value());
  p.seed           = m_seedSpinShared->value();
  p.width          = m_widthSpin->value();
  p.height         = m_heightSpin->value();
  setGenerating(true);
  m_resultLabel->setText(QString());
  m_controller->runTextToImage(p);
}

void AiPanel::onInpaintClicked() {
  if (m_controller == nullptr || !m_controller->isComfyUiConnected()) return;
  app::bridge::AppController::InpaintParams p;
  p.prompt         = m_promptEdit->toPlainText().trimmed();
  p.negativePrompt = m_negEdit->toPlainText().trimmed();
  p.checkpoint     = currentCheckpoint();
  p.steps          = m_stepsSpinShared->value();
  p.cfg            = static_cast<float>(m_cfgSpinShared->value());
  p.seed           = m_seedSpinShared->value();
  p.denoise        = static_cast<float>(m_denoiseSpin->value());
  setGenerating(true);
  m_resultLabel->setText(QString());
  m_controller->runInpaint(p);
}

void AiPanel::onCancelClicked() {
  if (m_controller != nullptr) {
    m_controller->cancelAiGeneration();
  }
  setGenerating(false);
  m_resultLabel->setText(QStringLiteral("キャンセルしました"));
}

void AiPanel::onComfyStateChanged(bool connected) {
  updateConnectionStatus(connected);
  m_connectButton->setEnabled(true);
  if (connected) {
    m_controller->fetchAiModels();
  }
}

void AiPanel::onModelsLoaded(const QStringList& models) {
  const QString current = m_modelCombo->currentText();
  m_modelCombo->clear();
  if (models.isEmpty()) {
    m_modelCombo->addItem(QStringLiteral("v1-5-pruned-emaonly.ckpt"));
  } else {
    for (const QString& m : models) {
      m_modelCombo->addItem(m);
    }
    const int idx = m_modelCombo->findText(current);
    if (idx >= 0) m_modelCombo->setCurrentIndex(idx);
  }
}

void AiPanel::onProgressUpdate(int step, int total) {
  if (total > 0) {
    m_progressBar->setRange(0, total);
    m_progressBar->setValue(step);
  } else {
    m_progressBar->setRange(0, 0);  // indeterminate
  }
}

void AiPanel::onGenerationComplete(const QString& opType) {
  setGenerating(false);
  const QString msg = (opType == "inpaint")
      ? QStringLiteral("✓ インペイント完了 — 新規レイヤーに配置されました")
      : QStringLiteral("✓ 生成完了 — 新規レイヤーに配置されました");
  m_resultLabel->setText(msg);
}

void AiPanel::onGenerationError(const QString& message) {
  setGenerating(false);
  m_resultLabel->setStyleSheet("color: #e05555;");
  m_resultLabel->setText(QStringLiteral("✗ ") + message);
}

// ── プライベート ────────────────────────────────────────────────────────────

void AiPanel::setGenerating(bool generating) {
  m_generateButton->setEnabled(!generating);
  m_inpaintButton->setEnabled(!generating);
  m_progressBar->setVisible(generating);
  m_cancelButton->setVisible(generating);
  if (generating) {
    m_progressBar->setRange(0, 0);  // indeterminate until first progress update
    m_resultLabel->setStyleSheet("color: #8af;");
  } else {
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

QString AiPanel::currentCheckpoint() const {
  return m_modelCombo->currentText().trimmed().isEmpty()
      ? QStringLiteral("v1-5-pruned-emaonly.ckpt")
      : m_modelCombo->currentText().trimmed();
}

}  // namespace app::panels
