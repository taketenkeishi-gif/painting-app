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
      m_vectorSection(nullptr),
      m_fillSection(nullptr),
      m_selectionSection(nullptr),
      m_toolNameLabel(new QLabel("Tool: -", this)),
      m_guideLabel(new QLabel("", this)),
      m_compatibilityLabel(new QLabel("", this)),
      m_colorLabel(new QLabel("Color", this)),
      m_sizeLabel(new QLabel("Size", this)),
      m_opacityLabel(new QLabel("Opacity", this)),
      m_hardnessLabel(new QLabel("Hardness", this)),
      m_flowLabel(new QLabel("Flow", this)),
      m_spacingLabel(new QLabel("Spacing", this)),
      m_stabilizationLabel(new QLabel("Stabilization", this)),
      m_angleLabel(new QLabel("Angle", this)),
      m_roundnessLabel(new QLabel("Roundness", this)),
      m_taperStartLabel(new QLabel("Taper Start", this)),
      m_taperEndLabel(new QLabel("Taper End", this)),
      m_snapAngleLabel(new QLabel("Snap Angle", this)),
      m_simplifyLabel(new QLabel("Simplify", this)),
      m_fillThresholdLabel(new QLabel("Threshold", this)),
      m_fillGapCloseLabel(new QLabel("Gap Close", this)),
      m_selectionModeLabel(new QLabel("Mode", this)),
      m_autoSelectThresholdLabel(new QLabel("Auto Threshold", this)),
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
      m_angleSlider(new QSlider(Qt::Horizontal, this)),
      m_angleSpin(new QSpinBox(this)),
      m_roundnessSlider(new QSlider(Qt::Horizontal, this)),
      m_roundnessSpin(new QSpinBox(this)),
      m_taperStartSlider(new QSlider(Qt::Horizontal, this)),
      m_taperStartSpin(new QSpinBox(this)),
      m_taperEndSlider(new QSlider(Qt::Horizontal, this)),
      m_taperEndSpin(new QSpinBox(this)),
      m_snapAngleSlider(new QSlider(Qt::Horizontal, this)),
      m_snapAngleSpin(new QSpinBox(this)),
      m_simplifySlider(new QSlider(Qt::Horizontal, this)),
      m_simplifySpin(new QSpinBox(this)),
      m_fillThresholdSlider(new QSlider(Qt::Horizontal, this)),
      m_fillThresholdSpin(new QSpinBox(this)),
      m_fillContiguousCheck(new QCheckBox("Contiguous", this)),
      m_fillReferAllLayersCheck(new QCheckBox("Refer All Layers", this)),
      m_fillGapCloseSlider(new QSlider(Qt::Horizontal, this)),
      m_fillGapCloseSpin(new QSpinBox(this)),
      m_selectionModeCombo(new QComboBox(this)),
      m_autoSelectThresholdSlider(new QSlider(Qt::Horizontal, this)),
      m_autoSelectThresholdSpin(new QSpinBox(this)),
      m_autoSelectContiguousCheck(new QCheckBox("Auto Contiguous", this)),
      m_autoSelectReferAllLayersCheck(new QCheckBox("Auto Refer All Layers", this)),
      m_blendModeCombo(new QComboBox(this)),
      m_eraseModeCheck(new QCheckBox("Erase Mode", this)),
      m_lockAlphaRespectCheck(new QCheckBox("Lock Alpha", this)) {
  auto* hostLayout = new QVBoxLayout(this);
  hostLayout->setContentsMargins(0, 0, 0, 0);
  hostLayout->setSpacing(0);

  m_guideLabel->setWordWrap(true);
  m_compatibilityLabel->setWordWrap(true);
  m_compatibilityLabel->setStyleSheet("color: #f3bf58; font-weight: 600;");
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
  m_angleSlider->setRange(-180, 180);
  m_angleSpin->setRange(-180, 180);
  m_roundnessSlider->setRange(0, 100);
  m_roundnessSpin->setRange(0, 100);
  m_taperStartSlider->setRange(0, 100);
  m_taperStartSpin->setRange(0, 100);
  m_taperEndSlider->setRange(0, 100);
  m_taperEndSpin->setRange(0, 100);
  m_snapAngleSlider->setRange(0, 180);
  m_snapAngleSpin->setRange(0, 180);
  m_simplifySlider->setRange(0, 100);
  m_simplifySpin->setRange(0, 100);
  m_fillThresholdSlider->setRange(0, 255);
  m_fillThresholdSpin->setRange(0, 255);
  m_fillGapCloseSlider->setRange(0, 8);
  m_fillGapCloseSpin->setRange(0, 8);
  m_autoSelectThresholdSlider->setRange(0, 255);
  m_autoSelectThresholdSpin->setRange(0, 255);

  m_shapeTypeCombo->addItem("Circle", static_cast<int>(core::BrushShapeType::Circle));
  m_shapeTypeCombo->addItem("Square", static_cast<int>(core::BrushShapeType::Square));

  m_blendModeCombo->addItem("Normal", static_cast<int>(core::BlendMode::Normal));
  m_blendModeCombo->addItem("Multiply", static_cast<int>(core::BlendMode::Multiply));
  m_blendModeCombo->addItem("Add", static_cast<int>(core::BlendMode::Add));

  m_selectionModeCombo->addItem("Rectangle", static_cast<int>(app::ui::SelectionMode::Rectangle));
  m_selectionModeCombo->addItem("Lasso", static_cast<int>(app::ui::SelectionMode::Lasso));
  m_selectionModeCombo->addItem("Auto Select", static_cast<int>(app::ui::SelectionMode::AutoSelect));

  auto* contentLayout = new QVBoxLayout(m_contentWidget);
  contentLayout->setContentsMargins(6, 6, 6, 6);
  contentLayout->setSpacing(8);

  auto* titleFrame = new QFrame(m_contentWidget);
  auto* titleLayout = new QVBoxLayout(titleFrame);
  titleLayout->setContentsMargins(8, 8, 8, 8);
  titleLayout->setSpacing(4);
  titleLayout->addWidget(m_toolNameLabel);
  titleLayout->addWidget(m_compatibilityLabel);
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
  auto* angleRow = new QHBoxLayout();
  angleRow->setContentsMargins(0, 0, 0, 0);
  angleRow->setSpacing(6);
  angleRow->addWidget(m_angleSlider, 1);
  angleRow->addWidget(m_angleSpin);
  auto* roundnessRow = new QHBoxLayout();
  roundnessRow->setContentsMargins(0, 0, 0, 0);
  roundnessRow->setSpacing(6);
  roundnessRow->addWidget(m_roundnessSlider, 1);
  roundnessRow->addWidget(m_roundnessSpin);
  auto* taperStartRow = new QHBoxLayout();
  taperStartRow->setContentsMargins(0, 0, 0, 0);
  taperStartRow->setSpacing(6);
  taperStartRow->addWidget(m_taperStartSlider, 1);
  taperStartRow->addWidget(m_taperStartSpin);
  auto* taperEndRow = new QHBoxLayout();
  taperEndRow->setContentsMargins(0, 0, 0, 0);
  taperEndRow->setSpacing(6);
  taperEndRow->addWidget(m_taperEndSlider, 1);
  taperEndRow->addWidget(m_taperEndSpin);
  shapeLayout->addWidget(m_shapeTypeCombo);
  shapeLayout->addWidget(m_angleLabel);
  shapeLayout->addLayout(angleRow);
  shapeLayout->addWidget(m_roundnessLabel);
  shapeLayout->addLayout(roundnessRow);
  shapeLayout->addWidget(m_taperStartLabel);
  shapeLayout->addLayout(taperStartRow);
  shapeLayout->addWidget(m_taperEndLabel);
  shapeLayout->addLayout(taperEndRow);
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

  auto* vectorGroup = new QGroupBox("Vector", m_contentWidget);
  auto* vectorLayout = new QVBoxLayout(vectorGroup);
  vectorLayout->setContentsMargins(8, 8, 8, 8);
  vectorLayout->setSpacing(6);
  auto* snapAngleRow = new QHBoxLayout();
  snapAngleRow->setContentsMargins(0, 0, 0, 0);
  snapAngleRow->setSpacing(6);
  snapAngleRow->addWidget(m_snapAngleSlider, 1);
  snapAngleRow->addWidget(m_snapAngleSpin);
  auto* simplifyRow = new QHBoxLayout();
  simplifyRow->setContentsMargins(0, 0, 0, 0);
  simplifyRow->setSpacing(6);
  simplifyRow->addWidget(m_simplifySlider, 1);
  simplifyRow->addWidget(m_simplifySpin);
  vectorLayout->addWidget(m_snapAngleLabel);
  vectorLayout->addLayout(snapAngleRow);
  vectorLayout->addWidget(m_simplifyLabel);
  vectorLayout->addLayout(simplifyRow);
  contentLayout->addWidget(vectorGroup);
  m_vectorSection = vectorGroup;

  auto* fillGroup = new QGroupBox("Fill", m_contentWidget);
  auto* fillLayout = new QVBoxLayout(fillGroup);
  fillLayout->setContentsMargins(8, 8, 8, 8);
  fillLayout->setSpacing(6);
  auto* fillThresholdRow = new QHBoxLayout();
  fillThresholdRow->setContentsMargins(0, 0, 0, 0);
  fillThresholdRow->setSpacing(6);
  fillThresholdRow->addWidget(m_fillThresholdSlider, 1);
  fillThresholdRow->addWidget(m_fillThresholdSpin);
  auto* fillGapCloseRow = new QHBoxLayout();
  fillGapCloseRow->setContentsMargins(0, 0, 0, 0);
  fillGapCloseRow->setSpacing(6);
  fillGapCloseRow->addWidget(m_fillGapCloseSlider, 1);
  fillGapCloseRow->addWidget(m_fillGapCloseSpin);
  fillLayout->addWidget(m_fillThresholdLabel);
  fillLayout->addLayout(fillThresholdRow);
  fillLayout->addWidget(m_fillContiguousCheck);
  fillLayout->addWidget(m_fillReferAllLayersCheck);
  fillLayout->addWidget(m_fillGapCloseLabel);
  fillLayout->addLayout(fillGapCloseRow);
  contentLayout->addWidget(fillGroup);
  m_fillSection = fillGroup;

  auto* selectionGroup = new QGroupBox("Selection", m_contentWidget);
  auto* selectionLayout = new QVBoxLayout(selectionGroup);
  selectionLayout->setContentsMargins(8, 8, 8, 8);
  selectionLayout->setSpacing(6);
  auto* autoSelectThresholdRow = new QHBoxLayout();
  autoSelectThresholdRow->setContentsMargins(0, 0, 0, 0);
  autoSelectThresholdRow->setSpacing(6);
  autoSelectThresholdRow->addWidget(m_autoSelectThresholdSlider, 1);
  autoSelectThresholdRow->addWidget(m_autoSelectThresholdSpin);
  selectionLayout->addWidget(m_selectionModeLabel);
  selectionLayout->addWidget(m_selectionModeCombo);
  selectionLayout->addWidget(m_autoSelectThresholdLabel);
  selectionLayout->addLayout(autoSelectThresholdRow);
  selectionLayout->addWidget(m_autoSelectContiguousCheck);
  selectionLayout->addWidget(m_autoSelectReferAllLayersCheck);
  contentLayout->addWidget(selectionGroup);
  m_selectionSection = selectionGroup;

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
  connect(m_angleSlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onAngleSliderChanged);
  connect(m_angleSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onAngleSpinChanged);
  connect(m_roundnessSlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onRoundnessSliderChanged);
  connect(m_roundnessSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onRoundnessSpinChanged);
  connect(m_taperStartSlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onTaperStartSliderChanged);
  connect(m_taperStartSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onTaperStartSpinChanged);
  connect(m_taperEndSlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onTaperEndSliderChanged);
  connect(m_taperEndSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onTaperEndSpinChanged);
  connect(m_snapAngleSlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onSnapAngleSliderChanged);
  connect(m_snapAngleSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onSnapAngleSpinChanged);
  connect(m_simplifySlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onSimplifySliderChanged);
  connect(m_simplifySpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onSimplifySpinChanged);
  connect(m_fillThresholdSlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onFillThresholdSliderChanged);
  connect(m_fillThresholdSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onFillThresholdSpinChanged);
  connect(m_fillContiguousCheck, &QCheckBox::toggled, this, &ToolPropertyPanel::onFillContiguousToggled);
  connect(m_fillReferAllLayersCheck, &QCheckBox::toggled, this, &ToolPropertyPanel::onFillReferAllLayersToggled);
  connect(m_fillGapCloseSlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onFillGapCloseSliderChanged);
  connect(m_fillGapCloseSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onFillGapCloseSpinChanged);
  connect(m_selectionModeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &ToolPropertyPanel::onSelectionModeChanged);
  connect(
      m_autoSelectThresholdSlider,
      &QSlider::valueChanged,
      this,
      &ToolPropertyPanel::onAutoSelectThresholdSliderChanged);
  connect(
      m_autoSelectThresholdSpin,
      qOverload<int>(&QSpinBox::valueChanged),
      this,
      &ToolPropertyPanel::onAutoSelectThresholdSpinChanged);
  connect(m_autoSelectContiguousCheck, &QCheckBox::toggled, this, &ToolPropertyPanel::onAutoSelectContiguousToggled);
  connect(
      m_autoSelectReferAllLayersCheck,
      &QCheckBox::toggled,
      this,
      &ToolPropertyPanel::onAutoSelectReferAllLayersToggled);
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
  const QString compatibilityHint = QString::fromStdString(m_controller->currentLayerCompatibilityHint());
  m_compatibilityLabel->setVisible(!compatibilityHint.isEmpty());
  m_compatibilityLabel->setText(compatibilityHint);
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
  const bool supportsAngle = m_controller->currentToolSupportsAngle();
  const bool supportsRoundness = m_controller->currentToolSupportsRoundness();
  const bool supportsTaperStart = m_controller->currentToolSupportsTaperStart();
  const bool supportsTaperEnd = m_controller->currentToolSupportsTaperEnd();
  const bool supportsBlend = m_controller->currentToolSupportsBlendMode();
  const bool supportsEraseMode = m_controller->currentToolSupportsEraseMode();
  const bool supportsLockAlpha = m_controller->currentToolSupportsLockAlphaRespect();
  const bool supportsSnapAngle = m_controller->currentToolSupportsSnapAngle();
  const bool supportsSimplify = m_controller->currentToolSupportsSimplifyLevel();
  const bool supportsFillThreshold = m_controller->currentToolSupportsFillThreshold();
  const bool supportsFillContiguous = m_controller->currentToolSupportsFillContiguous();
  const bool supportsFillReferAllLayers = m_controller->currentToolSupportsFillReferAllLayers();
  const bool supportsFillGapClose = m_controller->currentToolSupportsFillGapClose();
  const bool supportsSelectionMode = m_controller->currentToolSupportsSelectionMode();
  const bool supportsAutoSelectThreshold = m_controller->currentToolSupportsAutoSelectThreshold();
  const bool supportsAutoSelectContiguous = m_controller->currentToolSupportsAutoSelectContiguous();
  const bool supportsAutoSelectReferAllLayers = m_controller->currentToolSupportsAutoSelectReferAllLayers();
  m_sizeLabel->setText((supportsSnapAngle || supportsSimplify) ? "Stroke Width" : "Size");
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
  m_angleLabel->setVisible(supportsAngle);
  m_angleSlider->setVisible(supportsAngle);
  m_angleSpin->setVisible(supportsAngle);
  m_roundnessLabel->setVisible(supportsRoundness);
  m_roundnessSlider->setVisible(supportsRoundness);
  m_roundnessSpin->setVisible(supportsRoundness);
  m_taperStartLabel->setVisible(supportsTaperStart);
  m_taperStartSlider->setVisible(supportsTaperStart);
  m_taperStartSpin->setVisible(supportsTaperStart);
  m_taperEndLabel->setVisible(supportsTaperEnd);
  m_taperEndSlider->setVisible(supportsTaperEnd);
  m_taperEndSpin->setVisible(supportsTaperEnd);
  m_shapeSection->setVisible(
      supportsShape || supportsAngle || supportsRoundness || supportsTaperStart || supportsTaperEnd);
  m_blendModeCombo->setVisible(supportsBlend);
  m_eraseModeCheck->setVisible(supportsEraseMode);
  m_lockAlphaRespectCheck->setVisible(supportsLockAlpha);
  m_snapAngleLabel->setVisible(supportsSnapAngle);
  m_snapAngleSlider->setVisible(supportsSnapAngle);
  m_snapAngleSpin->setVisible(supportsSnapAngle);
  m_simplifyLabel->setVisible(supportsSimplify);
  m_simplifySlider->setVisible(supportsSimplify);
  m_simplifySpin->setVisible(supportsSimplify);
  m_fillThresholdLabel->setVisible(supportsFillThreshold);
  m_fillThresholdSlider->setVisible(supportsFillThreshold);
  m_fillThresholdSpin->setVisible(supportsFillThreshold);
  m_fillContiguousCheck->setVisible(supportsFillContiguous);
  m_fillReferAllLayersCheck->setVisible(supportsFillReferAllLayers);
  m_fillGapCloseLabel->setVisible(supportsFillGapClose);
  m_fillGapCloseSlider->setVisible(supportsFillGapClose);
  m_fillGapCloseSpin->setVisible(supportsFillGapClose);
  m_selectionModeLabel->setVisible(supportsSelectionMode);
  m_selectionModeCombo->setVisible(supportsSelectionMode);
  m_autoSelectThresholdLabel->setVisible(supportsAutoSelectThreshold);
  m_autoSelectThresholdSlider->setVisible(supportsAutoSelectThreshold);
  m_autoSelectThresholdSpin->setVisible(supportsAutoSelectThreshold);
  m_autoSelectContiguousCheck->setVisible(supportsAutoSelectContiguous);
  m_autoSelectReferAllLayersCheck->setVisible(supportsAutoSelectReferAllLayers);
  m_drawingControlSection->setVisible(supportsBlend || supportsEraseMode || supportsLockAlpha);
  m_vectorSection->setVisible(supportsSnapAngle || supportsSimplify);
  m_fillSection->setVisible(supportsFillThreshold || supportsFillContiguous || supportsFillReferAllLayers || supportsFillGapClose);
  m_selectionSection->setVisible(
      supportsSelectionMode || supportsAutoSelectThreshold || supportsAutoSelectContiguous ||
      supportsAutoSelectReferAllLayers);

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
  const QSignalBlocker blocker16(m_angleSlider);
  const QSignalBlocker blocker17(m_angleSpin);
  const QSignalBlocker blocker18(m_roundnessSlider);
  const QSignalBlocker blocker19(m_roundnessSpin);
  const QSignalBlocker blocker20(m_taperStartSlider);
  const QSignalBlocker blocker21(m_taperStartSpin);
  const QSignalBlocker blocker22(m_taperEndSlider);
  const QSignalBlocker blocker23(m_taperEndSpin);
  const QSignalBlocker blocker24(m_snapAngleSlider);
  const QSignalBlocker blocker25(m_snapAngleSpin);
  const QSignalBlocker blocker26(m_simplifySlider);
  const QSignalBlocker blocker27(m_simplifySpin);
  const QSignalBlocker blocker28(m_blendModeCombo);
  const QSignalBlocker blocker29(m_eraseModeCheck);
  const QSignalBlocker blocker30(m_lockAlphaRespectCheck);
  const QSignalBlocker blocker31(m_fillThresholdSlider);
  const QSignalBlocker blocker32(m_fillThresholdSpin);
  const QSignalBlocker blocker33(m_fillContiguousCheck);
  const QSignalBlocker blocker34(m_fillReferAllLayersCheck);
  const QSignalBlocker blocker35(m_fillGapCloseSlider);
  const QSignalBlocker blocker36(m_fillGapCloseSpin);
  const QSignalBlocker blocker37(m_selectionModeCombo);
  const QSignalBlocker blocker38(m_autoSelectThresholdSlider);
  const QSignalBlocker blocker39(m_autoSelectThresholdSpin);
  const QSignalBlocker blocker40(m_autoSelectContiguousCheck);
  const QSignalBlocker blocker41(m_autoSelectReferAllLayersCheck);
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
  m_angleSlider->setValue(state.angle);
  m_angleSpin->setValue(state.angle);
  m_roundnessSlider->setValue(state.roundness);
  m_roundnessSpin->setValue(state.roundness);
  m_taperStartSlider->setValue(state.taperStart);
  m_taperStartSpin->setValue(state.taperStart);
  m_taperEndSlider->setValue(state.taperEnd);
  m_taperEndSpin->setValue(state.taperEnd);
  m_snapAngleSlider->setValue(state.snapAngle);
  m_snapAngleSpin->setValue(state.snapAngle);
  m_simplifySlider->setValue(state.simplifyLevel);
  m_simplifySpin->setValue(state.simplifyLevel);
  m_blendModeCombo->setCurrentIndex(m_blendModeCombo->findData(static_cast<int>(state.blendMode)));
  m_eraseModeCheck->setChecked(state.eraseMode);
  m_lockAlphaRespectCheck->setChecked(state.lockAlphaRespect);
  m_fillThresholdSlider->setValue(state.fillThreshold);
  m_fillThresholdSpin->setValue(state.fillThreshold);
  m_fillContiguousCheck->setChecked(state.fillContiguous);
  m_fillReferAllLayersCheck->setChecked(state.fillReferAllLayers);
  m_fillGapCloseSlider->setValue(state.fillGapClose);
  m_fillGapCloseSpin->setValue(state.fillGapClose);
  m_selectionModeCombo->setCurrentIndex(m_selectionModeCombo->findData(static_cast<int>(state.selectionMode)));
  m_autoSelectThresholdSlider->setValue(state.autoSelectThreshold);
  m_autoSelectThresholdSpin->setValue(state.autoSelectThreshold);
  m_autoSelectContiguousCheck->setChecked(state.autoSelectContiguous);
  m_autoSelectReferAllLayersCheck->setChecked(state.autoSelectReferAllLayers);
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

void ToolPropertyPanel::onAngleSliderChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsAngle()) {
    return;
  }
  const QSignalBlocker blocker(m_angleSpin);
  m_angleSpin->setValue(value);
  m_controller->setBrushAngle(value);
}

