#include "app/panels/ToolPropertyPanel.h"

#include <cstdint>

#include <QColorDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QVariant>
#include <QVBoxLayout>

#include "app/bridge/AppController.h"

namespace app::panels {

namespace {

core::Color toCoreColor(const QColor& color) {
  return core::Color {
      static_cast<std::uint8_t>(color.red()),
      static_cast<std::uint8_t>(color.green()),
      static_cast<std::uint8_t>(color.blue()),
      static_cast<std::uint8_t>(color.alpha())};
}

QColor toQColor(const core::Color& color) {
  return QColor(color.r, color.g, color.b, color.a);
}

} // namespace

ToolPropertyPanel::ToolPropertyPanel(QWidget* parent)
    : QWidget(parent),
      m_scrollArea(new QScrollArea(this)),
      m_contentWidget(new QWidget(this)),
      m_brushDynamicsSection(nullptr),
      m_correctionSection(nullptr),
      m_shapeSection(nullptr),
      m_drawingControlSection(nullptr),
      m_toolNameLabel(new QLabel("Tool: -", this)),
      m_guideLabel(new QLabel("", this)),
      m_colorLabel(new QLabel("Color", this)),
      m_sizeLabel(new QLabel("Size", this)),
      m_opacityLabel(new QLabel("Opacity", this)),
      m_hardnessLabel(new QLabel("Hardness", this)),
      m_flowLabel(new QLabel("Flow", this)),
      m_spacingLabel(new QLabel("Spacing", this)),
      m_stabilizationLabel(new QLabel("Stabilization", this)),
      m_colorButton(new QPushButton("Color", this)),
      m_sizeSpin(new QSpinBox(this)),
      m_opacitySlider(new QSlider(Qt::Horizontal, this)),
      m_opacitySpin(new QSpinBox(this)),
      m_hardnessSlider(new QSlider(Qt::Horizontal, this)),
      m_hardnessSpin(new QSpinBox(this)),
      m_flowSlider(new QSlider(Qt::Horizontal, this)),
      m_flowSpin(new QSpinBox(this)),
      m_spacingSlider(new QSlider(Qt::Horizontal, this)),
      m_spacingSpin(new QSpinBox(this)),
      m_antiAliasCheck(new QCheckBox("Anti Alias", this)),
      m_stabilizationSlider(new QSlider(Qt::Horizontal, this)),
      m_stabilizationSpin(new QSpinBox(this)),
      m_postCorrectionCheck(new QCheckBox("Post Correction", this)),
      m_velocityCorrectionCheck(new QCheckBox("Velocity Correction", this)),
      m_shapeTypeCombo(new QComboBox(this)),
      m_blendModeCombo(new QComboBox(this)),
      m_eraseModeCheck(new QCheckBox("Erase Mode", this)),
      m_lockAlphaRespectCheck(new QCheckBox("Lock Alpha", this)) {
  auto* hostLayout = new QVBoxLayout(this);
  hostLayout->setContentsMargins(0, 0, 0, 0);
  hostLayout->setSpacing(0);

  m_guideLabel->setWordWrap(true);
  m_sizeSpin->setRange(1, 128);
  m_opacitySlider->setRange(0, 100);
  m_opacitySpin->setRange(0, 100);
  m_hardnessSlider->setRange(0, 100);
  m_hardnessSpin->setRange(0, 100);
  m_flowSlider->setRange(0, 100);
  m_flowSpin->setRange(0, 100);
  m_spacingSlider->setRange(1, 300);
  m_spacingSpin->setRange(1, 300);
  m_stabilizationSlider->setRange(0, 100);
  m_stabilizationSpin->setRange(0, 100);

  m_shapeTypeCombo->addItem("Circle", static_cast<int>(core::BrushShapeType::Circle));
  m_shapeTypeCombo->addItem("Square", static_cast<int>(core::BrushShapeType::Square));

  m_blendModeCombo->addItem("Normal", static_cast<int>(core::BlendMode::Normal));
  m_blendModeCombo->addItem("Multiply", static_cast<int>(core::BlendMode::Multiply));
  m_blendModeCombo->addItem("Add", static_cast<int>(core::BlendMode::Add));

  auto* contentLayout = new QVBoxLayout(m_contentWidget);
  contentLayout->setContentsMargins(6, 6, 6, 6);
  contentLayout->setSpacing(8);

  auto* titleFrame = new QFrame(m_contentWidget);
  auto* titleLayout = new QVBoxLayout(titleFrame);
  titleLayout->setContentsMargins(8, 8, 8, 8);
  titleLayout->setSpacing(4);
  titleLayout->addWidget(m_toolNameLabel);
  titleLayout->addWidget(m_guideLabel);
  contentLayout->addWidget(titleFrame);

  auto* basicGroup = new QGroupBox("Basic", m_contentWidget);
  auto* basicLayout = new QVBoxLayout(basicGroup);
  basicLayout->setContentsMargins(8, 8, 8, 8);
  basicLayout->setSpacing(6);

  auto* opacityRow = new QHBoxLayout();
  opacityRow->setContentsMargins(0, 0, 0, 0);
  opacityRow->setSpacing(6);
  opacityRow->addWidget(m_opacitySlider, 1);
  opacityRow->addWidget(m_opacitySpin);

  auto* hardnessRow = new QHBoxLayout();
  hardnessRow->setContentsMargins(0, 0, 0, 0);
  hardnessRow->setSpacing(6);
  hardnessRow->addWidget(m_hardnessSlider, 1);
  hardnessRow->addWidget(m_hardnessSpin);

  basicLayout->addWidget(m_colorLabel);
  basicLayout->addWidget(m_colorButton);
  basicLayout->addWidget(m_sizeLabel);
  basicLayout->addWidget(m_sizeSpin);
  basicLayout->addWidget(m_opacityLabel);
  basicLayout->addLayout(opacityRow);
  basicLayout->addWidget(m_hardnessLabel);
  basicLayout->addLayout(hardnessRow);
  contentLayout->addWidget(basicGroup);

  auto* dynamicsGroup = new QGroupBox("Brush Dynamics", m_contentWidget);
  auto* dynamicsLayout = new QVBoxLayout(dynamicsGroup);
  dynamicsLayout->setContentsMargins(8, 8, 8, 8);
  dynamicsLayout->setSpacing(6);

  auto* flowRow = new QHBoxLayout();
  flowRow->setContentsMargins(0, 0, 0, 0);
  flowRow->setSpacing(6);
  flowRow->addWidget(m_flowSlider, 1);
  flowRow->addWidget(m_flowSpin);

  auto* spacingRow = new QHBoxLayout();
  spacingRow->setContentsMargins(0, 0, 0, 0);
  spacingRow->setSpacing(6);
  spacingRow->addWidget(m_spacingSlider, 1);
  spacingRow->addWidget(m_spacingSpin);

  dynamicsLayout->addWidget(m_flowLabel);
  dynamicsLayout->addLayout(flowRow);
  dynamicsLayout->addWidget(m_spacingLabel);
  dynamicsLayout->addLayout(spacingRow);
  contentLayout->addWidget(dynamicsGroup);
  m_brushDynamicsSection = dynamicsGroup;

  auto* correctionGroup = new QGroupBox("Correction", m_contentWidget);
  auto* correctionLayout = new QVBoxLayout(correctionGroup);
  correctionLayout->setContentsMargins(8, 8, 8, 8);
  correctionLayout->setSpacing(6);

  auto* stabilizationRow = new QHBoxLayout();
  stabilizationRow->setContentsMargins(0, 0, 0, 0);
  stabilizationRow->setSpacing(6);
  stabilizationRow->addWidget(m_stabilizationSlider, 1);
  stabilizationRow->addWidget(m_stabilizationSpin);

  correctionLayout->addWidget(m_antiAliasCheck);
  correctionLayout->addWidget(m_stabilizationLabel);
  correctionLayout->addLayout(stabilizationRow);
  correctionLayout->addWidget(m_postCorrectionCheck);
  correctionLayout->addWidget(m_velocityCorrectionCheck);
  contentLayout->addWidget(correctionGroup);
  m_correctionSection = correctionGroup;

  auto* shapeGroup = new QGroupBox("Shape", m_contentWidget);
  auto* shapeLayout = new QVBoxLayout(shapeGroup);
  shapeLayout->setContentsMargins(8, 8, 8, 8);
  shapeLayout->setSpacing(6);
  shapeLayout->addWidget(m_shapeTypeCombo);
  contentLayout->addWidget(shapeGroup);
  m_shapeSection = shapeGroup;

  auto* drawControlGroup = new QGroupBox("Drawing Control", m_contentWidget);
  auto* drawControlLayout = new QVBoxLayout(drawControlGroup);
  drawControlLayout->setContentsMargins(8, 8, 8, 8);
  drawControlLayout->setSpacing(6);
  drawControlLayout->addWidget(m_blendModeCombo);
  drawControlLayout->addWidget(m_eraseModeCheck);
  drawControlLayout->addWidget(m_lockAlphaRespectCheck);
  contentLayout->addWidget(drawControlGroup);
  m_drawingControlSection = drawControlGroup;

  contentLayout->addStretch(1);

  m_scrollArea->setWidgetResizable(true);
  m_scrollArea->setFrameShape(QFrame::NoFrame);
  m_scrollArea->setWidget(m_contentWidget);
  hostLayout->addWidget(m_scrollArea);

  connect(m_colorButton, &QPushButton::clicked, this, &ToolPropertyPanel::onChooseColor);
  connect(m_sizeSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onSizeChanged);
  connect(m_opacitySlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onOpacitySliderChanged);
  connect(m_opacitySpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onOpacitySpinChanged);
  connect(m_hardnessSlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onHardnessSliderChanged);
  connect(m_hardnessSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onHardnessSpinChanged);
  connect(m_flowSlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onFlowSliderChanged);
  connect(m_flowSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onFlowSpinChanged);
  connect(m_spacingSlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onSpacingSliderChanged);
  connect(m_spacingSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onSpacingSpinChanged);
  connect(m_antiAliasCheck, &QCheckBox::toggled, this, &ToolPropertyPanel::onAntiAliasToggled);
  connect(m_stabilizationSlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onStabilizationSliderChanged);
  connect(m_stabilizationSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onStabilizationSpinChanged);
  connect(m_postCorrectionCheck, &QCheckBox::toggled, this, &ToolPropertyPanel::onPostCorrectionToggled);
  connect(m_velocityCorrectionCheck, &QCheckBox::toggled, this, &ToolPropertyPanel::onVelocityCorrectionToggled);
  connect(m_shapeTypeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &ToolPropertyPanel::onShapeTypeChanged);
  connect(m_blendModeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &ToolPropertyPanel::onBlendModeChanged);
  connect(m_eraseModeCheck, &QCheckBox::toggled, this, &ToolPropertyPanel::onEraseModeToggled);
  connect(m_lockAlphaRespectCheck, &QCheckBox::toggled, this, &ToolPropertyPanel::onLockAlphaRespectToggled);
}

void ToolPropertyPanel::setController(app::bridge::AppController* controller) {
  if (m_controller != nullptr) {
    disconnect(m_controller, nullptr, this, nullptr);
  }

  m_controller = controller;
  if (m_controller == nullptr) {
    return;
  }

  connect(m_controller, &app::bridge::AppController::toolStateChanged, this, &ToolPropertyPanel::refreshFromController);
  refreshFromController();
}

void ToolPropertyPanel::refreshFromController() {
  if (m_controller == nullptr) {
    return;
  }

  m_toolNameLabel->setText(QString("Tool: %1").arg(QString::fromStdString(m_controller->currentToolDisplayName())));
  m_guideLabel->setText(QString::fromStdString(m_controller->currentToolGuide()));

  const bool supportsColor = m_controller->currentToolSupportsColor();
  const bool supportsSize = m_controller->currentToolSupportsSize();
  const bool supportsOpacity = m_controller->currentToolSupportsOpacity();
  const bool supportsHardness = m_controller->currentToolSupportsHardness();
  const bool supportsFlow = m_controller->currentToolSupportsFlow();
  const bool supportsSpacing = m_controller->currentToolSupportsSpacing();
  const bool supportsAntiAlias = m_controller->currentToolSupportsAntiAlias();
  const bool supportsStabilization = m_controller->currentToolSupportsStabilization();
  const bool supportsPostCorrection = m_controller->currentToolSupportsPostCorrection();
  const bool supportsVelocityCorrection = m_controller->currentToolSupportsVelocityCorrection();
  const bool supportsShape = m_controller->currentToolSupportsShapeType();
  const bool supportsBlend = m_controller->currentToolSupportsBlendMode();
  const bool supportsEraseMode = m_controller->currentToolSupportsEraseMode();
  const bool supportsLockAlpha = m_controller->currentToolSupportsLockAlphaRespect();
  m_colorLabel->setVisible(supportsColor);
  m_colorButton->setVisible(supportsColor);
  m_sizeLabel->setVisible(supportsSize);
  m_sizeSpin->setVisible(supportsSize);
  m_opacityLabel->setVisible(supportsOpacity);
  m_opacitySlider->setVisible(supportsOpacity);
  m_opacitySpin->setVisible(supportsOpacity);
  m_hardnessLabel->setVisible(supportsHardness);
  m_hardnessSlider->setVisible(supportsHardness);
  m_hardnessSpin->setVisible(supportsHardness);
  m_flowLabel->setVisible(supportsFlow);
  m_flowSlider->setVisible(supportsFlow);
  m_flowSpin->setVisible(supportsFlow);
  m_spacingLabel->setVisible(supportsSpacing);
  m_spacingSlider->setVisible(supportsSpacing);
  m_spacingSpin->setVisible(supportsSpacing);
  m_brushDynamicsSection->setVisible(supportsFlow || supportsSpacing);
  m_antiAliasCheck->setVisible(supportsAntiAlias);
  m_stabilizationLabel->setVisible(supportsStabilization);
  m_stabilizationSlider->setVisible(supportsStabilization);
  m_stabilizationSpin->setVisible(supportsStabilization);
  m_postCorrectionCheck->setVisible(supportsPostCorrection);
  m_velocityCorrectionCheck->setVisible(supportsVelocityCorrection);
  m_correctionSection->setVisible(supportsAntiAlias || supportsStabilization || supportsPostCorrection || supportsVelocityCorrection);
  m_shapeTypeCombo->setVisible(supportsShape);
  m_shapeSection->setVisible(supportsShape);
  m_blendModeCombo->setVisible(supportsBlend);
  m_eraseModeCheck->setVisible(supportsEraseMode);
  m_lockAlphaRespectCheck->setVisible(supportsLockAlpha);
  m_drawingControlSection->setVisible(supportsBlend || supportsEraseMode || supportsLockAlpha);

  const app::bridge::ToolStateViewModel state = m_controller->toolState();
  const QSignalBlocker blocker1(m_sizeSpin);
  const QSignalBlocker blocker2(m_opacitySlider);
  const QSignalBlocker blocker3(m_opacitySpin);
  const QSignalBlocker blocker4(m_hardnessSlider);
  const QSignalBlocker blocker5(m_hardnessSpin);
  const QSignalBlocker blocker6(m_flowSlider);
  const QSignalBlocker blocker7(m_flowSpin);
  const QSignalBlocker blocker8(m_spacingSlider);
  const QSignalBlocker blocker9(m_spacingSpin);
  const QSignalBlocker blocker10(m_antiAliasCheck);
  const QSignalBlocker blocker11(m_stabilizationSlider);
  const QSignalBlocker blocker12(m_stabilizationSpin);
  const QSignalBlocker blocker13(m_postCorrectionCheck);
  const QSignalBlocker blocker14(m_velocityCorrectionCheck);
  const QSignalBlocker blocker15(m_shapeTypeCombo);
  const QSignalBlocker blocker16(m_blendModeCombo);
  const QSignalBlocker blocker17(m_eraseModeCheck);
  const QSignalBlocker blocker18(m_lockAlphaRespectCheck);
  m_sizeSpin->setValue(state.size);
  m_opacitySlider->setValue(state.opacity);
  m_opacitySpin->setValue(state.opacity);
  m_hardnessSlider->setValue(state.hardness);
  m_hardnessSpin->setValue(state.hardness);
  m_flowSlider->setValue(state.flow);
  m_flowSpin->setValue(state.flow);
  m_spacingSlider->setValue(state.spacing);
  m_spacingSpin->setValue(state.spacing);
  m_antiAliasCheck->setChecked(state.antiAlias);
  m_stabilizationSlider->setValue(state.stabilization);
  m_stabilizationSpin->setValue(state.stabilization);
  m_postCorrectionCheck->setChecked(state.postCorrection);
  m_velocityCorrectionCheck->setChecked(state.velocityBasedCorrection);
  m_shapeTypeCombo->setCurrentIndex(m_shapeTypeCombo->findData(static_cast<int>(state.shapeType)));
  m_blendModeCombo->setCurrentIndex(m_blendModeCombo->findData(static_cast<int>(state.blendMode)));
  m_eraseModeCheck->setChecked(state.eraseMode);
  m_lockAlphaRespectCheck->setChecked(state.lockAlphaRespect);
  updateColorButton();
}

void ToolPropertyPanel::onChooseColor() {
  if (m_controller == nullptr || !m_controller->currentToolSupportsColor()) {
    return;
  }
  const app::bridge::ToolStateViewModel state = m_controller->toolState();
  const QColor picked = QColorDialog::getColor(toQColor(state.color), this, "Tool Color", QColorDialog::ShowAlphaChannel);
  if (!picked.isValid()) {
    return;
  }
  m_controller->setBrushColor(toCoreColor(picked));
}

void ToolPropertyPanel::onSizeChanged(int size) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsSize()) {
    return;
  }
  m_controller->setBrushSize(size);
}

