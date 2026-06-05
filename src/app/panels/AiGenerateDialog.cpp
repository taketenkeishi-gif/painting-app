#include "app/panels/AiGenerateDialog.h"
#include "app/bridge/AppController.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>

namespace app::panels {

namespace {

const char kDefaultNegative[] =
    "low quality, bad anatomy, worst quality, blurry, text, watermark";

QLabel* makeLabel(const QString& text, QWidget* parent) {
  auto* lbl = new QLabel(text, parent);
  lbl->setStyleSheet("color:#9095ab; font-size:11px;");
  return lbl;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
AiGenerateDialog::AiGenerateDialog(Mode mode,
                                   app::bridge::AppController* controller,
                                   QWidget* parent)
    : QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint),
      m_mode(mode),
      m_controller(controller) {
  setWindowTitle(modeName());
  setMinimumWidth(400);
  setupUi();
  setupConnections();

  // 利用可能モデル一覧を非同期取得
  if (m_controller && m_controller->isComfyUiConnected()) {
    m_controller->fetchAiModels();
  }
}

// ─────────────────────────────────────────────────────────────────────────────
QString AiGenerateDialog::modeName() const {
  switch (m_mode) {
    case Mode::Img2Img:  return QString::fromUtf8(u8"img2img");
    case Mode::Inpaint:  return QString::fromUtf8(u8"生成塗りつぶし（インペイント）");
    case Mode::Scribble: return QString::fromUtf8(u8"ControlNet Scribble");
    case Mode::Canny:    return QString::fromUtf8(u8"ControlNet Canny");
    case Mode::Lineart:  return QString::fromUtf8(u8"ControlNet ラインアート");
  }
  return {};
}

QString AiGenerateDialog::workflowResource() const {
  switch (m_mode) {
    case Mode::Img2Img:  return ":/workflows/paintapp_img2img.json";
    case Mode::Inpaint:  return ":/workflows/paintapp_inpaint.json";
    case Mode::Scribble: return ":/workflows/paintapp_scribble.json";
    case Mode::Canny:    return ":/workflows/paintapp_canny.json";
    case Mode::Lineart:  return ":/workflows/paintapp_lineart.json";
  }
  return {};
}

bool AiGenerateDialog::needsCnControls() const {
  return m_mode == Mode::Scribble || m_mode == Mode::Canny || m_mode == Mode::Lineart;
}

bool AiGenerateDialog::needsMask() const {
  return m_mode == Mode::Inpaint;
}

// ─────────────────────────────────────────────────────────────────────────────
void AiGenerateDialog::setupUi() {
  const QString groupStyle =
      "QGroupBox { border:1px solid #2d3248; border-radius:4px;"
      " margin-top:8px; font-size:11px; color:#9095ab; }"
      "QGroupBox::title { subcontrol-origin:margin; left:6px; top:2px; }";
  const QString inputStyle =
      "background:#1a1d27; border:1px solid #2d3248; border-radius:3px;"
      " color:#c8ccd8; font-size:12px; padding:3px;";
  const QString sliderStyle =
      "QSlider::groove:horizontal { height:4px; background:#2d3248; border-radius:2px; }"
      "QSlider::handle:horizontal { width:12px; height:12px; margin:-4px 0;"
      " background:#4e8ef7; border-radius:6px; }"
      "QSlider::sub-page:horizontal { background:#4e8ef7; border-radius:2px; }";

  auto* root = new QVBoxLayout(this);
  root->setSpacing(8);
  root->setContentsMargins(10, 10, 10, 10);

  // ── モデル & 入力元 ───────────────────────────────────────────────────────
  {
    auto* grp = new QGroupBox(QString::fromUtf8(u8"モデル / 入力"), this);
    grp->setStyleSheet(groupStyle);
    auto* form = new QFormLayout(grp);
    form->setContentsMargins(8, 16, 8, 8);
    form->setSpacing(6);

    m_checkpointCombo = new QComboBox(grp);
    m_checkpointCombo->setEditable(true);
    m_checkpointCombo->addItem("v1-5-pruned-emaonly.ckpt");
    m_checkpointCombo->setStyleSheet(inputStyle);
    form->addRow(makeLabel(QString::fromUtf8(u8"チェックポイント"), grp), m_checkpointCombo);

    m_sourceCombo = new QComboBox(grp);
    m_sourceCombo->addItem(QString::fromUtf8(u8"アクティブレイヤー"));
    m_sourceCombo->addItem(QString::fromUtf8(u8"合成全体"));
    m_sourceCombo->setStyleSheet(inputStyle);
    form->addRow(makeLabel(QString::fromUtf8(u8"入力元"), grp), m_sourceCombo);

    root->addWidget(grp);
  }

  // ── プロンプト ──────────────────────────────────────────────────────────────
  {
    auto* grp = new QGroupBox(QString::fromUtf8(u8"プロンプト"), this);
    grp->setStyleSheet(groupStyle);
    auto* lay = new QVBoxLayout(grp);
    lay->setContentsMargins(8, 16, 8, 8);
    lay->setSpacing(4);

    m_promptEdit = new QTextEdit(grp);
    m_promptEdit->setPlaceholderText(QString::fromUtf8(u8"プロンプトを入力..."));
    m_promptEdit->setFixedHeight(54);
    m_promptEdit->setStyleSheet(inputStyle);
    lay->addWidget(m_promptEdit);

    lay->addWidget(makeLabel(QString::fromUtf8(u8"ネガティブ"), grp));
    m_negativeEdit = new QTextEdit(grp);
    m_negativeEdit->setPlainText(kDefaultNegative);
    m_negativeEdit->setFixedHeight(40);
    m_negativeEdit->setStyleSheet(inputStyle);
    lay->addWidget(m_negativeEdit);

    root->addWidget(grp);
  }

  // ── パラメータ ──────────────────────────────────────────────────────────────
  {
    auto* grp = new QGroupBox(QString::fromUtf8(u8"パラメータ"), this);
    grp->setStyleSheet(groupStyle);
    auto* form = new QFormLayout(grp);
    form->setContentsMargins(8, 16, 8, 8);
    form->setSpacing(6);

    // 変換強度
    {
      auto* row = new QHBoxLayout();
      m_denoiseSlider = new QSlider(Qt::Horizontal, grp);
      m_denoiseSlider->setRange(1, 100);
      m_denoiseSlider->setValue(80);
      m_denoiseSlider->setStyleSheet(sliderStyle);
      m_denoiseLabel = new QLabel("0.80", grp);
      m_denoiseLabel->setFixedWidth(34);
      m_denoiseLabel->setStyleSheet("color:#c8ccd8; font-size:11px;");
      row->addWidget(m_denoiseSlider);
      row->addWidget(m_denoiseLabel);
      form->addRow(makeLabel(QString::fromUtf8(u8"変換強度"), grp), row);
    }

    // ControlNet 強度（CN モードのみ）
    if (needsCnControls()) {
      auto* row = new QHBoxLayout();
      m_cnStrengthSlider = new QSlider(Qt::Horizontal, grp);
      m_cnStrengthSlider->setRange(1, 100);
      m_cnStrengthSlider->setValue(100);
      m_cnStrengthSlider->setStyleSheet(sliderStyle);
      m_cnStrengthLabel = new QLabel("1.00", grp);
      m_cnStrengthLabel->setFixedWidth(34);
      m_cnStrengthLabel->setStyleSheet("color:#c8ccd8; font-size:11px;");
      row->addWidget(m_cnStrengthSlider);
      row->addWidget(m_cnStrengthLabel);
      m_cnModelLabel = makeLabel(QString::fromUtf8(u8"CN 強度"), grp);
      form->addRow(m_cnModelLabel, row);

      m_cnModelEdit = new QLineEdit(grp);
      m_cnModelEdit->setStyleSheet(inputStyle);
      // デフォルトモデル名をモードに合わせる
      switch (m_mode) {
        case Mode::Scribble: m_cnModelEdit->setText("control_v11p_sd15_scribble.pth"); break;
        case Mode::Canny:    m_cnModelEdit->setText("control_v11p_sd15_canny.pth");    break;
        case Mode::Lineart:  m_cnModelEdit->setText("control_v11p_sd15_lineart.pth");  break;
        default: break;
      }
      form->addRow(makeLabel(QString::fromUtf8(u8"CN モデル"), grp), m_cnModelEdit);
    }

    // Steps / CFG / Seed
    {
      auto* row = new QHBoxLayout();
      row->setSpacing(8);

      m_stepsSpinBox = new QSpinBox(grp);
      m_stepsSpinBox->setRange(1, 150);
      m_stepsSpinBox->setValue(20);
      m_stepsSpinBox->setStyleSheet(inputStyle);
      m_stepsSpinBox->setFixedWidth(58);

      m_cfgSpinBox = new QDoubleSpinBox(grp);
      m_cfgSpinBox->setRange(1.0, 30.0);
      m_cfgSpinBox->setSingleStep(0.5);
      m_cfgSpinBox->setValue(7.5);
      m_cfgSpinBox->setStyleSheet(inputStyle);
      m_cfgSpinBox->setFixedWidth(60);

      m_seedSpinBox = new QSpinBox(grp);
      m_seedSpinBox->setRange(-1, 2147483647);
      m_seedSpinBox->setValue(-1);
      m_seedSpinBox->setSpecialValueText(QString::fromUtf8(u8"ランダム"));
      m_seedSpinBox->setStyleSheet(inputStyle);

      auto* stepsLbl = makeLabel("Steps", grp);
      auto* cfgLbl   = makeLabel("CFG",   grp);
      auto* seedLbl  = makeLabel("Seed",  grp);

      auto* stepsCol = new QVBoxLayout(); stepsCol->setSpacing(2);
      stepsCol->addWidget(stepsLbl); stepsCol->addWidget(m_stepsSpinBox);
      auto* cfgCol = new QVBoxLayout(); cfgCol->setSpacing(2);
      cfgCol->addWidget(cfgLbl); cfgCol->addWidget(m_cfgSpinBox);
      auto* seedCol = new QVBoxLayout(); seedCol->setSpacing(2);
      seedCol->addWidget(seedLbl); seedCol->addWidget(m_seedSpinBox);

      row->addLayout(stepsCol);
      row->addLayout(cfgCol);
      row->addLayout(seedCol);
      form->addRow(row);
    }

    // バッチ数
    {
      m_batchCombo = new QComboBox(grp);
      m_batchCombo->addItem(QString::fromUtf8(u8"×1"));
      m_batchCombo->addItem(QString::fromUtf8(u8"×2"));
      m_batchCombo->addItem(QString::fromUtf8(u8"×3"));
      m_batchCombo->setStyleSheet(inputStyle);
      m_batchCombo->setFixedWidth(70);
      form->addRow(makeLabel(QString::fromUtf8(u8"バッチ"), grp), m_batchCombo);
    }

    root->addWidget(grp);
  }

  // ── 進捗 & ボタン ─────────────────────────────────────────────────────────
  {
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setStyleSheet(
        "QProgressBar { background:#1a1d27; border-radius:3px; }"
        "QProgressBar::chunk { background:#4e8ef7; border-radius:3px; }");
    m_progressBar->setVisible(false);
    root->addWidget(m_progressBar);

    m_statusLabel = new QLabel("", this);
    m_statusLabel->setStyleSheet("color:#9095ab; font-size:11px;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_statusLabel);

    auto* btnRow = new QHBoxLayout();
    m_cancelBtn = new QPushButton(QString::fromUtf8(u8"キャンセル"), this);
    m_cancelBtn->setStyleSheet(
        "QPushButton { background:#252838; color:#9095ab; border:1px solid #353a50;"
        " border-radius:4px; padding:6px 14px; }"
        "QPushButton:hover { background:#2d3248; color:#c8ccd8; }");
    m_cancelBtn->setVisible(false);

    m_generateBtn = new QPushButton(QString::fromUtf8(u8"生　成"), this);
    m_generateBtn->setStyleSheet(
        "QPushButton { background:#2a3a5e; color:#7fb3f5; border:1px solid #4e8ef7;"
        " border-radius:4px; padding:8px 24px; font-size:13px; font-weight:600; }"
        "QPushButton:hover { background:#1e4080; color:#c5d0e8; }"
        "QPushButton:disabled { background:#252838; color:#555a70; border-color:#353a50; }");

    btnRow->addStretch();
    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_generateBtn);
    root->addLayout(btnRow);
  }

  setStyleSheet("QDialog { background:#13151c; }");
}

// ─────────────────────────────────────────────────────────────────────────────
void AiGenerateDialog::setupConnections() {
  // 変換強度スライダー → ラベル同期
  connect(m_denoiseSlider, &QSlider::valueChanged, this, [this](int v) {
    m_denoiseLabel->setText(QString::number(v / 100.0, 'f', 2));
  });
  if (m_cnStrengthSlider) {
    connect(m_cnStrengthSlider, &QSlider::valueChanged, this, [this](int v) {
      m_cnStrengthLabel->setText(QString::number(v / 100.0, 'f', 2));
    });
  }

  connect(m_generateBtn, &QPushButton::clicked, this, &AiGenerateDialog::onGenerate);
  connect(m_cancelBtn, &QPushButton::clicked, this, [this]() {
    if (m_controller) m_controller->cancelAiGeneration();
  });

  if (m_controller) {
    // モデルリスト取得
    connect(m_controller, &bridge::AppController::aiModelsLoaded, this,
            [this](const QStringList& models) {
              const QString current = m_checkpointCombo->currentText();
              m_checkpointCombo->clear();
              for (const QString& m : models) m_checkpointCombo->addItem(m);
              if (!current.isEmpty()) {
                const int idx = m_checkpointCombo->findText(current);
                if (idx >= 0) m_checkpointCombo->setCurrentIndex(idx);
                else m_checkpointCombo->setEditText(current);
              }
            });

    // 進捗
    connect(m_controller, &bridge::AppController::aiProgressUpdate, this,
            [this](int step, int total, const QString& nodeId) {
              onProgress(step, total, nodeId);
            });

    // 完了
    connect(m_controller, &bridge::AppController::aiGenerationComplete, this,
            [this](const QString&) { onComplete(); });

    // エラー
    connect(m_controller, &bridge::AppController::aiGenerationError, this,
            [this](const QString& msg) { onError(msg); });
  }
}

// ─────────────────────────────────────────────────────────────────────────────
void AiGenerateDialog::onGenerate() {
  if (!m_controller) return;
  if (!m_controller->isComfyUiConnected()) {
    m_statusLabel->setText(QString::fromUtf8(u8"ComfyUI に接続されていません"));
    return;
  }
  if (needsMask() && !m_controller->documentSelection().hasSelection()) {
    m_statusLabel->setText(QString::fromUtf8(u8"インペイントには選択範囲が必要です"));
    return;
  }

  m_generating = true;
  m_generateBtn->setEnabled(false);
  m_cancelBtn->setVisible(true);
  m_progressBar->setVisible(true);
  m_progressBar->setValue(0);
  m_statusLabel->setText(QString::fromUtf8(u8"準備中..."));

  app::bridge::AppController::ControlNetParams params;
  params.workflowResource  = workflowResource();
  params.prompt            = m_promptEdit->toPlainText();
  params.negativePrompt    = m_negativeEdit->toPlainText();
  params.checkpoint        = m_checkpointCombo->currentText();
  params.denoise           = m_denoiseSlider->value() / 100.0f;
  params.steps             = m_stepsSpinBox->value();
  params.cfg               = static_cast<float>(m_cfgSpinBox->value());
  params.seed              = m_seedSpinBox->value();
  params.batchCount        = m_batchCombo->currentIndex() + 1;
  params.useActiveLayerOnly = (m_sourceCombo->currentIndex() == 0);

  if (needsCnControls() && m_cnStrengthSlider && m_cnModelEdit) {
    params.cnStrength = m_cnStrengthSlider->value() / 100.0f;
    params.cnModel    = m_cnModelEdit->text();
  }

  m_controller->runControlNet(params);
}

// ─────────────────────────────────────────────────────────────────────────────
void AiGenerateDialog::onProgress(int step, int total, const QString& /*nodeId*/) {
  if (total > 0) {
    m_progressBar->setValue(step * 100 / total);
    m_statusLabel->setText(
        QString::fromUtf8(u8"生成中... %1 / %2").arg(step).arg(total));
  }
}

void AiGenerateDialog::onComplete() {
  m_generating = false;
  m_generateBtn->setEnabled(true);
  m_cancelBtn->setVisible(false);
  m_progressBar->setValue(100);
  m_statusLabel->setText(QString::fromUtf8(u8"完了"));
}

void AiGenerateDialog::onError(const QString& msg) {
  m_generating = false;
  m_generateBtn->setEnabled(true);
  m_cancelBtn->setVisible(false);
  m_progressBar->setVisible(false);
  m_statusLabel->setText(QString::fromUtf8(u8"エラー: ") + msg);
}

} // namespace app::panels
