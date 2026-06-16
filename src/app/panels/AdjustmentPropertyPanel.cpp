#include "app/panels/AdjustmentPropertyPanel.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "app/bridge/AppController.h"
#include "core/layer/Layer.h"

namespace app::panels {

// ─────────────────────────────────────────────────────────────────────────────
// helpers
// ─────────────────────────────────────────────────────────────────────────────

QSlider* AdjustmentPropertyPanel::makeSlider(int min, int max, int value, QWidget* parent) {
  auto* s = new QSlider(Qt::Horizontal, parent);
  s->setRange(min, max);
  s->setValue(value);
  return s;
}

QSpinBox* AdjustmentPropertyPanel::makeSpinBox(int min, int max, int value, QWidget* parent) {
  auto* s = new QSpinBox(parent);
  s->setRange(min, max);
  s->setValue(value);
  s->setFixedWidth(54);
  return s;
}

void AdjustmentPropertyPanel::addRow(QVBoxLayout* layout, const QString& label,
                                     QSlider* slider, QSpinBox* spin) {
  auto* lbl = new QLabel(label);
  lbl->setFixedHeight(14);
  layout->addWidget(lbl);

  auto* row = new QWidget;
  auto* h   = new QHBoxLayout(row);
  h->setContentsMargins(0, 0, 0, 0);
  h->setSpacing(4);
  h->addWidget(slider, 1);
  h->addWidget(spin);
  layout->addWidget(row);

  // sync slider ↔ spinbox
  connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
  connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), slider, &QSlider::setValue);
}

// ─────────────────────────────────────────────────────────────────────────────
// page builders
// ─────────────────────────────────────────────────────────────────────────────

QWidget* AdjustmentPropertyPanel::buildBrightnessContrastPage() {
  auto* w      = new QWidget;
  auto* layout = new QVBoxLayout(w);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(2);

  m_brightnessSlider = makeSlider(-100, 100, 0);
  m_brightnessSpin   = makeSpinBox(-100, 100, 0);
  addRow(layout, "明るさ", m_brightnessSlider, m_brightnessSpin);

  m_contrastSlider = makeSlider(-100, 100, 0);
  m_contrastSpin   = makeSpinBox(-100, 100, 0);
  addRow(layout, "コントラスト", m_contrastSlider, m_contrastSpin);

  layout->addStretch();

  connect(m_brightnessSlider, &QSlider::valueChanged, this, &AdjustmentPropertyPanel::onBrightnessChanged);
  connect(m_contrastSlider,   &QSlider::valueChanged, this, &AdjustmentPropertyPanel::onContrastChanged);

  return w;
}

QWidget* AdjustmentPropertyPanel::buildHueSaturationPage() {
  auto* w      = new QWidget;
  auto* layout = new QVBoxLayout(w);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(2);

  m_hueSlider       = makeSlider(-180, 180, 0);
  m_hueSpin         = makeSpinBox(-180, 180, 0);
  addRow(layout, "色相", m_hueSlider, m_hueSpin);

  m_saturationSlider = makeSlider(-100, 100, 0);
  m_saturationSpin   = makeSpinBox(-100, 100, 0);
  addRow(layout, "彩度", m_saturationSlider, m_saturationSpin);

  m_lightnessSlider = makeSlider(-100, 100, 0);
  m_lightnessSpin   = makeSpinBox(-100, 100, 0);
  addRow(layout, "明度", m_lightnessSlider, m_lightnessSpin);

  layout->addStretch();

  connect(m_hueSlider,        &QSlider::valueChanged, this, &AdjustmentPropertyPanel::onHueChanged);
  connect(m_saturationSlider, &QSlider::valueChanged, this, &AdjustmentPropertyPanel::onSaturationChanged);
  connect(m_lightnessSlider,  &QSlider::valueChanged, this, &AdjustmentPropertyPanel::onLightnessChanged);

  return w;
}

