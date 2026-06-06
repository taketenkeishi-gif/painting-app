#include "app/panels/ToolPropertyPanel.h"

#include <algorithm>
#include <cstdint>
#include <tuple>
#include <vector>

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSettings>
#include <QSignalBlocker>
#include <QSizePolicy>
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

QString toolNameJa(core::ToolKind kind) {
  switch (kind) {
    case core::ToolKind::Brush:
      return "ブラシ";
    case core::ToolKind::Eraser:
      return "消しゴム";
    case core::ToolKind::Eyedropper:
      return "スポイト";
    case core::ToolKind::Fill:
      return "塗りつぶし";
    case core::ToolKind::Line:
      return "直線";
    case core::ToolKind::RectSelection:
      return "選択";
    case core::ToolKind::MoveLayer:
      return "移動";
    case core::ToolKind::Hand:
      return "手のひら";
    case core::ToolKind::Zoom:
      return "ズーム";
    default:
      return "ツール";
  }
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
      m_toolNameLabel(new QLabel("ツール: -", this)),
      m_guideLabel(new QLabel("", this)),
      m_compatibilityLabel(new QLabel("", this)),
      m_detailToggleButton(new QPushButton("詳細を表示", this)),
      m_pinConfigButton(new QPushButton("常設項目", this)),
      m_colorLabel(new QLabel("色", this)),
      m_sizeLabel(new QLabel("サイズ", this)),
      m_opacityLabel(new QLabel("不透明度", this)),
      m_hardnessLabel(new QLabel("硬さ", this)),
      m_flowLabel(new QLabel("流量", this)),
      m_spacingLabel(new QLabel("間隔", this)),
      m_stabilizationLabel(new QLabel("手ブレ補正", this)),
      m_snapAngleLabel(new QLabel("角度スナップ", this)),
      m_simplifyLabel(new QLabel("単純化", this)),
      m_vectorEraseModeLabel(new QLabel("ベクター消去", this)),
      m_fillThresholdLabel(new QLabel("しきい値", this)),
      m_fillGapCloseLabel(new QLabel("隙間閉じ", this)),
      m_selectionModeLabel(new QLabel("選択モード", this)),
      m_autoSelectThresholdLabel(new QLabel("自動選択しきい値", this)),
      m_colorButton(new QPushButton("色を選択", this)),
      m_sizeSpin(new QSpinBox(this)),
      m_sizeSlider(new QSlider(Qt::Horizontal, this)),
      m_opacitySlider(new QSlider(Qt::Horizontal, this)),
      m_opacitySpin(new QSpinBox(this)),
      m_hardnessSlider(new QSlider(Qt::Horizontal, this)),
      m_hardnessSpin(new QSpinBox(this)),
      m_flowSlider(new QSlider(Qt::Horizontal, this)),
      m_flowSpin(new QSpinBox(this)),
      m_spacingSlider(new QSlider(Qt::Horizontal, this)),
      m_spacingSpin(new QSpinBox(this)),
      m_antiAliasCheck(new QCheckBox("アンチエイリアス", this)),
      m_stabilizationSlider(new QSlider(Qt::Horizontal, this)),
      m_stabilizationSpin(new QSpinBox(this)),
      m_postCorrectionCheck(new QCheckBox("後補正", this)),
      m_velocityCorrectionCheck(new QCheckBox("速度補正", this)),
      m_shapeTypeCombo(new QComboBox(this)),
      m_snapAngleSlider(new QSlider(Qt::Horizontal, this)),
      m_snapAngleSpin(new QSpinBox(this)),
      m_simplifySlider(new QSlider(Qt::Horizontal, this)),
      m_simplifySpin(new QSpinBox(this)),
      m_vectorEraseModeCombo(new QComboBox(this)),
      m_vectorTrimOutsideCheck(new QCheckBox("はみ出し削除", this)),
      m_fillThresholdSlider(new QSlider(Qt::Horizontal, this)),
      m_fillThresholdSpin(new QSpinBox(this)),
      m_fillContiguousCheck(new QCheckBox("連結領域のみ", this)),
      m_fillReferAllLayersCheck(new QCheckBox("全レイヤーを参照", this)),
      m_fillGapCloseSlider(new QSlider(Qt::Horizontal, this)),
      m_fillGapCloseSpin(new QSpinBox(this)),
      m_selectionModeCombo(new QComboBox(this)),
      m_autoSelectThresholdSlider(new QSlider(Qt::Horizontal, this)),
      m_autoSelectThresholdSpin(new QSpinBox(this)),
      m_autoSelectContiguousCheck(new QCheckBox("連結領域のみ", this)),
      m_autoSelectReferAllLayersCheck(new QCheckBox("全レイヤーを参照", this)),
      m_selectionFeatherSlider(new QSlider(Qt::Horizontal, this)),
      m_selectionFeatherSpin(new QSpinBox(this)),
      m_selectionAntiAliasCheck(new QCheckBox("アンチエイリアス", this)),
      m_selOpNewBtn      (new QPushButton("新規",  this)),
      m_selOpAddBtn      (new QPushButton("+追加", this)),
      m_selOpSubtractBtn (new QPushButton("−削除", this)),
      m_selOpIntersectBtn(new QPushButton("∩共通", this)),
      m_selectionOpLabel (new QLabel("選択モード", this)),
      m_selectionExpandLabel  (new QLabel("拡張(px)", this)),
      m_selectionExpandSpin   (new QSpinBox(this)),
      m_selectionGapCloseLabel(new QLabel("ギャップ(px)", this)),
      m_selectionGapCloseSpin (new QSpinBox(this)),
      m_selectionEdgeSnapCheck(new QCheckBox("エッジ吸着", this)),
      m_blendModeCombo(new QComboBox(this)),
      m_buildupModeCheck(new QCheckBox("積み上げモード（Buildup）", this)),
      m_eraseModeCheck(new QCheckBox("消しゴムモード", this)),
      m_lockAlphaRespectCheck(new QCheckBox("透明保護を尊重", this)),
      m_pressureSection(nullptr),
      m_pressureSizeCheck(new QCheckBox("筆圧→サイズ", this)),
      m_pressureSizeMinSlider(new QSlider(Qt::Horizontal, this)),
      m_pressureSizeMinSpin(new QSpinBox(this)),
      m_pressureOpacityCheck(new QCheckBox("筆圧→不透明度", this)),
      m_pressureOpacityMinSlider(new QSlider(Qt::Horizontal, this)),
      m_pressureOpacityMinSpin(new QSpinBox(this)),
      m_velocitySection(nullptr),
      m_velocitySizeCheck(new QCheckBox("速度→サイズ", this)),
      m_velocitySizeMinSlider(new QSlider(Qt::Horizontal, this)),
      m_velocityOpacityCheck(new QCheckBox("速度→不透明度", this)),
      m_velocityOpacityMinSlider(new QSlider(Qt::Horizontal, this)),
      m_textureSection(nullptr),
      m_textureGrainCheck(new QCheckBox("テクスチャグレイン", this)),
      m_textureStrengthSlider(new QSlider(Qt::Horizontal, this)),
      m_textureScaleSlider(new QSlider(Qt::Horizontal, this)),
      m_wetSection(nullptr),
      m_wetMixCheck(new QCheckBox("ウェットミックス", this)),
      m_wetMixRateSlider(new QSlider(Qt::Horizontal, this)),
      m_smearCheck(new QCheckBox("スメア（にじみ）", this)),
      m_smearRateSlider(new QSlider(Qt::Horizontal, this)) {
  auto* hostLayout = new QVBoxLayout(this);
  hostLayout->setContentsMargins(0, 0, 0, 0);
  hostLayout->setSpacing(0);

  m_guideLabel->setWordWrap(true);
  m_compatibilityLabel->setWordWrap(true);
  m_guideLabel->setStyleSheet("color: #aeb8c8; font-size: 10px;");
  m_compatibilityLabel->setStyleSheet("color: #f3bf58; font-weight: 600;");
  m_guideLabel->setVisible(false);
  m_sizeSpin->setRange(1, 2048);
  m_sizeSlider->setRange(1, 200);
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
  std::tie(m_angleLabel, m_angleSlider, m_angleSpin) =
      createLabeledSlider("角度", -180, 180, 0);
  std::tie(m_roundnessLabel, m_roundnessSlider, m_roundnessSpin) =
      createLabeledSlider("真円率", 0, 100, 0);
  std::tie(m_taperStartLabel, m_taperStartSlider, m_taperStartSpin) =
      createLabeledSlider("入り抜き（始点）", 0, 100, 0);
  std::tie(m_taperEndLabel, m_taperEndSlider, m_taperEndSpin) =
      createLabeledSlider("入り抜き（終点）", 0, 100, 0);
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
  m_selectionFeatherSlider->setRange(0, 100);
  m_selectionFeatherSpin->setRange(0, 100);
  m_selectionExpandSpin->setRange(0, 100);
  m_selectionGapCloseSpin->setRange(0, 20);
  // op buttons: checkable, exclusive
  for (auto* btn : {m_selOpNewBtn, m_selOpAddBtn, m_selOpSubtractBtn, m_selOpIntersectBtn}) {
    btn->setCheckable(true);
    btn->setFixedHeight(22);
  }
  m_selOpNewBtn->setChecked(true);
  m_pressureSizeMinSlider->setRange(0, 100);
  m_pressureSizeMinSpin->setRange(0, 100);
  m_pressureOpacityMinSlider->setRange(0, 100);
  m_pressureOpacityMinSpin->setRange(0, 100);

  // 速度感応スライダー (0-100 → 比率 0.0-1.0)
  m_velocitySizeMinSlider->setRange(0, 100);
  m_velocityOpacityMinSlider->setRange(0, 100);
  // テクスチャグレイン (0-100)
  m_textureStrengthSlider->setRange(0, 100);
  m_textureScaleSlider->setRange(10, 400);  // 0.1 - 4.0 (×0.01)
  // ウェットミックス / スメア (0-100)
  m_wetMixRateSlider->setRange(0, 100);
  m_smearRateSlider->setRange(0, 100);

  m_shapeTypeCombo->addItem("円",   static_cast<int>(core::BrushShapeType::Circle));
  m_shapeTypeCombo->addItem("四角", static_cast<int>(core::BrushShapeType::Square));
  m_shapeTypeCombo->addItem("楕円", static_cast<int>(core::BrushShapeType::Ellipse));

  auto addBrushBlend = [&](const char* label, core::BlendMode mode) {
    m_blendModeCombo->addItem(label, static_cast<int>(mode));
  };
  // 基本
  addBrushBlend("通常",              core::BlendMode::Normal);
  addBrushBlend("ディザ合成",        core::BlendMode::Dissolve);
  // 暗くする
  addBrushBlend("比較（暗）",        core::BlendMode::Darken);
  addBrushBlend("乗算",              core::BlendMode::Multiply);
  addBrushBlend("焼き込みカラー",    core::BlendMode::ColorBurn);
  addBrushBlend("焼き込みリニア",    core::BlendMode::LinearBurn);
  addBrushBlend("カラー比較（暗）",  core::BlendMode::DarkerColor);
  // 明るくする
  addBrushBlend("比較（明）",        core::BlendMode::Lighten);
  addBrushBlend("スクリーン",        core::BlendMode::Screen);
  addBrushBlend("覆い焼きカラー",    core::BlendMode::ColorDodge);
  addBrushBlend("加算",              core::BlendMode::LinearDodge);
  addBrushBlend("カラー比較（明）",  core::BlendMode::LighterColor);
  // コントラスト
  addBrushBlend("オーバーレイ",      core::BlendMode::Overlay);
  addBrushBlend("ソフトライト",      core::BlendMode::SoftLight);
  addBrushBlend("ハードライト",      core::BlendMode::HardLight);
  addBrushBlend("ビビットライト",    core::BlendMode::VividLight);
  addBrushBlend("リニアライト",      core::BlendMode::LinearLight);
  addBrushBlend("ピンライト",        core::BlendMode::PinLight);
  addBrushBlend("ハードミックス",    core::BlendMode::HardMix);
  // 比較
  addBrushBlend("差の絶対値",        core::BlendMode::Difference);
  addBrushBlend("除外",              core::BlendMode::Exclusion);
  addBrushBlend("減算",              core::BlendMode::Subtract);
  addBrushBlend("除算",              core::BlendMode::Divide);
  // カラー成分 (HSL)
  addBrushBlend("色相",              core::BlendMode::Hue);
  addBrushBlend("彩度",              core::BlendMode::HslSat);
  addBrushBlend("カラー",            core::BlendMode::HslColor);
  addBrushBlend("輝度",              core::BlendMode::Luminosity);

  m_selectionModeCombo->addItem("矩形", static_cast<int>(app::ui::SelectionMode::Rectangle));
  m_selectionModeCombo->addItem("なげなわ", static_cast<int>(app::ui::SelectionMode::Lasso));
  m_selectionModeCombo->addItem("多角形選択", static_cast<int>(app::ui::SelectionMode::PolygonLasso));
  m_selectionModeCombo->addItem("自動選択", static_cast<int>(app::ui::SelectionMode::AutoSelect));
  m_vectorEraseModeCombo->addItem("触れた部分を削除", static_cast<int>(app::ui::VectorEraserMode::TouchedOnly));
  m_vectorEraseModeCombo->addItem("交点まで削除", static_cast<int>(app::ui::VectorEraserMode::ToIntersection));
  m_vectorEraseModeCombo->addItem("はみ出し部分を削除", static_cast<int>(app::ui::VectorEraserMode::TrimOutside));

  auto* contentLayout = new QVBoxLayout(m_contentWidget);
  contentLayout->setContentsMargins(2, 2, 2, 2);
  contentLayout->setSpacing(2);

  auto markResponsiveRow = [](QHBoxLayout* row) {
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(4);
    row->setProperty("responsiveRow", true);
  };

  auto* titleFrame = new QFrame(m_contentWidget);
  auto* titleLayout = new QVBoxLayout(titleFrame);
  titleLayout->setContentsMargins(2, 2, 2, 2);
  titleLayout->setSpacing(1);

  // Compact header row: [tool name (stretch)] [detail btn 20x20] [pin btn 20x20]
  auto* titleRow = new QHBoxLayout();
  titleRow->setContentsMargins(0, 0, 0, 0);
  titleRow->setSpacing(2);

  m_toolNameLabel->setStyleSheet(QStringLiteral(
      "QLabel { font-weight: bold; font-size: 10px; }"));

  m_detailToggleButton->setFixedSize(20, 20);
  m_pinConfigButton->setFixedSize(20, 20);
  m_detailToggleButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  m_pinConfigButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  // Use compact symbols for the icon buttons
  m_detailToggleButton->setText(QString::fromUtf8("\xe2\x89\xa1"));   // ≡
  m_pinConfigButton->setText(QString::fromUtf8("\xe2\x8a\x95"));      // ⊕

  titleRow->addWidget(m_toolNameLabel, 1);
  titleRow->addWidget(m_detailToggleButton, 0);
  titleRow->addWidget(m_pinConfigButton, 0);

  titleLayout->addLayout(titleRow);
  titleLayout->addWidget(m_compatibilityLabel);
  titleLayout->addWidget(m_guideLabel);
  contentLayout->addWidget(titleFrame);

  auto* basicGroup = new QGroupBox("基本", m_contentWidget);
  auto* basicLayout = new QVBoxLayout(basicGroup);
  basicLayout->setContentsMargins(4, 4, 4, 4);
  basicLayout->setSpacing(4);

  auto* opacityRow = new QHBoxLayout();
  markResponsiveRow(opacityRow);
  opacityRow->addWidget(m_opacitySlider, 1);
  opacityRow->addWidget(m_opacitySpin);

  auto* hardnessRow = new QHBoxLayout();
  markResponsiveRow(hardnessRow);
  hardnessRow->addWidget(m_hardnessSlider, 1);
  hardnessRow->addWidget(m_hardnessSpin);

  auto* sizeRow = new QHBoxLayout();
  markResponsiveRow(sizeRow);
  sizeRow->addWidget(m_sizeSlider, 1);
  sizeRow->addWidget(m_sizeSpin);

  basicLayout->addWidget(m_colorLabel);
  basicLayout->addWidget(m_colorButton);
  basicLayout->addWidget(m_sizeLabel);
  basicLayout->addLayout(sizeRow);
  basicLayout->addWidget(m_opacityLabel);
  basicLayout->addLayout(opacityRow);
  basicLayout->addWidget(m_hardnessLabel);
  basicLayout->addLayout(hardnessRow);
  contentLayout->addWidget(basicGroup);

  auto* dynamicsGroup = new QGroupBox("ブラシ特性", m_contentWidget);
  auto* dynamicsLayout = new QVBoxLayout(dynamicsGroup);
  dynamicsLayout->setContentsMargins(4, 4, 4, 4);
  dynamicsLayout->setSpacing(4);

  appendLabeledRow(dynamicsLayout, m_flowLabel, m_flowSlider, m_flowSpin);

  auto* spacingRow = new QHBoxLayout();
  markResponsiveRow(spacingRow);
  spacingRow->addWidget(m_spacingSlider, 1);
  spacingRow->addWidget(m_spacingSpin);

  dynamicsLayout->addWidget(m_spacingLabel);
  dynamicsLayout->addLayout(spacingRow);
  contentLayout->addWidget(dynamicsGroup);
  m_brushDynamicsSection = dynamicsGroup;

  auto* correctionGroup = new QGroupBox("補正", m_contentWidget);
  auto* correctionLayout = new QVBoxLayout(correctionGroup);
  correctionLayout->setContentsMargins(4, 4, 4, 4);
  correctionLayout->setSpacing(4);

  auto* stabilizationRow = new QHBoxLayout();
  markResponsiveRow(stabilizationRow);
  stabilizationRow->addWidget(m_stabilizationSlider, 1);
  stabilizationRow->addWidget(m_stabilizationSpin);

  correctionLayout->addWidget(m_antiAliasCheck);
  correctionLayout->addWidget(m_stabilizationLabel);
  correctionLayout->addLayout(stabilizationRow);
  correctionLayout->addWidget(m_postCorrectionCheck);
  correctionLayout->addWidget(m_velocityCorrectionCheck);
  contentLayout->addWidget(correctionGroup);
  m_correctionSection = correctionGroup;

  auto* pressureGroup = new QGroupBox("筆圧ダイナミクス", m_contentWidget);
  auto* pressureLayout = new QVBoxLayout(pressureGroup);
  pressureLayout->setContentsMargins(4, 4, 4, 4);
  pressureLayout->setSpacing(4);
  auto* pressureSizeMinRow = new QHBoxLayout();
  markResponsiveRow(pressureSizeMinRow);
  pressureSizeMinRow->addWidget(m_pressureSizeMinSlider, 1);
  pressureSizeMinRow->addWidget(m_pressureSizeMinSpin);
  auto* pressureOpacityMinRow = new QHBoxLayout();
  markResponsiveRow(pressureOpacityMinRow);
  pressureOpacityMinRow->addWidget(m_pressureOpacityMinSlider, 1);
  pressureOpacityMinRow->addWidget(m_pressureOpacityMinSpin);
  pressureLayout->addWidget(m_pressureSizeCheck);
  pressureLayout->addLayout(pressureSizeMinRow);
  pressureLayout->addWidget(m_pressureOpacityCheck);
  pressureLayout->addLayout(pressureOpacityMinRow);
  contentLayout->addWidget(pressureGroup);
  m_pressureSection = pressureGroup;

  // ── 速度感応セクション ──────────────────────────────────────────────────
  {
    auto* grp = new QGroupBox("速度感応", m_contentWidget);
    auto* lay = new QVBoxLayout(grp);
    lay->setContentsMargins(4, 4, 4, 4); lay->setSpacing(4);

    auto addVelRow = [&](QCheckBox* check, QSlider* slider, const QString& minLabel) {
      lay->addWidget(check);
      auto* row = new QHBoxLayout();
      row->addWidget(new QLabel(minLabel, grp));
      row->addWidget(slider, 1);
      lay->addLayout(row);
    };
    addVelRow(m_velocitySizeCheck,    m_velocitySizeMinSlider,    "最小サイズ%");
    addVelRow(m_velocityOpacityCheck, m_velocityOpacityMinSlider, "最小不透明%");

    contentLayout->addWidget(grp);
    m_velocitySection = grp;
  }

  // ── テクスチャグレインセクション ───────────────────────────────────────
  {
    auto* grp = new QGroupBox("テクスチャグレイン", m_contentWidget);
    auto* lay = new QVBoxLayout(grp);
    lay->setContentsMargins(4, 4, 4, 4); lay->setSpacing(4);

    lay->addWidget(m_textureGrainCheck);
    auto* strRow = new QHBoxLayout();
    strRow->addWidget(new QLabel("強度", grp));
    strRow->addWidget(m_textureStrengthSlider, 1);
    lay->addLayout(strRow);
    auto* scaleRow = new QHBoxLayout();
    scaleRow->addWidget(new QLabel("粗さ", grp));
    scaleRow->addWidget(m_textureScaleSlider, 1);
    lay->addLayout(scaleRow);

    contentLayout->addWidget(grp);
    m_textureSection = grp;
  }

  // ── ウェットミックス / スメアセクション ────────────────────────────────
  {
    auto* grp = new QGroupBox("ウェット / スメア", m_contentWidget);
    auto* lay = new QVBoxLayout(grp);
    lay->setContentsMargins(4, 4, 4, 4); lay->setSpacing(4);

    lay->addWidget(m_wetMixCheck);
    auto* wetRow = new QHBoxLayout();
    wetRow->addWidget(new QLabel("混合率", grp));
    wetRow->addWidget(m_wetMixRateSlider, 1);
    lay->addLayout(wetRow);
    lay->addWidget(m_smearCheck);
    auto* smearRow = new QHBoxLayout();
    smearRow->addWidget(new QLabel("強度", grp));
    smearRow->addWidget(m_smearRateSlider, 1);
    lay->addLayout(smearRow);

    contentLayout->addWidget(grp);
    m_wetSection = grp;
  }

  auto* shapeGroup = new QGroupBox("形状", m_contentWidget);
  auto* shapeLayout = new QVBoxLayout(shapeGroup);
  shapeLayout->setContentsMargins(4, 4, 4, 4);
  shapeLayout->setSpacing(4);
  shapeLayout->addWidget(m_shapeTypeCombo);
  appendLabeledRow(shapeLayout, m_angleLabel, m_angleSlider, m_angleSpin);
  appendLabeledRow(shapeLayout, m_roundnessLabel, m_roundnessSlider, m_roundnessSpin);
  appendLabeledRow(shapeLayout, m_taperStartLabel, m_taperStartSlider, m_taperStartSpin);
  appendLabeledRow(shapeLayout, m_taperEndLabel, m_taperEndSlider, m_taperEndSpin);
  contentLayout->addWidget(shapeGroup);
  m_shapeSection = shapeGroup;

  auto* drawControlGroup = new QGroupBox("描画制御", m_contentWidget);
  auto* drawControlLayout = new QVBoxLayout(drawControlGroup);
  drawControlLayout->setContentsMargins(4, 4, 4, 4);
  drawControlLayout->setSpacing(4);
  drawControlLayout->addWidget(m_blendModeCombo);
  drawControlLayout->addWidget(m_buildupModeCheck);
  drawControlLayout->addWidget(m_eraseModeCheck);
  drawControlLayout->addWidget(m_lockAlphaRespectCheck);
  contentLayout->addWidget(drawControlGroup);
  m_drawingControlSection = drawControlGroup;

  auto* vectorGroup = new QGroupBox("ベクター", m_contentWidget);
  auto* vectorLayout = new QVBoxLayout(vectorGroup);
  vectorLayout->setContentsMargins(4, 4, 4, 4);
  vectorLayout->setSpacing(4);
  auto* snapAngleRow = new QHBoxLayout();
  markResponsiveRow(snapAngleRow);
  snapAngleRow->addWidget(m_snapAngleSlider, 1);
  snapAngleRow->addWidget(m_snapAngleSpin);
  auto* simplifyRow = new QHBoxLayout();
  markResponsiveRow(simplifyRow);
  simplifyRow->addWidget(m_simplifySlider, 1);
  simplifyRow->addWidget(m_simplifySpin);
  vectorLayout->addWidget(m_snapAngleLabel);
  vectorLayout->addLayout(snapAngleRow);
  vectorLayout->addWidget(m_simplifyLabel);
  vectorLayout->addLayout(simplifyRow);
  vectorLayout->addWidget(m_vectorEraseModeLabel);
  vectorLayout->addWidget(m_vectorEraseModeCombo);
  vectorLayout->addWidget(m_vectorTrimOutsideCheck);
  contentLayout->addWidget(vectorGroup);
  m_vectorSection = vectorGroup;

  auto* fillGroup = new QGroupBox("塗りつぶし", m_contentWidget);
  auto* fillLayout = new QVBoxLayout(fillGroup);
  fillLayout->setContentsMargins(4, 4, 4, 4);
  fillLayout->setSpacing(4);
  auto* fillThresholdRow = new QHBoxLayout();
  markResponsiveRow(fillThresholdRow);
  fillThresholdRow->addWidget(m_fillThresholdSlider, 1);
  fillThresholdRow->addWidget(m_fillThresholdSpin);
  auto* fillGapCloseRow = new QHBoxLayout();
  markResponsiveRow(fillGapCloseRow);
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

  m_selectionFeatherLabel = new QLabel("フェザー半径", this);
  auto* selectionGroup = new QGroupBox("選択", m_contentWidget);
  auto* selectionLayout = new QVBoxLayout(selectionGroup);
  selectionLayout->setContentsMargins(4, 4, 4, 4);
  selectionLayout->setSpacing(4);
  auto* autoSelectThresholdRow = new QHBoxLayout();
  markResponsiveRow(autoSelectThresholdRow);
  autoSelectThresholdRow->addWidget(m_autoSelectThresholdSlider, 1);
  autoSelectThresholdRow->addWidget(m_autoSelectThresholdSpin);
  auto* featherRow = new QHBoxLayout();
  markResponsiveRow(featherRow);
  featherRow->addWidget(m_selectionFeatherSlider, 1);
  featherRow->addWidget(m_selectionFeatherSpin);
  // op buttons row
  auto* opRow = new QHBoxLayout();
  opRow->setSpacing(2);
  opRow->addWidget(m_selOpNewBtn);
  opRow->addWidget(m_selOpAddBtn);
  opRow->addWidget(m_selOpSubtractBtn);
  opRow->addWidget(m_selOpIntersectBtn);
  // expand / gap close rows
  auto* expandRow = new QHBoxLayout();
  expandRow->addWidget(m_selectionExpandLabel);
  expandRow->addWidget(m_selectionExpandSpin);
  auto* gapRow = new QHBoxLayout();
  gapRow->addWidget(m_selectionGapCloseLabel);
  gapRow->addWidget(m_selectionGapCloseSpin);

  selectionLayout->addWidget(m_selectionModeLabel);
  selectionLayout->addWidget(m_selectionModeCombo);
  selectionLayout->addWidget(m_selectionOpLabel);
  selectionLayout->addLayout(opRow);
  selectionLayout->addWidget(m_autoSelectThresholdLabel);
  selectionLayout->addLayout(autoSelectThresholdRow);
  selectionLayout->addWidget(m_autoSelectContiguousCheck);
  selectionLayout->addWidget(m_autoSelectReferAllLayersCheck);
  selectionLayout->addWidget(m_selectionFeatherLabel);
  selectionLayout->addLayout(featherRow);
  selectionLayout->addWidget(m_selectionAntiAliasCheck);
  selectionLayout->addLayout(expandRow);
  selectionLayout->addLayout(gapRow);
  selectionLayout->addWidget(m_selectionEdgeSnapCheck);
  contentLayout->addWidget(selectionGroup);
  m_selectionSection = selectionGroup;

  contentLayout->addStretch(1);

  m_scrollArea->setWidgetResizable(true);
  m_scrollArea->setFrameShape(QFrame::NoFrame);
  m_scrollArea->setWidget(m_contentWidget);
  hostLayout->addWidget(m_scrollArea);

  connect(m_detailToggleButton, &QPushButton::clicked, this, &ToolPropertyPanel::onToggleDetailRequested);
  connect(m_pinConfigButton, &QPushButton::clicked, this, &ToolPropertyPanel::onConfigurePinnedRequested);
  connect(m_colorButton, &QPushButton::clicked, this, &ToolPropertyPanel::onChooseColor);
  connect(m_sizeSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onSizeChanged);
  connect(m_sizeSlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onSizeSliderChanged);
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
  connect(m_vectorEraseModeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &ToolPropertyPanel::onVectorEraseModeChanged);
  connect(m_vectorTrimOutsideCheck, &QCheckBox::toggled, this, &ToolPropertyPanel::onVectorTrimOutsideToggled);
  connect(m_fillThresholdSlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onFillThresholdSliderChanged);
  connect(m_fillThresholdSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onFillThresholdSpinChanged);
  connect(m_fillContiguousCheck, &QCheckBox::toggled, this, &ToolPropertyPanel::onFillContiguousToggled);
  connect(m_fillReferAllLayersCheck, &QCheckBox::toggled, this, &ToolPropertyPanel::onFillReferAllLayersToggled);
  connect(m_fillGapCloseSlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onFillGapCloseSliderChanged);
  connect(m_fillGapCloseSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onFillGapCloseSpinChanged);
  connect(m_selectionModeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &ToolPropertyPanel::onSelectionModeChanged);
  connect(m_selectionFeatherSlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onSelectionFeatherSliderChanged);
  connect(m_selectionFeatherSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onSelectionFeatherSpinChanged);
  connect(m_selectionAntiAliasCheck, &QCheckBox::toggled, this, &ToolPropertyPanel::onSelectionAntiAliasToggled);
  connect(m_selOpNewBtn,       &QPushButton::clicked, this, [this]{ onSelectionOpClicked(0); });
  connect(m_selOpAddBtn,       &QPushButton::clicked, this, [this]{ onSelectionOpClicked(1); });
  connect(m_selOpSubtractBtn,  &QPushButton::clicked, this, [this]{ onSelectionOpClicked(2); });
  connect(m_selOpIntersectBtn, &QPushButton::clicked, this, [this]{ onSelectionOpClicked(3); });
  connect(m_selectionExpandSpin,   qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onSelectionExpandChanged);
  connect(m_selectionGapCloseSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onSelectionGapCloseChanged);
  connect(m_selectionEdgeSnapCheck, &QCheckBox::toggled, this, &ToolPropertyPanel::onSelectionEdgeSnapToggled);
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
  connect(m_buildupModeCheck, &QCheckBox::toggled, this, &ToolPropertyPanel::onBuildupModeToggled);
  connect(m_eraseModeCheck, &QCheckBox::toggled, this, &ToolPropertyPanel::onEraseModeToggled);
  connect(m_lockAlphaRespectCheck, &QCheckBox::toggled, this, &ToolPropertyPanel::onLockAlphaRespectToggled);
  connect(m_pressureSizeCheck, &QCheckBox::toggled, this, &ToolPropertyPanel::onPressureSizeToggled);
  connect(m_pressureSizeMinSlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onPressureSizeMinSliderChanged);
  connect(m_pressureSizeMinSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onPressureSizeMinSpinChanged);
  connect(m_pressureOpacityCheck, &QCheckBox::toggled, this, &ToolPropertyPanel::onPressureOpacityToggled);
  connect(m_pressureOpacityMinSlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onPressureOpacityMinSliderChanged);
  connect(m_pressureOpacityMinSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onPressureOpacityMinSpinChanged);

  // 速度感応
  connect(m_velocitySizeCheck,       &QCheckBox::toggled,         this, &ToolPropertyPanel::onVelocitySizeToggled);
  connect(m_velocitySizeMinSlider,   &QSlider::valueChanged,      this, &ToolPropertyPanel::onVelocitySizeMinSliderChanged);
  connect(m_velocityOpacityCheck,    &QCheckBox::toggled,         this, &ToolPropertyPanel::onVelocityOpacityToggled);
  connect(m_velocityOpacityMinSlider,&QSlider::valueChanged,      this, &ToolPropertyPanel::onVelocityOpacityMinSliderChanged);
  // テクスチャグレイン
  connect(m_textureGrainCheck,       &QCheckBox::toggled,         this, &ToolPropertyPanel::onTextureGrainToggled);
  connect(m_textureStrengthSlider,   &QSlider::valueChanged,      this, &ToolPropertyPanel::onTextureStrengthSliderChanged);
  connect(m_textureScaleSlider,      &QSlider::valueChanged,      this, &ToolPropertyPanel::onTextureScaleSliderChanged);
  // ウェットミックス / スメア
  connect(m_wetMixCheck,             &QCheckBox::toggled,         this, &ToolPropertyPanel::onWetMixToggled);
  connect(m_wetMixRateSlider,        &QSlider::valueChanged,      this, &ToolPropertyPanel::onWetMixRateSliderChanged);
  connect(m_smearCheck,              &QCheckBox::toggled,         this, &ToolPropertyPanel::onSmearToggled);
  connect(m_smearRateSlider,         &QSlider::valueChanged,      this, &ToolPropertyPanel::onSmearRateSliderChanged);

  applyResponsiveLayout();
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
  loadPinnedForCurrentTool();
  refreshDetailToggleText();
  refreshFromController();
}

void ToolPropertyPanel::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  applyResponsiveLayout();
}

