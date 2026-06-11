#pragma once

#include <QWidget>

class QLabel;
class QSlider;
class QSpinBox;
class QScrollArea;
class QStackedWidget;
class QVBoxLayout;
class QDoubleSpinBox;

namespace app::bridge {
class AppController;
}

namespace app::panels {

/// 選択中レイヤーが調整レイヤーのとき、そのパラメータを編集するパネル。
/// 非調整レイヤーが選択されているときは自動的に非表示になる。
class AdjustmentPropertyPanel : public QWidget {
  Q_OBJECT

public:
  explicit AdjustmentPropertyPanel(QWidget* parent = nullptr);
  void setController(app::bridge::AppController* controller);

private slots:
  void refreshFromController();

  // BrightnessContrast
  void onBrightnessChanged(int value);
  void onContrastChanged(int value);

  // HueSaturation
  void onHueChanged(int value);
  void onSaturationChanged(int value);
  void onLightnessChanged(int value);

  // Levels
  void onInputBlackChanged(int value);
  void onInputWhiteChanged(int value);
  void onGammaChanged(int value);   // stored *100 internally
  void onOutputBlackChanged(int value);
  void onOutputWhiteChanged(int value);

  // Vibrance
  void onVibranceChanged(int value);

private:
  QWidget* buildBrightnessContrastPage();
  QWidget* buildHueSaturationPage();
  QWidget* buildLevelsPage();
  QWidget* buildVibrancePage();

  QSlider* makeSlider(int min, int max, int value, QWidget* parent = nullptr);
  QSpinBox* makeSpinBox(int min, int max, int value, QWidget* parent = nullptr);
  void addRow(QVBoxLayout* layout, const QString& label, QSlider* slider, QSpinBox* spin);

  void applyParams();
  bool m_refreshing {false};

  app::bridge::AppController* m_controller {nullptr};
  QStackedWidget* m_stack {nullptr};
  QLabel* m_titleLabel {nullptr};

  // BrightnessContrast controls
  QSlider*  m_brightnessSlider  {nullptr};
  QSpinBox* m_brightnessSpin    {nullptr};
  QSlider*  m_contrastSlider    {nullptr};
  QSpinBox* m_contrastSpin      {nullptr};

  // HueSaturation controls
  QSlider*  m_hueSlider         {nullptr};
  QSpinBox* m_hueSpin           {nullptr};
  QSlider*  m_saturationSlider  {nullptr};
  QSpinBox* m_saturationSpin    {nullptr};
  QSlider*  m_lightnessSlider   {nullptr};
  QSpinBox* m_lightnessSpin     {nullptr};

  // Levels controls
  QSlider*  m_inputBlackSlider  {nullptr};
  QSpinBox* m_inputBlackSpin    {nullptr};
  QSlider*  m_inputWhiteSlider  {nullptr};
  QSpinBox* m_inputWhiteSpin    {nullptr};
  QSlider*  m_gammaSlider       {nullptr};   // 10..999 = 0.10..9.99
  QSpinBox* m_gammaSpin         {nullptr};   // display: 10..999
  QSlider*  m_outputBlackSlider {nullptr};
  QSpinBox* m_outputBlackSpin   {nullptr};
  QSlider*  m_outputWhiteSlider {nullptr};
  QSpinBox* m_outputWhiteSpin   {nullptr};

  // Vibrance controls
  QSlider*  m_vibranceSlider    {nullptr};
  QSpinBox* m_vibranceSpin      {nullptr};
};

} // namespace app::panels