void ToolPropertyPanel::onOpacitySliderChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsOpacity()) {
    return;
  }
  const QSignalBlocker blocker(m_opacitySpin);
  m_opacitySpin->setValue(value);
  m_controller->setBrushOpacity(value);
}

void ToolPropertyPanel::onOpacitySpinChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsOpacity()) {
    return;
  }
  const QSignalBlocker blocker(m_opacitySlider);
  m_opacitySlider->setValue(value);
  m_controller->setBrushOpacity(value);
}

void ToolPropertyPanel::onHardnessSliderChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsHardness()) {
    return;
  }
  const QSignalBlocker blocker(m_hardnessSpin);
  m_hardnessSpin->setValue(value);
  m_controller->setBrushHardness(value);
}

void ToolPropertyPanel::onHardnessSpinChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsHardness()) {
    return;
  }
  const QSignalBlocker blocker(m_hardnessSlider);
  m_hardnessSlider->setValue(value);
  m_controller->setBrushHardness(value);
}

void ToolPropertyPanel::onFlowSliderChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsFlow()) {
    return;
  }
  const QSignalBlocker blocker(m_flowSpin);
  m_flowSpin->setValue(value);
  m_controller->setBrushFlow(value);
}

void ToolPropertyPanel::onFlowSpinChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsFlow()) {
    return;
  }
  const QSignalBlocker blocker(m_flowSlider);
  m_flowSlider->setValue(value);
  m_controller->setBrushFlow(value);
}