void ToolPropertyPanel::applyResponsiveLayout() {
  const bool compact = width() < 260;
  if (compact == m_compactLayout || m_contentWidget == nullptr) {
    return;
  }
  m_compactLayout = compact;

  const auto rows = m_contentWidget->findChildren<QHBoxLayout*>();
  for (QHBoxLayout* row : rows) {
    if (row == nullptr || !row->property("responsiveRow").toBool()) {
      continue;
    }
    row->setDirection(compact ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    row->setSpacing(compact ? 2 : 4);
  }
}

QString ToolPropertyPanel::currentToolSettingsKey() const {
  if (m_controller == nullptr) {
    return QStringLiteral("none");
  }
  const QString subToolId = QString::fromStdString(m_controller->currentSubToolId());
  QString toolKey;
  switch (m_controller->currentTool()) {
    case core::ToolKind::Brush:
      toolKey = QStringLiteral("brush");
      break;
    case core::ToolKind::Eraser:
      toolKey = QStringLiteral("eraser");
      break;
    case core::ToolKind::Eyedropper:
      toolKey = QStringLiteral("eyedropper");
      break;
    case core::ToolKind::Fill:
      toolKey = QStringLiteral("fill");
      break;
    case core::ToolKind::Line:
      toolKey = QStringLiteral("line");
      break;
    case core::ToolKind::RectSelection:
      toolKey = QStringLiteral("selection");
      break;
    case core::ToolKind::MoveLayer:
      toolKey = QStringLiteral("move_layer");
      break;
    case core::ToolKind::Hand:
      toolKey = QStringLiteral("hand");
      break;
    case core::ToolKind::Zoom:
      toolKey = QStringLiteral("zoom");
      break;
    default:
      toolKey = QStringLiteral("tool");
      break;
  }
  if (subToolId.isEmpty()) {
    return toolKey;
  }
  return QStringLiteral("%1/%2").arg(toolKey, subToolId);
}

bool ToolPropertyPanel::isPinned(const QString& key) const {
  return m_pinnedKeys.contains(key);
}

void ToolPropertyPanel::loadPinnedForCurrentTool() {
  const QString toolKey = currentToolSettingsKey();
  m_lastPinnedToolKey = toolKey;

  QSettings settings("taketenkeishi", "LayeredPaintApp");
  settings.beginGroup("toolPropertyPinned");
  const QStringList stored = settings.value(
      toolKey,
      QStringList {QStringLiteral("size"), QStringLiteral("opacity"), QStringLiteral("hardness"), QStringLiteral("blend")})
                                 .toStringList();
  settings.endGroup();

  m_pinnedKeys.clear();
  for (const QString& key : stored) {
    if (!key.isEmpty()) {
      m_pinnedKeys.insert(key);
    }
  }
}

void ToolPropertyPanel::savePinnedForCurrentTool() const {
  const QString toolKey = currentToolSettingsKey();
  QStringList out;
  out.reserve(m_pinnedKeys.size());
  for (const QString& key : m_pinnedKeys) {
    out.push_back(key);
  }
  std::sort(out.begin(), out.end(), [](const QString& lhs, const QString& rhs) { return lhs < rhs; });

  QSettings settings("taketenkeishi", "LayeredPaintApp");
  settings.beginGroup("toolPropertyPinned");
  settings.setValue(toolKey, out);
  settings.endGroup();
}

void ToolPropertyPanel::refreshDetailToggleText() {
  if (m_detailToggleButton == nullptr) {
    return;
  }
  // Keep compact symbol; use tooltip to communicate state
  m_detailToggleButton->setToolTip(m_showDetails ? QStringLiteral("詳細を隠す") : QStringLiteral("詳細を表示"));
}

void ToolPropertyPanel::onToggleDetailRequested() {
  m_showDetails = !m_showDetails;
  refreshDetailToggleText();
  refreshFromController();
}

void ToolPropertyPanel::onConfigurePinnedRequested() {
  QDialog dialog(this);
  dialog.setWindowTitle(QStringLiteral("常設項目の設定"));
  auto* layout = new QVBoxLayout(&dialog);
  layout->setContentsMargins(10, 10, 10, 10);
  layout->setSpacing(8);

  auto* info = new QLabel(QStringLiteral("常設表示したい項目を選択してください。"), &dialog);
  info->setWordWrap(true);
  layout->addWidget(info);

  struct PinRow {
    QString key;
    QString label;
  };
  const std::vector<PinRow> rows {
      {QStringLiteral("color"), QStringLiteral("色")},
      {QStringLiteral("size"), QStringLiteral("サイズ")},
      {QStringLiteral("opacity"), QStringLiteral("不透明度")},
      {QStringLiteral("hardness"), QStringLiteral("硬さ")},
      {QStringLiteral("blend"), QStringLiteral("合成モード")},
      {QStringLiteral("antialias"), QStringLiteral("アンチエイリアス")},
      {QStringLiteral("stabilization"), QStringLiteral("手ブレ補正")},
      {QStringLiteral("vector_mode"), QStringLiteral("ベクター消去モード")},
      {QStringLiteral("vector_trim"), QStringLiteral("はみ出し削除")}};

  QList<QCheckBox*> checks;
  for (const PinRow& row : rows) {
    auto* check = new QCheckBox(row.label, &dialog);
    check->setProperty("pinKey", row.key);
    check->setChecked(m_pinnedKeys.contains(row.key));
    checks.push_back(check);
    layout->addWidget(check);
  }

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  if (dialog.exec() != QDialog::Accepted) {
    return;
  }

  m_pinnedKeys.clear();
  for (QCheckBox* check : checks) {
    if (check != nullptr && check->isChecked()) {
      m_pinnedKeys.insert(check->property("pinKey").toString());
    }
  }
  savePinnedForCurrentTool();
  refreshFromController();
}

void ToolPropertyPanel::refreshFromController() {
  if (m_controller == nullptr) {
    return;
  }

  const QString toolKey = currentToolSettingsKey();
  if (toolKey != m_lastPinnedToolKey) {
    loadPinnedForCurrentTool();
  }
  refreshDetailToggleText();

  m_toolNameLabel->setText(QString("ツール: %1").arg(toolNameJa(m_controller->currentTool())));
  const QString compatibilityHint = QString::fromStdString(m_controller->currentLayerCompatibilityHint());
  m_compatibilityLabel->setVisible(!compatibilityHint.isEmpty());
  m_compatibilityLabel->setText(compatibilityHint);
  const QString guideText = QString::fromStdString(m_controller->currentToolGuide()).trimmed();
  m_guideLabel->setText(guideText);
  m_guideLabel->setVisible(!guideText.isEmpty());

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
  const bool supportsVectorEraseMode = m_controller->currentToolSupportsVectorEraseMode();
  const bool supportsVectorTrimOutside = m_controller->currentToolSupportsVectorTrimOutside();
  const bool supportsFillThreshold = m_controller->currentToolSupportsFillThreshold();
  const bool supportsFillContiguous = m_controller->currentToolSupportsFillContiguous();
  const bool supportsFillReferAllLayers = m_controller->currentToolSupportsFillReferAllLayers();
  const bool supportsFillGapClose = m_controller->currentToolSupportsFillGapClose();
  const bool supportsSelectionMode = m_controller->currentToolSupportsSelectionMode();
  const bool supportsAutoSelectThreshold = m_controller->currentToolSupportsAutoSelectThreshold();
  const bool supportsAutoSelectContiguous = m_controller->currentToolSupportsAutoSelectContiguous();
  const bool supportsAutoSelectReferAllLayers = m_controller->currentToolSupportsAutoSelectReferAllLayers();
  const auto pinnedOrDetail = [this](bool supports, const QString& key) {
    return supports && (m_showDetails || isPinned(key));
  };
  const bool supportsStrokeWidth = m_controller->currentToolHasProperty(app::ui::ToolPropertyKey::StrokeWidth);
  m_sizeLabel->setText(supportsStrokeWidth ? "線幅" : "サイズ");
  const bool showColor = pinnedOrDetail(supportsColor, QStringLiteral("color"));
  const bool showSize = supportsStrokeWidth || pinnedOrDetail(supportsSize, QStringLiteral("size"));
  const bool showOpacity = pinnedOrDetail(supportsOpacity, QStringLiteral("opacity"));
  const bool showHardness = pinnedOrDetail(supportsHardness, QStringLiteral("hardness"));
  const bool showBlend = pinnedOrDetail(supportsBlend, QStringLiteral("blend"));
  const bool showAntiAlias = pinnedOrDetail(supportsAntiAlias, QStringLiteral("antialias"));
  const bool showStabilization = pinnedOrDetail(supportsStabilization, QStringLiteral("stabilization"));
  const bool showVectorMode = pinnedOrDetail(supportsVectorEraseMode, QStringLiteral("vector_mode"));
  const bool showVectorTrim = pinnedOrDetail(supportsVectorTrimOutside, QStringLiteral("vector_trim"));

  m_colorLabel->setVisible(showColor);
  m_colorButton->setVisible(showColor);
  m_sizeLabel->setVisible(showSize);
  m_sizeSlider->setVisible(showSize);
  m_sizeSpin->setVisible(showSize);
  m_opacityLabel->setVisible(showOpacity);
  m_opacitySlider->setVisible(showOpacity);
  m_opacitySpin->setVisible(showOpacity);
  m_hardnessLabel->setVisible(showHardness);
  m_hardnessSlider->setVisible(showHardness);
  m_hardnessSpin->setVisible(showHardness);
  m_flowLabel->setVisible(supportsFlow);
  m_flowSlider->setVisible(supportsFlow);
  m_flowSpin->setVisible(supportsFlow);
  m_spacingLabel->setVisible(m_showDetails && supportsSpacing);
  m_spacingSlider->setVisible(m_showDetails && supportsSpacing);
  m_spacingSpin->setVisible(m_showDetails && supportsSpacing);
  m_brushDynamicsSection->setVisible(supportsFlow || (m_showDetails && supportsSpacing));
  m_antiAliasCheck->setVisible(showAntiAlias);
  m_stabilizationLabel->setVisible(showStabilization);
  m_stabilizationSlider->setVisible(showStabilization);
  m_stabilizationSpin->setVisible(showStabilization);
  m_postCorrectionCheck->setVisible(m_showDetails && supportsPostCorrection);
  m_velocityCorrectionCheck->setVisible(m_showDetails && supportsVelocityCorrection);
  m_correctionSection->setVisible(
      showAntiAlias || showStabilization || (m_showDetails && (supportsPostCorrection || supportsVelocityCorrection)));
  const bool showPressure = m_showDetails && supportsFlow;
  m_pressureSection->setVisible(showPressure);
  m_pressureSizeCheck->setVisible(showPressure);
  m_pressureSizeMinSlider->setVisible(showPressure);
  m_pressureSizeMinSpin->setVisible(showPressure);
  m_pressureOpacityCheck->setVisible(showPressure);
  m_pressureOpacityMinSlider->setVisible(showPressure);
  m_pressureOpacityMinSpin->setVisible(showPressure);
  // 速度感応・テクスチャ・ウェット — ブラシ詳細モードのみ表示
  const bool showBrushAdv = showPressure && m_showDetails;
  m_velocitySection->setVisible(showBrushAdv);
  m_textureSection->setVisible(showBrushAdv);
  m_wetSection->setVisible(showBrushAdv);
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
  m_blendModeCombo->setVisible(showBlend);
  m_buildupModeCheck->setVisible(m_showDetails && showBlend);
  m_eraseModeCheck->setVisible(m_showDetails && supportsEraseMode);
  m_lockAlphaRespectCheck->setVisible(m_showDetails && supportsLockAlpha);
  m_snapAngleLabel->setVisible(supportsSnapAngle);
  m_snapAngleSlider->setVisible(supportsSnapAngle);
  m_snapAngleSpin->setVisible(supportsSnapAngle);
  m_simplifyLabel->setVisible(supportsSimplify);
  m_simplifySlider->setVisible(supportsSimplify);
  m_simplifySpin->setVisible(supportsSimplify);
  m_vectorEraseModeLabel->setVisible(showVectorMode);
  m_vectorEraseModeCombo->setVisible(showVectorMode);
  m_vectorTrimOutsideCheck->setVisible(showVectorTrim);
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
  const bool supportsSelectionFeather = m_controller->currentToolHasProperty(app::ui::ToolPropertyKey::SelectionFeather);
  const bool supportsSelectionAA      = m_controller->currentToolHasProperty(app::ui::ToolPropertyKey::SelectionAntiAlias);
  if (m_selectionFeatherLabel != nullptr) m_selectionFeatherLabel->setVisible(supportsSelectionFeather);
  if (m_selectionFeatherSlider != nullptr) m_selectionFeatherSlider->setVisible(supportsSelectionFeather);
  if (m_selectionFeatherSpin != nullptr) m_selectionFeatherSpin->setVisible(supportsSelectionFeather);
  if (m_selectionAntiAliasCheck != nullptr) m_selectionAntiAliasCheck->setVisible(supportsSelectionAA);
  const bool supportsSelectionOp      = m_controller->currentToolHasProperty(app::ui::ToolPropertyKey::SelectionOp);
  const bool supportsSelectionExpand  = m_controller->currentToolHasProperty(app::ui::ToolPropertyKey::SelectionExpand);
  const bool supportsSelectionGap     = m_controller->currentToolHasProperty(app::ui::ToolPropertyKey::SelectionGapClose);
  const bool supportsSelectionEdge    = m_controller->currentToolHasProperty(app::ui::ToolPropertyKey::SelectionEdgeSnap);
  if (m_selectionOpLabel    != nullptr) m_selectionOpLabel->setVisible(supportsSelectionOp);
  if (m_selOpNewBtn         != nullptr) m_selOpNewBtn->setVisible(supportsSelectionOp);
  if (m_selOpAddBtn         != nullptr) m_selOpAddBtn->setVisible(supportsSelectionOp);
  if (m_selOpSubtractBtn    != nullptr) m_selOpSubtractBtn->setVisible(supportsSelectionOp);
  if (m_selOpIntersectBtn   != nullptr) m_selOpIntersectBtn->setVisible(supportsSelectionOp);
  if (m_selectionExpandLabel!= nullptr) m_selectionExpandLabel->setVisible(supportsSelectionExpand);
  if (m_selectionExpandSpin != nullptr) m_selectionExpandSpin->setVisible(supportsSelectionExpand);
  if (m_selectionGapCloseLabel != nullptr) m_selectionGapCloseLabel->setVisible(supportsSelectionGap);
  if (m_selectionGapCloseSpin  != nullptr) m_selectionGapCloseSpin->setVisible(supportsSelectionGap);
  if (m_selectionEdgeSnapCheck != nullptr) m_selectionEdgeSnapCheck->setVisible(supportsSelectionEdge);
  m_drawingControlSection->setVisible(showBlend || (m_showDetails && (supportsEraseMode || supportsLockAlpha)));
  m_vectorSection->setVisible(
      m_showDetails && (supportsSnapAngle || supportsSimplify || showVectorMode || showVectorTrim));
  m_fillSection->setVisible(
      m_showDetails && (supportsFillThreshold || supportsFillContiguous || supportsFillReferAllLayers || supportsFillGapClose));
  m_selectionSection->setVisible(
      m_showDetails &&
      (supportsSelectionMode || supportsSelectionOp || supportsAutoSelectThreshold || supportsAutoSelectContiguous ||
       supportsAutoSelectReferAllLayers || supportsSelectionFeather || supportsSelectionAA ||
       supportsSelectionExpand || supportsSelectionGap || supportsSelectionEdge));

  const app::bridge::ToolStateViewModel state = m_controller->toolState();
  const QSignalBlocker blocker1(m_sizeSpin);
  const QSignalBlocker blockerSizeSlider(m_sizeSlider);
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
  const QSignalBlocker blockerBuildupMode(m_buildupModeCheck);
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
  const QSignalBlocker blocker42(m_vectorEraseModeCombo);
  const QSignalBlocker blocker43(m_vectorTrimOutsideCheck);
  const QSignalBlocker blocker44(m_pressureSizeCheck);
  const QSignalBlocker blocker45(m_pressureSizeMinSlider);
  const QSignalBlocker blocker46(m_pressureSizeMinSpin);
  const QSignalBlocker blocker47(m_pressureOpacityCheck);
  const QSignalBlocker blocker48(m_pressureOpacityMinSlider);
  const QSignalBlocker blocker49(m_pressureOpacityMinSpin);
  m_sizeSpin->setValue(state.size);
  m_sizeSlider->setValue(std::min(state.size, m_sizeSlider->maximum()));
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
  m_buildupModeCheck->setChecked(state.buildupMode);
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
  if (m_selectionFeatherSlider != nullptr) m_selectionFeatherSlider->setValue(state.selectionFeather);
  if (m_selectionFeatherSpin != nullptr) m_selectionFeatherSpin->setValue(state.selectionFeather);
  if (m_selectionAntiAliasCheck != nullptr) m_selectionAntiAliasCheck->setChecked(state.selectionAntiAlias);
  // op buttons: uncheck all then check active
  if (m_selOpNewBtn != nullptr) {
    m_selOpNewBtn->setChecked(state.selectionOp == core::SelectionOp::New);
    m_selOpAddBtn->setChecked(state.selectionOp == core::SelectionOp::Add);
    m_selOpSubtractBtn->setChecked(state.selectionOp == core::SelectionOp::Subtract);
    m_selOpIntersectBtn->setChecked(state.selectionOp == core::SelectionOp::Intersect);
  }
  if (m_selectionExpandSpin   != nullptr) m_selectionExpandSpin->setValue(state.selectionExpand);
  if (m_selectionGapCloseSpin != nullptr) m_selectionGapCloseSpin->setValue(state.selectionGapClose);
  if (m_selectionEdgeSnapCheck!= nullptr) m_selectionEdgeSnapCheck->setChecked(state.selectionEdgeSnap);
  m_vectorEraseModeCombo->setCurrentIndex(m_vectorEraseModeCombo->findData(static_cast<int>(state.vectorEraseMode)));
  m_vectorTrimOutsideCheck->setChecked(state.vectorTrimOutside);
  m_pressureSizeCheck->setChecked(state.pressureSizeEnabled);
  m_pressureSizeMinSlider->setValue(state.pressureSizeMin);
  m_pressureSizeMinSpin->setValue(state.pressureSizeMin);
  m_pressureOpacityCheck->setChecked(state.pressureOpacityEnabled);
  m_pressureOpacityMinSlider->setValue(state.pressureOpacityMin);
  m_pressureOpacityMinSpin->setValue(state.pressureOpacityMin);
  // 速度感応
  m_velocitySizeCheck->setChecked(state.velocitySize);
  m_velocitySizeMinSlider->setValue(state.velocitySizeMin);
  m_velocityOpacityCheck->setChecked(state.velocityOpacity);
  m_velocityOpacityMinSlider->setValue(state.velocityOpacityMin);
  // テクスチャグレイン
  m_textureGrainCheck->setChecked(state.textureGrain);
  m_textureStrengthSlider->setValue(state.textureStrength);
  m_textureScaleSlider->setValue(state.textureScale);
  // ウェットミックス / スメア
  m_wetMixCheck->setChecked(state.wetMix);
  m_wetMixRateSlider->setValue(state.wetMixRate);
  m_smearCheck->setChecked(state.smear);
  m_smearRateSlider->setValue(state.smearRate);
  updateColorButton();
}

void ToolPropertyPanel::onChooseColor() {
  if (m_controller == nullptr || !m_controller->currentToolSupportsColor()) {
    return;
  }
  const app::bridge::ToolStateViewModel state = m_controller->toolState();
  const QColor picked = QColorDialog::getColor(toQColor(state.color), this, "描画色", QColorDialog::ShowAlphaChannel);
  if (!picked.isValid()) {
    return;
  }
  m_controller->setBrushColor(toCoreColor(picked));
}

void ToolPropertyPanel::onSizeChanged(int size) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsSize()) {
    return;
  }
  if (m_sizeSlider != nullptr) {
    const QSignalBlocker blocker(m_sizeSlider);
    m_sizeSlider->setValue(std::min(size, m_sizeSlider->maximum()));
  }
  m_controller->setBrushSize(size);
}