void ToolPropertyPanel::onAngleSpinChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsAngle()) {
    return;
  }
  const QSignalBlocker blocker(m_angleSlider);
  m_angleSlider->setValue(value);
  m_controller->setBrushAngle(value);
}

void ToolPropertyPanel::onRoundnessSliderChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsRoundness()) {
    return;
  }
  const QSignalBlocker blocker(m_roundnessSpin);
  m_roundnessSpin->setValue(value);
  m_controller->setBrushRoundness(value);
}

void ToolPropertyPanel::onRoundnessSpinChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsRoundness()) {
    return;
  }
  const QSignalBlocker blocker(m_roundnessSlider);
  m_roundnessSlider->setValue(value);
  m_controller->setBrushRoundness(value);
}

void ToolPropertyPanel::onTaperStartSliderChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsTaperStart()) {
    return;
  }
  const QSignalBlocker blocker(m_taperStartSpin);
  m_taperStartSpin->setValue(value);
  m_controller->setBrushTaperStart(value);
}

void ToolPropertyPanel::onTaperStartSpinChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsTaperStart()) {
    return;
  }
  const QSignalBlocker blocker(m_taperStartSlider);
  m_taperStartSlider->setValue(value);
  m_controller->setBrushTaperStart(value);
}

void ToolPropertyPanel::onTaperEndSliderChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsTaperEnd()) {
    return;
  }
  const QSignalBlocker blocker(m_taperEndSpin);
  m_taperEndSpin->setValue(value);
  m_controller->setBrushTaperEnd(value);
}