void ToolPropertyPanel::onSpacingSliderChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsSpacing()) {
    return;
  }
  const QSignalBlocker blocker(m_spacingSpin);
  m_spacingSpin->setValue(value);
  m_controller->setBrushSpacing(value);
}

void ToolPropertyPanel::onSpacingSpinChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsSpacing()) {
    return;
  }
  const QSignalBlocker blocker(m_spacingSlider);
  m_spacingSlider->setValue(value);
  m_controller->setBrushSpacing(value);
}

void ToolPropertyPanel::onAntiAliasToggled(bool checked) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsAntiAlias()) {
    return;
  }
  m_controller->setBrushAntiAlias(checked);
}

void ToolPropertyPanel::onStabilizationSliderChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsStabilization()) {
    return;
  }
  const QSignalBlocker blocker(m_stabilizationSpin);
  m_stabilizationSpin->setValue(value);
  m_controller->setBrushStabilization(value);
}

void ToolPropertyPanel::onStabilizationSpinChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsStabilization()) {
    return;
  }
  const QSignalBlocker blocker(m_stabilizationSlider);
  m_stabilizationSlider->setValue(value);
  m_controller->setBrushStabilization(value);
}

void ToolPropertyPanel::onPostCorrectionToggled(bool checked) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsPostCorrection()) {
    return;
  }
  m_controller->setBrushPostCorrection(checked);
}