void ToolPropertyPanel::onSizeSliderChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsSize()) {
    return;
  }
  const QSignalBlocker blocker(m_sizeSpin);
  m_sizeSpin->setValue(value);
  m_controller->setBrushSize(value);
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
  qDebug() << "[ToolPropertyPanel] Flow slider changed:" << value;
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

void ToolPropertyPanel::onVectorEraseModeChanged(int index) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsVectorEraseMode()) {
    return;
  }
  const QVariant value = m_vectorEraseModeCombo->itemData(index);
  if (!value.isValid()) {
    return;
  }
  m_controller->setVectorEraseMode(static_cast<app::ui::VectorEraserMode>(value.toInt()));
}

void ToolPropertyPanel::onVectorTrimOutsideToggled(bool checked) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsVectorTrimOutside()) {
    return;
  }
  m_controller->setVectorTrimOutside(checked);
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

void ToolPropertyPanel::onSelectionFeatherSliderChanged(int value) {
  if (m_controller == nullptr) return;
  if (m_selectionFeatherSpin != nullptr) {
    const QSignalBlocker blocker(m_selectionFeatherSpin);
    m_selectionFeatherSpin->setValue(value);
  }
  m_controller->setSelectionFeather(value);
}

void ToolPropertyPanel::onSelectionFeatherSpinChanged(int value) {
  if (m_controller == nullptr) return;
  if (m_selectionFeatherSlider != nullptr) {
    const QSignalBlocker blocker(m_selectionFeatherSlider);
    m_selectionFeatherSlider->setValue(value);
  }
  m_controller->setSelectionFeather(value);
}

