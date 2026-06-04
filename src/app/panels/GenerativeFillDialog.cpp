#include "app/panels/GenerativeFillDialog.h"

#include <algorithm>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>

#include "platform/qt/QtImageConverter.h"

namespace app::panels {

GenerativeFillDialog::GenerativeFillDialog(
    const core::PixelBuffer&   canvas,
    const core::SelectionMask& mask,
    QWidget*                   parent)
    : QDialog(parent)
    , m_canvas(canvas)
    , m_mask(mask)
    , m_engine(new core::ai::GenerativeFillEngine(this)) {
  setWindowTitle("AI 生成塗りつぶし（β）");
  setMinimumSize(680, 620);
  resize(720, 680);

  connect(m_engine, &core::ai::GenerativeFillEngine::progressChanged,
          this, &GenerativeFillDialog::onProgressChanged);
  connect(m_engine, &core::ai::GenerativeFillEngine::resultReady,
          this, &GenerativeFillDialog::onResultReady);
  connect(m_engine, &core::ai::GenerativeFillEngine::errorOccurred,
          this, &GenerativeFillDialog::onErrorOccurred);

  setupUi();
  updatePreviewThumbnails();
}

// ─────────────────────────────────────────────────────────────────────────────
// UI 構築
// ─────────────────────────────────────────────────────────────────────────────
void GenerativeFillDialog::setupUi() {
  auto* root = new QVBoxLayout(this);
  root->setSpacing(10);
  root->setContentsMargins(12, 12, 12, 12);

  // ── β 警告 ───────────────────────────────────────────────────────────────
  auto* betaLabel = new QLabel(
      "⚠  この機能は実験的です。Stub バックエンドはサンプル変換を行うだけで実際の AI 推論は行いません。",
      this);
  betaLabel->setWordWrap(true);
  betaLabel->setStyleSheet(
      "color: #f0c060; background: #2a2010; border: 1px solid #6a5010;"
      "border-radius: 4px; padding: 6px 8px; font-size: 11px;");
  root->addWidget(betaLabel);

  // ── ビフォー / アフタープレビュー ──────────────────────────────────────────
  auto* previewGroup = new QGroupBox("プレビュー", this);
  auto* previewRow   = new QHBoxLayout(previewGroup);
  previewRow->setSpacing(12);

  auto makePrevBox = [&](const QString& title) {
    auto* col   = new QVBoxLayout();
    auto* label = new QLabel(title, previewGroup);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("font-size: 10px; color: #7a86a3;");
    auto* thumb = new QLabel(previewGroup);
    thumb->setFixedSize(260, 180);
    thumb->setAlignment(Qt::AlignCenter);
    thumb->setStyleSheet("background: #0d0f14; border: 1px solid #2a2e3e; border-radius: 3px;");
    col->addWidget(label);
    col->addWidget(thumb);
    previewRow->addLayout(col);
    return thumb;
  };

  m_beforeThumb = makePrevBox("変更前");
  previewRow->addStretch(1);
  m_afterThumb  = makePrevBox("生成結果");

  root->addWidget(previewGroup);

  // ── プロンプト ────────────────────────────────────────────────────────────
  auto* promptGroup  = new QGroupBox("プロンプト", this);
  auto* promptLayout = new QVBoxLayout(promptGroup);

  auto* promptLabel = new QLabel("生成内容の説明（英語が推奨）:", promptGroup);
  m_promptEdit = new QTextEdit(promptGroup);
  m_promptEdit->setPlaceholderText(
      "例: smooth skin texture, soft lighting, natural colors");
  m_promptEdit->setFixedHeight(58);

  auto* negLabel = new QLabel("ネガティブプロンプト（除外したい要素）:", promptGroup);
  m_negPromptEdit = new QTextEdit(promptGroup);
  m_negPromptEdit->setPlaceholderText(
      "例: blurry, low quality, artifacts, noise");
  m_negPromptEdit->setFixedHeight(44);

  promptLayout->addWidget(promptLabel);
  promptLayout->addWidget(m_promptEdit);
  promptLayout->addWidget(negLabel);
  promptLayout->addWidget(m_negPromptEdit);
  root->addWidget(promptGroup);

  // ── 生成パラメータ ────────────────────────────────────────────────────────
  auto* paramGroup  = new QGroupBox("生成パラメータ", this);
  auto* paramLayout = new QHBoxLayout(paramGroup);
  paramLayout->setSpacing(16);

  // ステップ数
  auto* colSteps   = new QVBoxLayout();
  auto* stepsLabel = new QLabel("ステップ数", paramGroup);
  stepsLabel->setAlignment(Qt::AlignCenter);
  m_stepsSpin = new QSpinBox(paramGroup);
  m_stepsSpin->setRange(1, 100);
  m_stepsSpin->setValue(20);
  m_stepsSpin->setToolTip("推論ステップ数。多いほど精度が上がりますが時間がかかります。");
  colSteps->addWidget(stepsLabel);
  colSteps->addWidget(m_stepsSpin);
  paramLayout->addLayout(colSteps);

  // CFG スケール
  auto* colGuid     = new QVBoxLayout();
  auto* guidLabel   = new QLabel("CFG スケール", paramGroup);
  guidLabel->setAlignment(Qt::AlignCenter);
  m_guidanceSpin = new QDoubleSpinBox(paramGroup);
  m_guidanceSpin->setRange(1.0, 30.0);
  m_guidanceSpin->setValue(7.5);
  m_guidanceSpin->setSingleStep(0.5);
  m_guidanceSpin->setToolTip("プロンプトへの忠実度。高いほど指示に従います。");
  colGuid->addWidget(guidLabel);
  colGuid->addWidget(m_guidanceSpin);
  paramLayout->addLayout(colGuid);

  // 強度
  auto* colStr    = new QVBoxLayout();
  auto* strLabel  = new QLabel("インペイント強度", paramGroup);
  strLabel->setAlignment(Qt::AlignCenter);
  m_strengthSlider = new QSlider(Qt::Horizontal, paramGroup);
  m_strengthSlider->setRange(0, 100);
  m_strengthSlider->setValue(80);
  m_strengthLabel = new QLabel("80%", paramGroup);
  m_strengthLabel->setAlignment(Qt::AlignCenter);
  m_strengthLabel->setFixedWidth(36);
  connect(m_strengthSlider, &QSlider::valueChanged, this, [this](int v) {
    m_strengthLabel->setText(QString("%1%").arg(v));
  });
  auto* strRow = new QHBoxLayout();
  strRow->addWidget(m_strengthSlider);
  strRow->addWidget(m_strengthLabel);
  colStr->addWidget(strLabel);
  colStr->addLayout(strRow);
  paramLayout->addLayout(colStr, 1);

  // シード
  auto* colSeed    = new QVBoxLayout();
  auto* seedLabel  = new QLabel("シード", paramGroup);
  seedLabel->setAlignment(Qt::AlignCenter);
  m_seedSpin = new QSpinBox(paramGroup);
  m_seedSpin->setRange(-1, 2147483647);
  m_seedSpin->setValue(-1);
  m_seedSpin->setSpecialValueText("ランダム");
  m_seedSpin->setToolTip("-1 でランダムシード。固定値で再現性を確保できます。");
  colSeed->addWidget(seedLabel);
  colSeed->addWidget(m_seedSpin);
  paramLayout->addLayout(colSeed);

  // バックエンド
  auto* colBack   = new QVBoxLayout();
  auto* backLabel = new QLabel("バックエンド", paramGroup);
  backLabel->setAlignment(Qt::AlignCenter);
  m_backendCombo = new QComboBox(paramGroup);
  m_backendCombo->addItem("スタブ（テスト）",  static_cast<int>(core::ai::GenerativeFillEngine::Backend::Stub));
  m_backendCombo->addItem("ローカル ONNX（未実装）", static_cast<int>(core::ai::GenerativeFillEngine::Backend::LocalOnnx));
  m_backendCombo->addItem("リモート API（未実装）",  static_cast<int>(core::ai::GenerativeFillEngine::Backend::ApiRemote));
  colBack->addWidget(backLabel);
  colBack->addWidget(m_backendCombo);
  paramLayout->addLayout(colBack);

  root->addWidget(paramGroup);

  // ── 生成ボタン + プログレス ────────────────────────────────────────────────
  auto* actionRow = new QHBoxLayout();
  m_generateBtn = new QPushButton("生成する", this);
  m_generateBtn->setMinimumHeight(36);
  m_generateBtn->setStyleSheet(
      "QPushButton { background: #1d4a8a; border: 1px solid #4e8ef7; color: #edf0f9;"
      "font-weight: 700; border-radius: 5px; font-size: 13px; }"
      "QPushButton:hover { background: #2a60b0; }"
      "QPushButton:disabled { background: #1a1d27; color: #4a5268; border-color: #252a38; }");
  m_cancelGenBtn = new QPushButton("中断", this);
  m_cancelGenBtn->setMinimumHeight(36);
  m_cancelGenBtn->setEnabled(false);
  actionRow->addWidget(m_generateBtn, 1);
  actionRow->addWidget(m_cancelGenBtn);
  root->addLayout(actionRow);

  m_progressBar = new QProgressBar(this);
  m_progressBar->setRange(0, 100);
  m_progressBar->setValue(0);
  m_progressBar->setTextVisible(true);
  m_progressBar->setVisible(false);
  m_progressBar->setFixedHeight(12);
  root->addWidget(m_progressBar);

  m_statusLabel = new QLabel("準備完了", this);
  m_statusLabel->setStyleSheet("color: #7a86a3; font-size: 10px;");
  root->addWidget(m_statusLabel);

  // ── ダイアログボタン ─────────────────────────────────────────────────────
  m_dialogButtons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  m_dialogButtons->button(QDialogButtonBox::Ok)->setText("適用");
  m_dialogButtons->button(QDialogButtonBox::Ok)->setEnabled(false);
  m_dialogButtons->button(QDialogButtonBox::Ok)->setToolTip(
      "生成結果をキャンバスに適用します。生成後に有効になります。");
  root->addWidget(m_dialogButtons);

  connect(m_generateBtn,                  &QPushButton::clicked,
          this, &GenerativeFillDialog::onGenerateClicked);
  connect(m_cancelGenBtn,                 &QPushButton::clicked,
          this, &GenerativeFillDialog::onCancelGeneration);
  connect(m_dialogButtons,                &QDialogButtonBox::accepted,
          this, &QDialog::accept);
  connect(m_dialogButtons,                &QDialogButtonBox::rejected,
          this, &QDialog::reject);
}

// ─────────────────────────────────────────────────────────────────────────────
// サムネイル更新
// ─────────────────────────────────────────────────────────────────────────────
void GenerativeFillDialog::updatePreviewThumbnails() {
  const QSize thumbSize(260, 180);
  m_beforeThumb->setPixmap(bufferToPixmap(m_canvas, thumbSize));
}

QPixmap GenerativeFillDialog::bufferToPixmap(const core::PixelBuffer& buf,
                                              const QSize& size) {
  const QImage img = platform::qt::QtImageConverter::toQImage(buf);
  if (img.isNull()) {
    QPixmap blank(size);
    blank.fill(Qt::darkGray);
    return blank;
  }
  return QPixmap::fromImage(img).scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

// ─────────────────────────────────────────────────────────────────────────────
// 生成ボタン
// ─────────────────────────────────────────────────────────────────────────────
void GenerativeFillDialog::onGenerateClicked() {
  if (m_engine->isRunning()) {
    return;
  }

  core::ai::GenerativeFillEngine::Settings s;
  s.prompt         = m_promptEdit->toPlainText().trimmed();
  s.negativePrompt = m_negPromptEdit->toPlainText().trimmed();
  s.steps          = m_stepsSpin->value();
  s.guidance       = static_cast<float>(m_guidanceSpin->value());
  s.strength       = m_strengthSlider->value() / 100.0f;
  s.seed           = m_seedSpin->value();
  s.backend        = static_cast<core::ai::GenerativeFillEngine::Backend>(
                         m_backendCombo->currentData().toInt());

  m_hasResult = false;
  m_dialogButtons->button(QDialogButtonBox::Ok)->setEnabled(false);
  m_afterThumb->clear();
  m_afterThumb->setStyleSheet(
      "background: #0d0f14; border: 1px solid #2a2e3e; border-radius: 3px;");

  setGenerating(true);
  m_statusLabel->setText(QString("生成中... (%1 ステップ)").arg(s.steps));
  m_engine->generate(m_canvas, m_mask, s);
}

void GenerativeFillDialog::onCancelGeneration() {
  m_engine->cancel();
  m_statusLabel->setText("キャンセルしました。");
  setGenerating(false);
}

void GenerativeFillDialog::onProgressChanged(int percent) {
  m_progressBar->setValue(percent);
}

void GenerativeFillDialog::onResultReady(core::PixelBuffer result) {
  m_result    = std::move(result);
  m_hasResult = true;

  setGenerating(false);
  m_statusLabel->setText("生成完了！「適用」ボタンでキャンバスに反映できます。");
  m_dialogButtons->button(QDialogButtonBox::Ok)->setEnabled(true);

  // アフタープレビュー更新
  const QSize thumbSize(260, 180);
  m_afterThumb->setPixmap(bufferToPixmap(m_result, thumbSize));
}

void GenerativeFillDialog::onErrorOccurred(const QString& message) {
  setGenerating(false);
  m_statusLabel->setText(QString("エラー: %1").arg(message));
  QMessageBox::warning(this, "生成エラー", message);
}

// ─────────────────────────────────────────────────────────────────────────────
// 生成中 UI 切り替え
// ─────────────────────────────────────────────────────────────────────────────
void GenerativeFillDialog::setGenerating(bool generating) {
  m_generateBtn->setEnabled(!generating);
  m_cancelGenBtn->setEnabled(generating);
  m_progressBar->setVisible(generating);
  if (!generating) {
    m_progressBar->setValue(0);
  }
  m_promptEdit->setReadOnly(generating);
  m_negPromptEdit->setReadOnly(generating);
  m_stepsSpin->setEnabled(!generating);
  m_guidanceSpin->setEnabled(!generating);
  m_strengthSlider->setEnabled(!generating);
  m_seedSpin->setEnabled(!generating);
  m_backendCombo->setEnabled(!generating);
  m_dialogButtons->button(QDialogButtonBox::Cancel)->setEnabled(!generating);
}

} // namespace app::panels
