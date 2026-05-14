#include "app/panels/ToolPropertyPanel.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include <QColorDialog>
#include <QCheckBox>
#include <QComboBox>
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
#include <QListWidget>

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
      m_angleLabel(new QLabel("角度", this)),
      m_roundnessLabel(new QLabel("真円率", this)),
      m_taperStartLabel(new QLabel("入り抜き（始点）", this)),
      m_taperEndLabel(new QLabel("入り抜き（終点）", this)),
      m_snapAngleLabel(new QLabel("角度スナップ", this)),
      m_simplifyLabel(new QLabel("単純化", this)),
      m_vectorEraseModeLabel(new QLabel("ベクター消去", this)),
      m_fillThresholdLabel(new QLabel("しきい値", this)),
      m_fillGapCloseLabel(new QLabel("隙間閉じ", this)),
      m_selectionModeLabel(new QLabel("選択モード", this)),
      m_autoSelectThresholdLabel(new QLabel("自動選択しきい値", this)),
      m_colorButton(new QPushButton("色を選択", this)),
      m_sizeSpin(new QSpinBox(this)),
      m_textBoldBtn(new QPushButton("B", this)),
      m_textItalicBtn(new QPushButton("I", this)),
      m_textUnderlineBtn(new QPushButton("U", this)),
      m_textStrikeOutBtn(new QPushButton("S", this)),
      m_textVerticalBtn(new QPushButton("縦", this)),
      m_vertDirRightBtn(new QPushButton("←列", this)),
      m_vertDirLeftBtn(new QPushButton("列→", this)),
      m_textLineSpacingLabel(new QLabel("行間", this)),
      m_textLineSpacingSlider(new QSlider(Qt::Horizontal, this)),
      m_textLineSpacingSpin(new QSpinBox(this)),
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
      m_blendModeCombo(new QComboBox(this)),
      m_eraseModeCheck(new QCheckBox("消しゴムモード", this)),
      m_lockAlphaRespectCheck(new QCheckBox("透明保護を尊重", this)) {
  auto* hostLayout = new QVBoxLayout(this);
  hostLayout->setContentsMargins(0, 0, 0, 0);
  hostLayout->setSpacing(0);

  m_guideLabel->setWordWrap(true);
  m_compatibilityLabel->setWordWrap(true);
  m_guideLabel->setStyleSheet("color: #aeb8c8; font-size: 10px;");
  m_compatibilityLabel->setStyleSheet("color: #f3bf58; font-weight: 600;");
  m_guideLabel->setVisible(false);
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

  m_shapeTypeCombo->addItem("円", static_cast<int>(core::BrushShapeType::Circle));
  m_shapeTypeCombo->addItem("四角", static_cast<int>(core::BrushShapeType::Square));

  m_blendModeCombo->addItem("通常", static_cast<int>(core::BlendMode::Normal));
  m_blendModeCombo->addItem("乗算", static_cast<int>(core::BlendMode::Multiply));
  m_blendModeCombo->addItem("加算", static_cast<int>(core::BlendMode::Add));

  m_selectionModeCombo->addItem("矩形", static_cast<int>(app::ui::SelectionMode::Rectangle));
  m_selectionModeCombo->addItem("なげなわ", static_cast<int>(app::ui::SelectionMode::Lasso));
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
  titleLayout->setContentsMargins(3, 3, 3, 3);
  titleLayout->setSpacing(2);
  auto* titleActions = new QHBoxLayout();
  titleActions->setContentsMargins(0, 0, 0, 0);
  titleActions->setSpacing(4);
  m_detailToggleButton->setMinimumHeight(20);
  m_pinConfigButton->setMinimumHeight(20);
  m_detailToggleButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_pinConfigButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  titleActions->addWidget(m_detailToggleButton);
  titleActions->addWidget(m_pinConfigButton);
  titleLayout->addWidget(m_toolNameLabel);
  titleLayout->addLayout(titleActions);
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

  basicLayout->addWidget(m_colorLabel);
  basicLayout->addWidget(m_colorButton);
  basicLayout->addWidget(m_sizeLabel);
  basicLayout->addWidget(m_sizeSpin);
  basicLayout->addWidget(m_opacityLabel);
  basicLayout->addLayout(opacityRow);
  basicLayout->addWidget(m_hardnessLabel);
  basicLayout->addLayout(hardnessRow);
  contentLayout->addWidget(basicGroup);

  auto* dynamicsGroup = new QGroupBox("ブラシ特性", m_contentWidget);
  auto* dynamicsLayout = new QVBoxLayout(dynamicsGroup);
  dynamicsLayout->setContentsMargins(4, 4, 4, 4);
  dynamicsLayout->setSpacing(4);

  auto* flowRow = new QHBoxLayout();
  markResponsiveRow(flowRow);
  flowRow->addWidget(m_flowSlider, 1);
  flowRow->addWidget(m_flowSpin);

  auto* spacingRow = new QHBoxLayout();
  markResponsiveRow(spacingRow);
  spacingRow->addWidget(m_spacingSlider, 1);
  spacingRow->addWidget(m_spacingSpin);

  dynamicsLayout->addWidget(m_flowLabel);
  dynamicsLayout->addLayout(flowRow);
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

  auto* shapeGroup = new QGroupBox("形状", m_contentWidget);
  auto* shapeLayout = new QVBoxLayout(shapeGroup);
  shapeLayout->setContentsMargins(4, 4, 4, 4);
  shapeLayout->setSpacing(4);
  auto* angleRow = new QHBoxLayout();
  markResponsiveRow(angleRow);
  angleRow->addWidget(m_angleSlider, 1);
  angleRow->addWidget(m_angleSpin);
  auto* roundnessRow = new QHBoxLayout();
  markResponsiveRow(roundnessRow);
  roundnessRow->addWidget(m_roundnessSlider, 1);
  roundnessRow->addWidget(m_roundnessSpin);
  auto* taperStartRow = new QHBoxLayout();
  markResponsiveRow(taperStartRow);
  taperStartRow->addWidget(m_taperStartSlider, 1);
  taperStartRow->addWidget(m_taperStartSpin);
  auto* taperEndRow = new QHBoxLayout();
  markResponsiveRow(taperEndRow);
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

  auto* drawControlGroup = new QGroupBox("描画制御", m_contentWidget);
  auto* drawControlLayout = new QVBoxLayout(drawControlGroup);
  drawControlLayout->setContentsMargins(4, 4, 4, 4);
  drawControlLayout->setSpacing(4);
  drawControlLayout->addWidget(m_blendModeCombo);
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

  auto* selectionGroup = new QGroupBox("選択", m_contentWidget);
  auto* selectionLayout = new QVBoxLayout(selectionGroup);
  selectionLayout->setContentsMargins(4, 4, 4, 4);
  selectionLayout->setSpacing(4);
  auto* autoSelectThresholdRow = new QHBoxLayout();
  markResponsiveRow(autoSelectThresholdRow);
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

  // テキスト書式ボタンのスタイル設定
  {
    const QString btnStyle =
        "QPushButton { border: 1px solid #555; border-radius: 3px; background: #2d2d2d;"
        " color: #ccc; min-width: 26px; max-width: 26px; min-height: 26px; max-height: 26px; }"
        "QPushButton:checked { background: #3a6ea8; border-color: #5090d0; color: #fff; }"
        "QPushButton:hover { background: #3d3d3d; }";

    QFont boldFont = m_textBoldBtn->font();
    boldFont.setBold(true);
    boldFont.setPointSize(10);
    m_textBoldBtn->setFont(boldFont);
    m_textBoldBtn->setCheckable(true);
    m_textBoldBtn->setToolTip("太字");
    m_textBoldBtn->setStyleSheet(btnStyle);

    QFont italicFont = m_textItalicBtn->font();
    italicFont.setItalic(true);
    italicFont.setPointSize(10);
    m_textItalicBtn->setFont(italicFont);
    m_textItalicBtn->setCheckable(true);
    m_textItalicBtn->setToolTip("斜体");
    m_textItalicBtn->setStyleSheet(btnStyle);

    QFont underlineFont = m_textUnderlineBtn->font();
    underlineFont.setUnderline(true);
    underlineFont.setPointSize(10);
    m_textUnderlineBtn->setFont(underlineFont);
    m_textUnderlineBtn->setCheckable(true);
    m_textUnderlineBtn->setToolTip("下線");
    m_textUnderlineBtn->setStyleSheet(btnStyle);

    QFont strikeFont = m_textStrikeOutBtn->font();
    strikeFont.setStrikeOut(true);
    strikeFont.setPointSize(10);
    m_textStrikeOutBtn->setFont(strikeFont);
    m_textStrikeOutBtn->setCheckable(true);
    m_textStrikeOutBtn->setToolTip("取り消し線");
    m_textStrikeOutBtn->setStyleSheet(btnStyle);

    m_textVerticalBtn->setCheckable(true);
    m_textVerticalBtn->setToolTip("縦書き");
    m_textVerticalBtn->setStyleSheet(btnStyle);

    m_vertDirRightBtn->setCheckable(true);
    m_vertDirRightBtn->setToolTip("RTL: 列が右から左へ（伝統的縦書き）");
    m_vertDirRightBtn->setStyleSheet(btnStyle);
    m_vertDirLeftBtn->setCheckable(true);
    m_vertDirLeftBtn->setToolTip("LTR: 列が左から右へ");
    m_vertDirLeftBtn->setStyleSheet(btnStyle);

    // 行間: 50〜300 → 0.5x〜3.0x, default 100 = 1.0x
    m_textLineSpacingSlider->setRange(50, 300);
    m_textLineSpacingSlider->setValue(100);
    m_textLineSpacingSpin->setRange(50, 300);
    m_textLineSpacingSpin->setValue(100);
    m_textLineSpacingSpin->setSuffix("%");
  }

  auto* textGroup = new QGroupBox("テキスト書式", m_contentWidget);
  auto* textGroupLayout = new QVBoxLayout(textGroup);
  textGroupLayout->setContentsMargins(4, 4, 4, 4);
  textGroupLayout->setSpacing(4);
  // 1行目: B/I/U/S/縦 アイコンボタン
  auto* textBtnRow = new QHBoxLayout();
  textBtnRow->setSpacing(4);
  textBtnRow->addWidget(m_textBoldBtn);
  textBtnRow->addWidget(m_textItalicBtn);
  textBtnRow->addWidget(m_textUnderlineBtn);
  textBtnRow->addWidget(m_textStrikeOutBtn);
  textBtnRow->addWidget(m_textVerticalBtn);
  textBtnRow->addStretch(1);
  textGroupLayout->addLayout(textBtnRow);
  // 2行目: 縦書き方向（text_verticalツールのときのみ表示）
  auto* vertDirRow = new QHBoxLayout();
  vertDirRow->setSpacing(4);
  vertDirRow->addWidget(m_vertDirRightBtn);
  vertDirRow->addWidget(m_vertDirLeftBtn);
  vertDirRow->addStretch(1);
  textGroupLayout->addLayout(vertDirRow);
  // 3行目: 行間スライダー
  textGroupLayout->addWidget(m_textLineSpacingLabel);
  auto* lsRow = new QHBoxLayout();
  markResponsiveRow(lsRow);
  lsRow->addWidget(m_textLineSpacingSlider, 1);
  lsRow->addWidget(m_textLineSpacingSpin);
  textGroupLayout->addLayout(lsRow);
  contentLayout->addWidget(textGroup);
  m_textSection = textGroup;

  contentLayout->addStretch(1);

  m_scrollArea->setWidgetResizable(true);
  m_scrollArea->setFrameShape(QFrame::NoFrame);
  m_scrollArea->setWidget(m_contentWidget);
  hostLayout->addWidget(m_scrollArea);

  connect(m_detailToggleButton, &QPushButton::clicked, this, &ToolPropertyPanel::onToggleDetailRequested);
  connect(m_pinConfigButton, &QPushButton::clicked, this, &ToolPropertyPanel::onConfigurePinnedRequested);
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
  connect(m_vectorEraseModeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &ToolPropertyPanel::onVectorEraseModeChanged);
  connect(m_vectorTrimOutsideCheck, &QCheckBox::toggled, this, &ToolPropertyPanel::onVectorTrimOutsideToggled);
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
  connect(m_textBoldBtn, &QPushButton::toggled, this, &ToolPropertyPanel::onTextBoldToggled);
  connect(m_textItalicBtn, &QPushButton::toggled, this, &ToolPropertyPanel::onTextItalicToggled);
  connect(m_textUnderlineBtn, &QPushButton::toggled, this, &ToolPropertyPanel::onTextUnderlineToggled);
  connect(m_textStrikeOutBtn, &QPushButton::toggled, this, &ToolPropertyPanel::onTextStrikeOutToggled);
  connect(m_textVerticalBtn, &QPushButton::clicked, this, [this]() {
    if (m_controller) {
      m_controller->textEditorToggleVertical();
    }
  });
  connect(m_vertDirRightBtn, &QPushButton::clicked, this, [this]() {
    if (m_controller) {
      m_controller->textEditorSetVerticalRTL(true);
      const QSignalBlocker b1(m_vertDirRightBtn);
      const QSignalBlocker b2(m_vertDirLeftBtn);
      m_vertDirRightBtn->setChecked(true);
      m_vertDirLeftBtn->setChecked(false);
    }
  });
  connect(m_vertDirLeftBtn, &QPushButton::clicked, this, [this]() {
    if (m_controller) {
      m_controller->textEditorSetVerticalRTL(false);
      const QSignalBlocker b1(m_vertDirRightBtn);
      const QSignalBlocker b2(m_vertDirLeftBtn);
      m_vertDirRightBtn->setChecked(false);
      m_vertDirLeftBtn->setChecked(true);
    }
  });
  connect(m_textLineSpacingSlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onTextLineSpacingChanged);
  connect(m_textLineSpacingSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onTextLineSpacingChanged);
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
  connect(m_controller, &app::bridge::AppController::overlayChanged, this, &ToolPropertyPanel::refreshTextStyleButtons);
  connect(m_controller, &app::bridge::AppController::textCaretChanged, this, &ToolPropertyPanel::refreshTextStyleButtons);
  loadPinnedForCurrentTool();
  refreshDetailToggleText();
  refreshFromController();
}