void ToolPropertyPanel::onSelectionAntiAliasToggled(bool checked) {
  if (m_controller == nullptr) return;
  m_controller->setSelectionAntiAlias(checked);
}

void ToolPropertyPanel::onSelectionOpClicked(int op) {
  if (m_controller == nullptr) return;
  static const core::SelectionOp ops[] = {
    core::SelectionOp::New,
    core::SelectionOp::Add,
    core::SelectionOp::Subtract,
    core::SelectionOp::Intersect,
  };
  if (op >= 0 && op < 4) {
    m_controller->setSelectionOp(ops[op]);
  }
}

void ToolPropertyPanel::onSelectionExpandChanged(int value) {
  if (m_controller == nullptr) return;
  m_controller->setSelectionExpand(value);
}

void ToolPropertyPanel::onSelectionGapCloseChanged(int value) {
  if (m_controller == nullptr) return;
  m_controller->setSelectionGapClose(value);
}

void ToolPropertyPanel::onSelectionEdgeSnapToggled(bool checked) {
  if (m_controller == nullptr) return;
  m_controller->setSelectionEdgeSnap(checked);
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

void ToolPropertyPanel::onBuildupModeToggled(bool checked) {
  if (m_controller == nullptr) return;
  m_controller->setBrushBuildupMode(checked);
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

void ToolPropertyPanel::onPressureSizeToggled(bool checked) {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->setPressureSizeEnabled(checked);
}

void ToolPropertyPanel::onPressureSizeMinSliderChanged(int value) {
  if (m_controller == nullptr) {
    return;
  }
  const QSignalBlocker blocker(m_pressureSizeMinSpin);
  m_pressureSizeMinSpin->setValue(value);
  m_controller->setPressureSizeMin(value);
}

void ToolPropertyPanel::onPressureSizeMinSpinChanged(int value) {
  if (m_controller == nullptr) {
    return;
  }
  const QSignalBlocker blocker(m_pressureSizeMinSlider);
  m_pressureSizeMinSlider->setValue(value);
  m_controller->setPressureSizeMin(value);
}

void ToolPropertyPanel::onPressureOpacityToggled(bool checked) {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->setPressureOpacityEnabled(checked);
}

void ToolPropertyPanel::onPressureOpacityMinSliderChanged(int value) {
  if (m_controller == nullptr) {
    return;
  }
  const QSignalBlocker blocker(m_pressureOpacityMinSpin);
  m_pressureOpacityMinSpin->setValue(value);
  m_controller->setPressureOpacityMin(value);
}

void ToolPropertyPanel::onPressureOpacityMinSpinChanged(int value) {
  if (m_controller == nullptr) {
    return;
  }
  const QSignalBlocker blocker(m_pressureOpacityMinSlider);
  m_pressureOpacityMinSlider->setValue(value);
  m_controller->setPressureOpacityMin(value);
}

// ── 速度感応スロット ─────────────────────────────────────────────────────────
void ToolPropertyPanel::onVelocitySizeToggled(bool checked) {
  if (m_controller) m_controller->setVelocitySize(checked);
}
void ToolPropertyPanel::onVelocitySizeMinSliderChanged(int value) {
  if (m_controller) m_controller->setVelocitySizeMin(value);
}
void ToolPropertyPanel::onVelocityOpacityToggled(bool checked) {
  if (m_controller) m_controller->setVelocityOpacity(checked);
}
void ToolPropertyPanel::onVelocityOpacityMinSliderChanged(int value) {
  if (m_controller) m_controller->setVelocityOpacityMin(value);
}
// ── テクスチャグレインスロット ───────────────────────────────────────────────
void ToolPropertyPanel::onTextureGrainToggled(bool checked) {
  if (m_controller) m_controller->setTextureGrain(checked);
}
void ToolPropertyPanel::onTextureStrengthSliderChanged(int value) {
  if (m_controller) m_controller->setTextureStrength(value);
}
void ToolPropertyPanel::onTextureScaleSliderChanged(int value) {
  if (m_controller) m_controller->setTextureScale(value);
}
// ── ウェットミックス / スメアスロット ─────────────────────────────────────
void ToolPropertyPanel::onWetMixToggled(bool checked) {
  if (m_controller) m_controller->setWetMix(checked);
}
void ToolPropertyPanel::onWetMixRateSliderChanged(int value) {
  if (m_controller) m_controller->setWetMixRate(value);
}
void ToolPropertyPanel::onSmearToggled(bool checked) {
  if (m_controller) m_controller->setSmear(checked);
}
void ToolPropertyPanel::onSmearRateSliderChanged(int value) {
  if (m_controller) m_controller->setSmearRate(value);
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

  m_colorButton->setText(QString("色 %1").arg(hex));
  m_colorButton->setStyleSheet(
      QString("QPushButton { background-color: rgba(%1, %2, %3, %4); color: %5; border: 1px solid #555; padding: 2px 4px; }")
          .arg(color.red())
          .arg(color.green())
          .arg(color.blue())
          .arg(color.alpha())
          .arg(textColor));
}

std::tuple<QLabel*, QSlider*, QSpinBox*> ToolPropertyPanel::createLabeledSlider(
    const QString& label, int min, int max, int value)
{
  auto* lbl = new QLabel(label, this);
  auto* slider = new QSlider(Qt::Horizontal, this);
  auto* spin = new QSpinBox(this);
  slider->setRange(min, max);
  slider->setValue(value);
  spin->setRange(min, max);
  spin->setValue(value);
  return {lbl, slider, spin};
}

void ToolPropertyPanel::appendLabeledRow(
    QVBoxLayout* layout, QLabel* label, QSlider* slider, QSpinBox* spin)
{
  auto* row = new QHBoxLayout();
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(4);
  row->setProperty("responsiveRow", true);
  row->addWidget(slider, 1);
  row->addWidget(spin);
  layout->addWidget(label);
  layout->addLayout(row);
}

} // namespace app::panels

