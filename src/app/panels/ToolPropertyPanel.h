#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QSpinBox;
class QSlider;

namespace app::bridge {
class AppController;
}

namespace app::panels {

class ToolPropertyPanel : public QWidget {
  Q_OBJECT

public:
  explicit ToolPropertyPanel(QWidget* parent = nullptr);
  void setController(app::bridge::AppController* controller);

private slots:
  void refreshFromController();
  void onChooseColor();
  void onSizeChanged(int size);
  void onOpacitySliderChanged(int value);
  void onOpacitySpinChanged(int value);
  void onHardnessSliderChanged(int value);
  void onHardnessSpinChanged(int value);

private:
  void updateColorButton();

  app::bridge::AppController* m_controller {nullptr};
  QLabel* m_toolNameLabel {nullptr};
  QLabel* m_guideLabel {nullptr};
  QLabel* m_colorLabel {nullptr};
  QLabel* m_sizeLabel {nullptr};
  QLabel* m_opacityLabel {nullptr};
  QLabel* m_hardnessLabel {nullptr};
  QPushButton* m_colorButton {nullptr};
  QSpinBox* m_sizeSpin {nullptr};
  QSlider* m_opacitySlider {nullptr};
  QSpinBox* m_opacitySpin {nullptr};
  QSlider* m_hardnessSlider {nullptr};
  QSpinBox* m_hardnessSpin {nullptr};
};

} // namespace app::panels