void ToolPropertyPanel::onTaperEndSpinChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsTaperEnd()) {
    return;
  }
  const QSignalBlocker blocker(m_taperEndSlider);
  m_taperEndSlider->setValue(value);
  m_controller->setBrushTaperEnd(value);
}

void ToolPropertyPanel::onSnapAngleSliderChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsSnapAngle()) {
    return;
  }
  const QSignalBlocker blocker(m_snapAngleSpin);
  m_snapAngleSpin->setValue(value);
  m_controller->setLineSnapAngle(value);
}

void ToolPropertyPanel::onSnapAngleSpinChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsSnapAngle()) {
    return;
  }
  const QSignalBlocker blocker(m_snapAngleSlider);
  m_snapAngleSlider->setValue(value);
  m_controller->setLineSnapAngle(value);
}

void ToolPropertyPanel::onSimplifySliderChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsSimplifyLevel()) {
    return;
  }
  const QSignalBlocker blocker(m_simplifySpin);
  m_simplifySpin->setValue(value);
  m_controller->setLineSimplifyLevel(value);
}

void ToolPropertyPanel::onSimplifySpinChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsSimplifyLevel()) {
    return;
  }
  const QSignalBlocker blocker(m_simplifySlider);
  m_simplifySlider->setValue(value);
  m_controller->setLineSimplifyLevel(value);
}