QWidget* AdjustmentPropertyPanel::buildLevelsPage() {
  auto* w      = new QWidget;
  auto* layout = new QVBoxLayout(w);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(2);

  m_inputBlackSlider  = makeSlider(0, 255, 0);
  m_inputBlackSpin    = makeSpinBox(0, 255, 0);
  addRow(layout, "入力黒点", m_inputBlackSlider, m_inputBlackSpin);

  m_inputWhiteSlider  = makeSlider(0, 255, 255);
  m_inputWhiteSpin    = makeSpinBox(0, 255, 255);
  addRow(layout, "入力白点", m_inputWhiteSlider, m_inputWhiteSpin);

  // gamma slider: range 10..999 represents 0.10..9.99, display as integer /100
  m_gammaSlider = makeSlider(10, 999, 100);
  m_gammaSpin   = makeSpinBox(10, 999, 100);
  addRow(layout, "ガンマ (×0.01)", m_gammaSlider, m_gammaSpin);

  m_outputBlackSlider = makeSlider(0, 255, 0);
  m_outputBlackSpin   = makeSpinBox(0, 255, 0);
  addRow(layout, "出力黒点", m_outputBlackSlider, m_outputBlackSpin);

  m_outputWhiteSlider = makeSlider(0, 255, 255);
  m_outputWhiteSpin   = makeSpinBox(0, 255, 255);
  addRow(layout, "出力白点", m_outputWhiteSlider, m_outputWhiteSpin);

  layout->addStretch();

  connect(m_inputBlackSlider,  &QSlider::valueChanged, this, &AdjustmentPropertyPanel::onInputBlackChanged);
  connect(m_inputWhiteSlider,  &QSlider::valueChanged, this, &AdjustmentPropertyPanel::onInputWhiteChanged);
  connect(m_gammaSlider,       &QSlider::valueChanged, this, &AdjustmentPropertyPanel::onGammaChanged);
  connect(m_outputBlackSlider, &QSlider::valueChanged, this, &AdjustmentPropertyPanel::onOutputBlackChanged);
  connect(m_outputWhiteSlider, &QSlider::valueChanged, this, &AdjustmentPropertyPanel::onOutputWhiteChanged);

  return w;
}

QWidget* AdjustmentPropertyPanel::buildVibrancePage() {
  auto* w      = new QWidget;
  auto* layout = new QVBoxLayout(w);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(2);

  m_vibranceSlider = makeSlider(-100, 100, 0);
  m_vibranceSpin   = makeSpinBox(-100, 100, 0);
  addRow(layout, "自然な彩度", m_vibranceSlider, m_vibranceSpin);

  layout->addStretch();

  connect(m_vibranceSlider, &QSlider::valueChanged, this, &AdjustmentPropertyPanel::onVibranceChanged);

  return w;
}

// ─────────────────────────────────────────────────────────────────────────────
// constructor
// ─────────────────────────────────────────────────────────────────────────────

AdjustmentPropertyPanel::AdjustmentPropertyPanel(QWidget* parent)
    : QWidget(parent) {
  auto* outerLayout = new QVBoxLayout(this);
  outerLayout->setContentsMargins(4, 4, 4, 4);
  outerLayout->setSpacing(4);

  m_titleLabel = new QLabel("調整レイヤー");
  m_titleLabel->setAlignment(Qt::AlignCenter);
  QFont f = m_titleLabel->font();
  f.setBold(true);
  m_titleLabel->setFont(f);
  outerLayout->addWidget(m_titleLabel);

  m_stack = new QStackedWidget;
  // Pages indexed by AdjustmentKind cast to int:
  //   BrightnessContrast=0, HueSaturation=1, ColorBalance=2, Levels=3,
  //   Curves=4, GradientMap=5, Invert=6, Threshold=7, Vibrance=8
  // We build only the 4 supported types; unsupported pages are plain QWidget.
  for (int i = 0; i < 9; ++i) {
    QWidget* page = nullptr;
    switch (static_cast<core::AdjustmentKind>(i)) {
      case core::AdjustmentKind::BrightnessContrast: page = buildBrightnessContrastPage(); break;
      case core::AdjustmentKind::HueSaturation:      page = buildHueSaturationPage();      break;
      case core::AdjustmentKind::Levels:             page = buildLevelsPage();             break;
      case core::AdjustmentKind::Vibrance:           page = buildVibrancePage();           break;
      default:
        page = new QWidget;
        auto* lbl = new QLabel("このタイプは現在編集未対応です", page);
        auto* pl = new QVBoxLayout(page);
        pl->addWidget(lbl);
        pl->addStretch();
        break;
    }
    m_stack->addWidget(page);
  }

  auto* scrollArea = new QScrollArea;
  scrollArea->setWidgetResizable(true);
  scrollArea->setWidget(m_stack);
  outerLayout->addWidget(scrollArea, 1);

  setVisible(false);
}

