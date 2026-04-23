#pragma once

#include <QWidget>
#include <QSet>

class QLabel;
class QPushButton;
class QResizeEvent;
class QSpinBox;
class QSlider;
class QScrollArea;
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
  void onBlendModeChanged(int index);
  void onEraseModeToggled(bool checked);
  void onLockAlphaRespectToggled(bool checked);
  void onToggleDetailRequested();
  void onConfigurePinnedRequested();

private:
  void applyResponsiveLayout();
  void updateColorButton();
  QString currentToolSettingsKey() const;
  bool isPinned(const QString& key) const;
  void loadPinnedForCurrentTool();
  void savePinnedForCurrentTool() const;
  void refreshDetailToggleText();

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
  QLabel* m_fillThresholdLabel {nullptr};
  QLabel* m_fillGapCloseLabel {nullptr};
  QLabel* m_selectionModeLabel {nullptr};
  QLabel* m_autoSelectThresholdLabel {nullptr};
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
  QComboBox* m_blendModeCombo {nullptr};
  QCheckBox* m_eraseModeCheck {nullptr};
  QCheckBox* m_lockAlphaRespectCheck {nullptr};
  bool m_compactLayout {false};
  bool m_showDetails {false};
  QSet<QString> m_pinnedKeys;
  QString m_lastPinnedToolKey;
};

} // namespace app::panels