void ToolPropertyPanel::onFillThresholdSliderChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsFillThreshold()) {
    return;
  }
  const QSignalBlocker blocker(m_fillThresholdSpin);
  m_fillThresholdSpin->setValue(value);
  m_controller->setFillThreshold(value);
}

void ToolPropertyPanel::onFillThresholdSpinChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsFillThreshold()) {
    return;
  }
  const QSignalBlocker blocker(m_fillThresholdSlider);
  m_fillThresholdSlider->setValue(value);
  m_controller->setFillThreshold(value);
}

void ToolPropertyPanel::onFillContiguousToggled(bool checked) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsFillContiguous()) {
    return;
  }
  m_controller->setFillContiguous(checked);
}

void ToolPropertyPanel::onFillReferAllLayersToggled(bool checked) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsFillReferAllLayers()) {
    return;
  }
  m_controller->setFillReferAllLayers(checked);
}

void ToolPropertyPanel::onFillGapCloseSliderChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsFillGapClose()) {
    return;
  }
  const QSignalBlocker blocker(m_fillGapCloseSpin);
  m_fillGapCloseSpin->setValue(value);
  m_controller->setFillGapClose(value);
}

void ToolPropertyPanel::onFillGapCloseSpinChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsFillGapClose()) {
    return;
  }
  const QSignalBlocker blocker(m_fillGapCloseSlider);
  m_fillGapCloseSlider->setValue(value);
  m_controller->setFillGapClose(value);
}