void ToolPropertyPanel::onVelocityCorrectionToggled(bool checked) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsVelocityCorrection()) {
    return;
  }
  m_controller->setBrushVelocityBasedCorrection(checked);
}

void ToolPropertyPanel::onShapeTypeChanged(int index) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsShapeType()) {
    return;
  }
  const QVariant value = m_shapeTypeCombo->itemData(index);
  if (!value.isValid()) {
    return;
  }
  m_controller->setBrushShapeType(static_cast<core::BrushShapeType>(value.toInt()));
}

void ToolPropertyPanel::onBlendModeChanged(int index) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsBlendMode()) {
    return;
  }
  const QVariant value = m_blendModeCombo->itemData(index);
  if (!value.isValid()) {
    return;
  }
  m_controller->setBrushBlendMode(static_cast<core::BlendMode>(value.toInt()));
}

void ToolPropertyPanel::onEraseModeToggled(bool checked) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsEraseMode()) {
    return;
  }
  m_controller->setBrushEraseMode(checked);
}

void ToolPropertyPanel::onLockAlphaRespectToggled(bool checked) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsLockAlphaRespect()) {
    return;
  }
  m_controller->setBrushLockAlphaRespect(checked);
}

void ToolPropertyPanel::updateColorButton() {
  if (m_controller == nullptr) {
    return;
  }
  const app::bridge::ToolStateViewModel state = m_controller->toolState();
  const QColor color = toQColor(state.color);
  const int luminance = (299 * color.red() + 587 * color.green() + 114 * color.blue()) / 1000;
  const QString textColor = luminance > 128 ? "#111111" : "#f5f5f5";
  const QString hex = color.name(QColor::HexRgb).toUpper();

  m_colorButton->setText(QString("Color %1").arg(hex));
  m_colorButton->setStyleSheet(
      QString("QPushButton { background-color: rgba(%1, %2, %3, %4); color: %5; border: 1px solid #555; padding: 2px 4px; }")
          .arg(color.red())
          .arg(color.green())
          .arg(color.blue())
          .arg(color.alpha())
          .arg(textColor));
}

} // namespace app::panels
