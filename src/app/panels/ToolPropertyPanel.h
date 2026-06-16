#pragma once

#include <tuple>

#include <QWidget>
#include <QSet>

class QHBoxLayout;
class QLabel;
class QPushButton;
class QResizeEvent;
class QSpinBox;
class QSlider;
class QScrollArea;
class QVBoxLayout;
class QWidget;
class QCheckBox;
class QComboBox;

namespace app::bridge {
class AppController;
}

namespace app::panels {

class ToolPropertyPanel : public QWidget {
  Q_OBJECT

public:
  explicit ToolPropertyPanel(QWidget* parent = nullptr);
  void setController(app::bridge::AppController* controller);

protected:
  void resizeEvent(QResizeEvent* event) override;

private slots:
  void refreshFromController();
  void onChooseColor();
  void onSizeChanged(int size);
  void onSizeSliderChanged(int value);
  void onOpacitySliderChanged(int value);
  void onOpacitySpinChanged(int value);
  void onHardnessSliderChanged(int value);
  void onHardnessSpinChanged(int value);
  void onFlowSliderChanged(int value);
  void onFlowSpinChanged(int value);
  void onSpacingSliderChanged(int value);
  void onSpacingSpinChanged(int value);
  void onAntiAliasToggled(bool checked);
  void onStabilizationSliderChanged(int value);
  void onStabilizationSpinChanged(int value);
  void onPostCorrectionToggled(bool checked);
  void onVelocityCorrectionToggled(bool checked);
  void onShapeTypeChanged(int index);
  void onAngleSliderChanged(int value);
  void onAngleSpinChanged(int value);
  void onRoundnessSliderChanged(int value);
  void onRoundnessSpinChanged(int value);
  void onTaperStartSliderChanged(int value);
  void onTaperStartSpinChanged(int value);
  void onTaperEndSliderChanged(int value);
  void onTaperEndSpinChanged(int value);
  void onSnapAngleSliderChanged(int value);
  void onSnapAngleSpinChanged(int value);
  void onSimplifySliderChanged(int value);
  void onSimplifySpinChanged(int value);
  void onVectorEraseModeChanged(int index);
  void onVectorTrimOutsideToggled(bool checked);
  void onFillThresholdSliderChanged(int value);
  void onFillThresholdSpinChanged(int value);
  void onFillContiguousToggled(bool checked);
  void onFillReferAllLayersToggled(bool checked);
  void onFillGapCloseSliderChanged(int value);
  void onFillGapCloseSpinChanged(int value);
  void onSelectionModeChanged(int index);
  void onAutoSelectThresholdSliderChanged(int value);
  void onAutoSelectThresholdSpinChanged(int value);
  void onAutoSelectContiguousToggled(bool checked);
  void onAutoSelectReferAllLayersToggled(bool checked);
  void onAiGranularityChanged(int index);
  void onRotoBrushFgClicked();
  void onRotoBrushBgClicked();
  void onRotoBrushClearClicked();
  void onRotoBrushConfirmClicked();
  void onRotoBrushRadiusChanged(int value);
  void onAiThresholdChanged(int value);
  void onVectorApproxChanged(int value);
  void onExpandPixelsChanged(int value);
  void onSelectionFeatherSliderChanged(int value);
  void onSelectionFeatherSpinChanged(int value);
  void onSelectionAntiAliasToggled(bool checked);
  void onSelectionOpClicked(int op);
  void onSelectionExpandChanged(int value);
  void onSelectionGapCloseChanged(int value);
  void onSelectionEdgeSnapToggled(bool checked);
  void onBlendModeChanged(int index);
  void onBuildupModeToggled(bool checked);
  void onEraseModeToggled(bool checked);
  void onLockAlphaRespectToggled(bool checked);
  void onToggleDetailRequested();
  void onConfigurePinnedRequested();
  void onPressureSizeToggled(bool checked);
  void onPressureSizeMinSliderChanged(int value);
  void onPressureSizeMinSpinChanged(int value);
  void onPressureOpacityToggled(bool checked);
  void onPressureOpacityMinSliderChanged(int value);
  void onPressureOpacityMinSpinChanged(int value);
  // 速度感応
  void onVelocitySizeToggled(bool checked);
  void onVelocitySizeMinSliderChanged(int value);
  void onVelocityOpacityToggled(bool checked);
  void onVelocityOpacityMinSliderChanged(int value);
  // テクスチャグレイン
  void onTextureGrainToggled(bool checked);
  void onTextureStrengthSliderChanged(int value);
  void onTextureScaleSliderChanged(int value);
  // ウェットミックス / スメア
  void onWetMixToggled(bool checked);
  void onWetMixRateSliderChanged(int value);
  void onSmearToggled(bool checked);
  void onSmearRateSliderChanged(int value);
  // Dab 散布 / 角度ジッター / 粒子数 (OSS 吸収改善)
  void onScatterToggled(bool checked);
  void onScatterAmountSliderChanged(int value);
  void onAngleJitterToggled(bool checked);
  void onAngleJitterAmountSliderChanged(int value);
  void onDabCountSliderChanged(int value);
  // メッシュ変形セクション
  void onMeshDeformRowsChanged(int value);
  void onMeshDeformColsChanged(int value);
  void onMeshDeformModeChanged(int index);
  void onMeshDeformGeneratorChanged(int index);
  void onMeshDeformRegenerateClicked();
  void onMeshDeformConfirmClicked();
  void onMeshDeformCancelClicked();

private:
  void applyResponsiveLayout();
  void updateColorButton();
  QString currentToolSettingsKey() const;
  bool isPinned(const QString& key) const;
  void loadPinnedForCurrentTool();
  void savePinnedForCurrentTool() const;
  void refreshDetailToggleText();
  std::tuple<QLabel*, QSlider*, QSpinBox*> createLabeledSlider(
      const QString& label, int min, int max, int value);
  void appendLabeledRow(QVBoxLayout* layout, QLabel* label, QSlider* slider, QSpinBox* spin);