void ToolPropertyPanel::setDetailMode(bool enabled) {
  m_showDetails = enabled;
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
  m_detailToggleButton->setText(m_showDetails ? QStringLiteral("詳細を隠す") : QStringLiteral("詳細を表示"));
}

void ToolPropertyPanel::onToggleDetailRequested() {
  if (m_controller == nullptr) {
    return;
  }
  QDialog dialog(this);
  dialog.setWindowTitle(QStringLiteral("サブツール詳細"));
  dialog.resize(980, 680);
  auto* layout = new QHBoxLayout(&dialog);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(8);

  auto* categoryList = new QListWidget(&dialog);
  categoryList->addItems({
      QStringLiteral("基本"),
      QStringLiteral("ブラシ特性"),
      QStringLiteral("補正"),
      QStringLiteral("形状"),
      QStringLiteral("描画制御"),
      QStringLiteral("ベクター"),
      QStringLiteral("塗りつぶし"),
      QStringLiteral("選択")});
  categoryList->setFixedWidth(170);
  categoryList->setCurrentRow(0);

  auto* detailPanel = new ToolPropertyPanel(&dialog);
  detailPanel->setController(m_controller);
  detailPanel->setDetailMode(true);

  layout->addWidget(categoryList);
  layout->addWidget(detailPanel, 1);

  QObject::connect(categoryList, &QListWidget::currentTextChanged, &dialog, [detailPanel](const QString& name) {
    const auto boxes = detailPanel->findChildren<QGroupBox*>();
    for (QGroupBox* box : boxes) {
      if (box == nullptr || box->title() != name) {
        continue;
      }
      if (auto* scroll = detailPanel->findChild<QScrollArea*>()) {
        scroll->ensureWidgetVisible(box, 0, 8);
      }
      break;
    }
  });

  dialog.exec();
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

void ToolPropertyPanel::refreshTextStyleButtons() {
  if (m_controller == nullptr || m_textSection == nullptr) return;
  const std::string refreshSubTool = m_controller->currentSubToolId();
  const bool isTextTool = m_controller->currentTool() == core::ToolKind::MoveLayer
                          && (refreshSubTool == "text_basic" || refreshSubTool == "text_vertical");
  if (!isTextTool) return;
  const QSignalBlocker b1(m_textBoldBtn);
  const QSignalBlocker b2(m_textItalicBtn);
  const QSignalBlocker b3(m_textUnderlineBtn);
  const QSignalBlocker b4(m_textStrikeOutBtn);
  m_textBoldBtn->setChecked(m_controller->textEditorAtCaretBold());
  m_textItalicBtn->setChecked(m_controller->textEditorAtCaretItalic());
  m_textUnderlineBtn->setChecked(m_controller->textEditorAtCaretUnderline());
  m_textStrikeOutBtn->setChecked(m_controller->textEditorAtCaretStrikeOut());
  {
    // 縦書きボタン: text_vertical ツールでは非表示（常に縦書きなので不要）
    const QSignalBlocker bv(m_textVerticalBtn);
    const bool isVerticalTool = (refreshSubTool == "text_vertical");
    m_textVerticalBtn->setChecked(isVerticalTool || m_controller->textEditorIsVertical());
    m_textVerticalBtn->setEnabled(!isVerticalTool);
    m_textVerticalBtn->setVisible(!isVerticalTool);
    // 方向ボタン: text_vertical ツールのときのみ表示
    const bool rtl = m_controller->textEditorGetVerticalRTL();
    {
      const QSignalBlocker br(m_vertDirRightBtn);
      const QSignalBlocker bl(m_vertDirLeftBtn);
      m_vertDirRightBtn->setChecked(rtl);
      m_vertDirLeftBtn->setChecked(!rtl);
      m_vertDirRightBtn->setVisible(isVerticalTool);
      m_vertDirLeftBtn->setVisible(isVerticalTool);
    }
  }
  {
    const QSignalBlocker b1(m_textLineSpacingSlider);
    const QSignalBlocker b2(m_textLineSpacingSpin);
    const int lsVal = static_cast<int>(m_controller->textEditorGetLineSpacing() * 100.0f);
    m_textLineSpacingSlider->setValue(lsVal);
    m_textLineSpacingSpin->setValue(lsVal);
  }
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

  const QString effectiveToolName = QString::fromStdString(m_controller->currentToolDisplayName());
  m_toolNameLabel->setText(QString("ツール: %1").arg(
      effectiveToolName.isEmpty() ? toolNameJa(m_controller->currentTool()) : effectiveToolName));
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
  m_sizeLabel->setText((supportsSnapAngle || supportsSimplify) ? "Stroke Width" : "Size");
  const bool showColor = pinnedOrDetail(supportsColor, QStringLiteral("color"));
  const bool showSize = pinnedOrDetail(supportsSize, QStringLiteral("size"));
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
  m_sizeSpin->setVisible(showSize);
  m_opacityLabel->setVisible(showOpacity);
  m_opacitySlider->setVisible(showOpacity);
  m_opacitySpin->setVisible(showOpacity);
  m_hardnessLabel->setVisible(showHardness);
  m_hardnessSlider->setVisible(showHardness);
  m_hardnessSpin->setVisible(showHardness);
  m_flowLabel->setVisible(m_showDetails && supportsFlow);
  m_flowSlider->setVisible(m_showDetails && supportsFlow);
  m_flowSpin->setVisible(m_showDetails && supportsFlow);
  m_spacingLabel->setVisible(m_showDetails && supportsSpacing);
  m_spacingSlider->setVisible(m_showDetails && supportsSpacing);
  m_spacingSpin->setVisible(m_showDetails && supportsSpacing);
  m_brushDynamicsSection->setVisible(m_showDetails && (supportsFlow || supportsSpacing));
  m_antiAliasCheck->setVisible(showAntiAlias);
  m_stabilizationLabel->setVisible(showStabilization);
  m_stabilizationSlider->setVisible(showStabilization);
  m_stabilizationSpin->setVisible(showStabilization);
  m_postCorrectionCheck->setVisible(m_showDetails && supportsPostCorrection);
  m_velocityCorrectionCheck->setVisible(m_showDetails && supportsVelocityCorrection);
  m_correctionSection->setVisible(
      showAntiAlias || showStabilization || (m_showDetails && (supportsPostCorrection || supportsVelocityCorrection)));
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
      m_showDetails && (supportsShape || supportsAngle || supportsRoundness || supportsTaperStart || supportsTaperEnd));
  m_blendModeCombo->setVisible(showBlend);
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
  m_drawingControlSection->setVisible(showBlend || (m_showDetails && (supportsEraseMode || supportsLockAlpha)));
  m_vectorSection->setVisible(
      m_showDetails && (supportsSnapAngle || supportsSimplify || showVectorMode || showVectorTrim));
  m_fillSection->setVisible(
      m_showDetails && (supportsFillThreshold || supportsFillContiguous || supportsFillReferAllLayers || supportsFillGapClose));
  m_selectionSection->setVisible(
      m_showDetails &&
      (supportsSelectionMode || supportsAutoSelectThreshold || supportsAutoSelectContiguous ||
       supportsAutoSelectReferAllLayers));
  const std::string refreshFromSubTool = m_controller->currentSubToolId();
  const bool isTextTool = m_controller->currentTool() == core::ToolKind::MoveLayer
                          && (refreshFromSubTool == "text_basic" || refreshFromSubTool == "text_vertical");
  m_textSection->setVisible(isTextTool);

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
  const QSignalBlocker blocker42(m_vectorEraseModeCombo);
  const QSignalBlocker blocker43(m_vectorTrimOutsideCheck);
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
  m_vectorEraseModeCombo->setCurrentIndex(m_vectorEraseModeCombo->findData(static_cast<int>(state.vectorEraseMode)));
  m_vectorTrimOutsideCheck->setChecked(state.vectorTrimOutside);
  updateColorButton();
  if (isTextTool) {
    const QSignalBlocker b1(m_textBoldBtn);
    const QSignalBlocker b2(m_textItalicBtn);
    const QSignalBlocker b3(m_textUnderlineBtn);
    const QSignalBlocker b4(m_textStrikeOutBtn);
    m_textBoldBtn->setChecked(m_controller->textEditorAtCaretBold());
    m_textItalicBtn->setChecked(m_controller->textEditorAtCaretItalic());
    m_textUnderlineBtn->setChecked(m_controller->textEditorAtCaretUnderline());
    m_textStrikeOutBtn->setChecked(m_controller->textEditorAtCaretStrikeOut());
    {
      const QSignalBlocker bv(m_textVerticalBtn);
      const bool isVerticalTool = (m_controller->currentSubToolId() == "text_vertical");
      m_textVerticalBtn->setChecked(isVerticalTool || m_controller->textEditorIsVertical());
      m_textVerticalBtn->setVisible(!isVerticalTool);
      const bool rtl = m_controller->textEditorGetVerticalRTL();
      const QSignalBlocker br(m_vertDirRightBtn);
      const QSignalBlocker bl(m_vertDirLeftBtn);
      m_vertDirRightBtn->setChecked(rtl);
      m_vertDirLeftBtn->setChecked(!rtl);
      m_vertDirRightBtn->setVisible(isVerticalTool);
      m_vertDirLeftBtn->setVisible(isVerticalTool);
    }
    {
      const QSignalBlocker b1(m_textLineSpacingSlider);
      const QSignalBlocker b2(m_textLineSpacingSpin);
      const int lsVal = static_cast<int>(m_controller->textEditorGetLineSpacing() * 100.0f);
      m_textLineSpacingSlider->setValue(lsVal);
      m_textLineSpacingSpin->setValue(lsVal);
    }
  }
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
  m_controller->setBrushSize(size);
}