void ToolPropertyPanel::onSelectionModeChanged(int index) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsSelectionMode()) {
    return;
  }
  const QVariant value = m_selectionModeCombo->itemData(index);
  if (!value.isValid()) {
    return;
  }
  m_controller->setSelectionMode(static_cast<app::ui::SelectionMode>(value.toInt()));
}

void ToolPropertyPanel::onAutoSelectThresholdSliderChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsAutoSelectThreshold()) {
    return;
  }
  const QSignalBlocker blocker(m_autoSelectThresholdSpin);
  m_autoSelectThresholdSpin->setValue(value);
  m_controller->setAutoSelectThreshold(value);
}

void ToolPropertyPanel::onAutoSelectThresholdSpinChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsAutoSelectThreshold()) {
    return;
  }
  const QSignalBlocker blocker(m_autoSelectThresholdSlider);
  m_autoSelectThresholdSlider->setValue(value);
  m_controller->setAutoSelectThreshold(value);
}

void ToolPropertyPanel::onAutoSelectContiguousToggled(bool checked) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsAutoSelectContiguous()) {
    return;
  }
  m_controller->setAutoSelectContiguous(checked);
}

void ToolPropertyPanel::onAutoSelectReferAllLayersToggled(bool checked) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsAutoSelectReferAllLayers()) {
    return;
  }
  m_controller->setAutoSelectReferAllLayers(checked);
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