// ─────────────────────────────────────────────────────────────────────────────
// controller wiring
// ─────────────────────────────────────────────────────────────────────────────

void AdjustmentPropertyPanel::setController(app::bridge::AppController* controller) {
  m_controller = controller;
  if (m_controller) {
    connect(m_controller, &app::bridge::AppController::layersChanged,
            this, &AdjustmentPropertyPanel::refreshFromController);
    connect(m_controller, &app::bridge::AppController::toolStateChanged,
            this, &AdjustmentPropertyPanel::refreshFromController);
    refreshFromController();
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// refresh from controller
// ─────────────────────────────────────────────────────────────────────────────

void AdjustmentPropertyPanel::refreshFromController() {
  if (!m_controller) {
    setVisible(false);
    return;
  }

  const core::Document& doc = m_controller->document();
  if (doc.layerCount() == 0) {
    setVisible(false);
    return;
  }

  const core::Layer* layer = doc.activeLayer();
  if (!layer || !layer->isAdjustment()) {
    setVisible(false);
    return;
  }

  setVisible(true);

  const core::AdjustmentParams& p = layer->adjustmentParams();
  const int kindIdx = static_cast<int>(p.kind);

  m_refreshing = true;

  m_titleLabel->setText(QString("調整レイヤー: %1").arg([&]() -> const char* {
    switch (p.kind) {
      case core::AdjustmentKind::BrightnessContrast: return "明るさ・コントラスト";
      case core::AdjustmentKind::HueSaturation:      return "色相・彩度";
      case core::AdjustmentKind::Levels:             return "レベル補正";
      case core::AdjustmentKind::Vibrance:           return "自然な彩度";
      default: return "調整";
    }
  }()));

  if (kindIdx >= 0 && kindIdx < m_stack->count()) {
    m_stack->setCurrentIndex(kindIdx);
  }

  // Update controls based on kind
  switch (p.kind) {
    case core::AdjustmentKind::BrightnessContrast:
      if (m_brightnessSlider) {
        m_brightnessSlider->setValue(static_cast<int>(p.brightness * 100.0f));
        m_contrastSlider->setValue(static_cast<int>(p.contrast * 100.0f));
      }
      break;
    case core::AdjustmentKind::HueSaturation:
      if (m_hueSlider) {
        m_hueSlider->setValue(static_cast<int>(p.hue));
        m_saturationSlider->setValue(static_cast<int>(p.saturation * 100.0f));
        m_lightnessSlider->setValue(static_cast<int>(p.lightness * 100.0f));
      }
      break;
    case core::AdjustmentKind::Levels:
      if (m_inputBlackSlider) {
        m_inputBlackSlider->setValue(static_cast<int>(p.inputBlack * 255.0f));
        m_inputWhiteSlider->setValue(static_cast<int>(p.inputWhite * 255.0f));
        m_gammaSlider->setValue(static_cast<int>(p.gamma * 100.0f));
        m_outputBlackSlider->setValue(static_cast<int>(p.outputBlack * 255.0f));
        m_outputWhiteSlider->setValue(static_cast<int>(p.outputWhite * 255.0f));
      }
      break;
    case core::AdjustmentKind::Vibrance:
      if (m_vibranceSlider) {
        m_vibranceSlider->setValue(static_cast<int>(p.vibrance * 100.0f));
      }
      break;
    default:
      break;
  }

  m_refreshing = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// apply params helper — reads all controls and pushes to controller
// ─────────────────────────────────────────────────────────────────────────────

void AdjustmentPropertyPanel::applyParams() {
  if (m_refreshing || !m_controller) return;

  const core::Document& doc = m_controller->document();
  if (doc.layerCount() == 0) return;
  const core::Layer* layer = doc.activeLayer();
  if (!layer || !layer->isAdjustment()) return;

  core::AdjustmentParams p = layer->adjustmentParams();

  switch (p.kind) {
    case core::AdjustmentKind::BrightnessContrast:
      p.brightness = m_brightnessSlider ? static_cast<float>(m_brightnessSlider->value()) / 100.0f : p.brightness;
      p.contrast   = m_contrastSlider   ? static_cast<float>(m_contrastSlider->value())   / 100.0f : p.contrast;
      break;
    case core::AdjustmentKind::HueSaturation:
      p.hue        = m_hueSlider        ? static_cast<float>(m_hueSlider->value())               : p.hue;
      p.saturation = m_saturationSlider ? static_cast<float>(m_saturationSlider->value()) / 100.0f : p.saturation;
      p.lightness  = m_lightnessSlider  ? static_cast<float>(m_lightnessSlider->value())  / 100.0f : p.lightness;
      break;
    case core::AdjustmentKind::Levels:
      p.inputBlack  = m_inputBlackSlider  ? static_cast<float>(m_inputBlackSlider->value())  / 255.0f : p.inputBlack;
      p.inputWhite  = m_inputWhiteSlider  ? static_cast<float>(m_inputWhiteSlider->value())  / 255.0f : p.inputWhite;
      p.gamma       = m_gammaSlider       ? static_cast<float>(m_gammaSlider->value())       / 100.0f : p.gamma;
      p.outputBlack = m_outputBlackSlider ? static_cast<float>(m_outputBlackSlider->value()) / 255.0f : p.outputBlack;
      p.outputWhite = m_outputWhiteSlider ? static_cast<float>(m_outputWhiteSlider->value()) / 255.0f : p.outputWhite;
      break;
    case core::AdjustmentKind::Vibrance:
      p.vibrance = m_vibranceSlider ? static_cast<float>(m_vibranceSlider->value()) / 100.0f : p.vibrance;
      break;
    default:
      break;
  }

  m_controller->setActiveLayerAdjustmentParams(p);
}

// ─────────────────────────────────────────────────────────────────────────────
// slot implementations (all delegate to applyParams)
// ─────────────────────────────────────────────────────────────────────────────

void AdjustmentPropertyPanel::onBrightnessChanged(int)  { applyParams(); }
void AdjustmentPropertyPanel::onContrastChanged(int)    { applyParams(); }
void AdjustmentPropertyPanel::onHueChanged(int)         { applyParams(); }
void AdjustmentPropertyPanel::onSaturationChanged(int)  { applyParams(); }
void AdjustmentPropertyPanel::onLightnessChanged(int)   { applyParams(); }
void AdjustmentPropertyPanel::onInputBlackChanged(int)  { applyParams(); }
void AdjustmentPropertyPanel::onInputWhiteChanged(int)  { applyParams(); }
void AdjustmentPropertyPanel::onGammaChanged(int)       { applyParams(); }
void AdjustmentPropertyPanel::onOutputBlackChanged(int) { applyParams(); }
void AdjustmentPropertyPanel::onOutputWhiteChanged(int) { applyParams(); }
void AdjustmentPropertyPanel::onVibranceChanged(int)    { applyParams(); }

} // namespace app::panels