void ToolPropertyPanel::onTextBoldToggled(bool checked) {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->setSelectedTextBold(checked);
}

void ToolPropertyPanel::onTextItalicToggled(bool checked) {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->setSelectedTextItalic(checked);
}

void ToolPropertyPanel::onTextUnderlineToggled(bool checked) {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->setSelectedTextUnderline(checked);
}

void ToolPropertyPanel::onTextStrikeOutToggled(bool checked) {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->setSelectedTextStrikeOut(checked);
}

void ToolPropertyPanel::onTextLineSpacingChanged(int value) {
  if (m_controller == nullptr) return;
  {
    const QSignalBlocker b1(m_textLineSpacingSlider);
    const QSignalBlocker b2(m_textLineSpacingSpin);
    m_textLineSpacingSlider->setValue(value);
    m_textLineSpacingSpin->setValue(value);
  }
  m_controller->textEditorSetLineSpacing(static_cast<float>(value) / 100.0f);
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

  m_colorButton->setText(QString("色 %1").arg(hex));
  m_colorButton->setStyleSheet(
      QString("QPushButton { background-color: rgba(%1, %2, %3, %4); color: %5; border: 1px solid #555; padding: 2px 4px; }")
          .arg(color.red())
          .arg(color.green())
          .arg(color.blue())
          .arg(color.alpha())
          .arg(textColor));
}

} // namespace app::panels