  app::bridge::AppController* m_controller {nullptr};
  QScrollArea* m_scrollArea {nullptr};
  QWidget* m_contentWidget {nullptr};
  QWidget* m_brushDynamicsSection {nullptr};
  QWidget* m_correctionSection {nullptr};
  QWidget* m_shapeSection {nullptr};
  QWidget* m_drawingControlSection {nullptr};
  QWidget* m_vectorSection {nullptr};
  QWidget* m_fillSection {nullptr};
  QWidget* m_selectionSection {nullptr};
  QLabel* m_toolNameLabel {nullptr};
  QLabel* m_guideLabel {nullptr};
  QLabel* m_compatibilityLabel {nullptr};
  QPushButton* m_detailToggleButton {nullptr};
  QPushButton* m_pinConfigButton {nullptr};
  QLabel* m_colorLabel {nullptr};
  QLabel* m_sizeLabel {nullptr};
  QSlider* m_sizeSlider {nullptr};
  QLabel* m_opacityLabel {nullptr};
  QLabel* m_hardnessLabel {nullptr};
  QLabel* m_flowLabel {nullptr};
  QLabel* m_spacingLabel {nullptr};
  QLabel* m_stabilizationLabel {nullptr};
  QLabel* m_angleLabel {nullptr};
  QLabel* m_roundnessLabel {nullptr};
  QLabel* m_taperStartLabel {nullptr};
  QLabel* m_taperEndLabel {nullptr};
  QLabel* m_snapAngleLabel {nullptr};
  QLabel* m_simplifyLabel {nullptr};
  QLabel* m_vectorEraseModeLabel {nullptr};
  QLabel* m_fillThresholdLabel {nullptr};
  QLabel* m_fillGapCloseLabel {nullptr};
  QLabel* m_selectionModeLabel {nullptr};
  QLabel* m_autoSelectThresholdLabel {nullptr};
  QLabel* m_aiGranularityLabel {nullptr};
  QLabel* m_selectionFeatherLabel {nullptr};
  QPushButton* m_colorButton {nullptr};
  QSpinBox* m_sizeSpin {nullptr};
  QSlider* m_opacitySlider {nullptr};
  QSpinBox* m_opacitySpin {nullptr};
  QSlider* m_hardnessSlider {nullptr};
  QSpinBox* m_hardnessSpin {nullptr};
  QSlider* m_flowSlider {nullptr};
  QSpinBox* m_flowSpin {nullptr};
  QSlider* m_spacingSlider {nullptr};
  QSpinBox* m_spacingSpin {nullptr};
  QCheckBox* m_antiAliasCheck {nullptr};
  QSlider* m_stabilizationSlider {nullptr};
  QSpinBox* m_stabilizationSpin {nullptr};
  QCheckBox* m_postCorrectionCheck {nullptr};
  QCheckBox* m_velocityCorrectionCheck {nullptr};
  QComboBox* m_shapeTypeCombo {nullptr};
  QSlider* m_angleSlider {nullptr};
  QSpinBox* m_angleSpin {nullptr};
  QSlider* m_roundnessSlider {nullptr};
  QSpinBox* m_roundnessSpin {nullptr};
  QSlider* m_taperStartSlider {nullptr};
  QSpinBox* m_taperStartSpin {nullptr};
  QSlider* m_taperEndSlider {nullptr};
  QSpinBox* m_taperEndSpin {nullptr};
  QSlider* m_snapAngleSlider {nullptr};
  QSpinBox* m_snapAngleSpin {nullptr};
  QSlider* m_simplifySlider {nullptr};
  QSpinBox* m_simplifySpin {nullptr};
  QComboBox* m_vectorEraseModeCombo {nullptr};
  QCheckBox* m_vectorTrimOutsideCheck {nullptr};
  QSlider* m_fillThresholdSlider {nullptr};
  QSpinBox* m_fillThresholdSpin {nullptr};
  QCheckBox* m_fillContiguousCheck {nullptr};
  QCheckBox* m_fillReferAllLayersCheck {nullptr};
  QSlider* m_fillGapCloseSlider {nullptr};
  QSpinBox* m_fillGapCloseSpin {nullptr};
  QComboBox* m_selectionModeCombo {nullptr};
  QSlider* m_autoSelectThresholdSlider {nullptr};
  QSpinBox* m_autoSelectThresholdSpin {nullptr};
  QCheckBox* m_autoSelectContiguousCheck {nullptr};
  QCheckBox* m_autoSelectReferAllLayersCheck {nullptr};
  QComboBox* m_aiGranularityCombo {nullptr};
  // Rotoブラシ (AiSelect 専用)
  QWidget*     m_rotoBrushSection      {nullptr};
  QPushButton* m_rotoBrushFgBtn         {nullptr};
  QPushButton* m_rotoBrushBgBtn         {nullptr};
  QPushButton* m_rotoBrushClearBtn      {nullptr};
  QPushButton* m_rotoBrushConfirmBtn    {nullptr};
  QSlider*     m_rotoBrushRadiusSlider {nullptr};
  QLabel*      m_rotoBrushRadiusLabel  {nullptr};
  QSlider*     m_aiThresholdSlider     {nullptr};
  QLabel*      m_aiThresholdLabel      {nullptr};
  QSlider*     m_vectorApproxSlider    {nullptr};
  QLabel*      m_vectorApproxLabel     {nullptr};
  QSlider*     m_expandPixelsSlider    {nullptr};
  QLabel*      m_expandPixelsLabel     {nullptr};
  QSlider*   m_selectionFeatherSlider {nullptr};
  QSpinBox*  m_selectionFeatherSpin {nullptr};
  QCheckBox* m_selectionAntiAliasCheck {nullptr};
  // 選択オペレーション
  QPushButton* m_selOpNewBtn       {nullptr};
  QPushButton* m_selOpAddBtn       {nullptr};
  QPushButton* m_selOpSubtractBtn  {nullptr};
  QPushButton* m_selOpIntersectBtn {nullptr};
  QLabel*      m_selectionOpLabel  {nullptr};
  // 拡張 / ギャップ / エッジスナップ
  QLabel*      m_selectionExpandLabel    {nullptr};
  QSpinBox*    m_selectionExpandSpin     {nullptr};
  QLabel*      m_selectionGapCloseLabel  {nullptr};
  QSpinBox*    m_selectionGapCloseSpin   {nullptr};
  QCheckBox*   m_selectionEdgeSnapCheck  {nullptr};
  QComboBox* m_blendModeCombo {nullptr};
  QCheckBox* m_buildupModeCheck {nullptr};
  QCheckBox* m_eraseModeCheck {nullptr};
  QCheckBox* m_lockAlphaRespectCheck {nullptr};
  QWidget* m_pressureSection {nullptr};
  QCheckBox* m_pressureSizeCheck {nullptr};
  QSlider* m_pressureSizeMinSlider {nullptr};
  QSpinBox* m_pressureSizeMinSpin {nullptr};
  QCheckBox* m_pressureOpacityCheck {nullptr};
  QSlider* m_pressureOpacityMinSlider {nullptr};
  QSpinBox* m_pressureOpacityMinSpin {nullptr};
  // 速度感応
  QWidget*   m_velocitySection {nullptr};
  QCheckBox* m_velocitySizeCheck {nullptr};
  QSlider*   m_velocitySizeMinSlider {nullptr};
  QCheckBox* m_velocityOpacityCheck {nullptr};
  QSlider*   m_velocityOpacityMinSlider {nullptr};
  // テクスチャグレイン
  QWidget*   m_textureSection {nullptr};
  QCheckBox* m_textureGrainCheck {nullptr};
  QSlider*   m_textureStrengthSlider {nullptr};
  QSlider*   m_textureScaleSlider {nullptr};
  // ウェットミックス / スメア
  QWidget*   m_wetSection {nullptr};
  QCheckBox* m_wetMixCheck {nullptr};
  QSlider*   m_wetMixRateSlider {nullptr};
  QCheckBox* m_smearCheck {nullptr};
  QSlider*   m_smearRateSlider {nullptr};
  // Dab 散布 / 角度ジッター / 粒子数
  QWidget*   m_dabSection {nullptr};
  QCheckBox* m_scatterCheck {nullptr};
  QSlider*   m_scatterAmountSlider {nullptr};
  QCheckBox* m_angleJitterCheck {nullptr};
  QSlider*   m_angleJitterAmountSlider {nullptr};
  QLabel*    m_dabCountLabel {nullptr};
  QSlider*   m_dabCountSlider {nullptr};
  // メッシュ変形セクション
  QWidget*     m_meshDeformSection    {nullptr};
  QLabel*      m_meshDeformRowsLabel  {nullptr};
  QSlider*     m_meshDeformRowsSlider {nullptr};
  QLabel*      m_meshDeformColsLabel  {nullptr};
  QSlider*     m_meshDeformColsSlider {nullptr};
  QComboBox*   m_meshDeformModeCombo  {nullptr};
  QComboBox*   m_meshDeformGenCombo   {nullptr};
  QCheckBox*   m_meshDeformWireCheck  {nullptr};
  QPushButton* m_meshDeformRegenBtn   {nullptr};
  QPushButton* m_meshDeformConfirmBtn {nullptr};
  QPushButton* m_meshDeformCancelBtn  {nullptr};
  bool m_compactLayout {false};
  bool m_showDetails {false};
  QSet<QString> m_pinnedKeys;
  QString m_lastPinnedToolKey;
};

} // namespace app::panels
