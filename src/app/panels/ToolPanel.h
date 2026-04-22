#pragma once

#include <map>

#include <QWidget>

#include "core/tools/ToolType.h"

class QToolButton;
class QSlider;
class QLabel;

namespace app::bridge {
class AppController;
}

namespace app::panels {

class ToolPanel : public QWidget {
  Q_OBJECT

public:
  explicit ToolPanel(QWidget* parent = nullptr);
  void setController(app::bridge::AppController* controller);

private slots:
  void refreshFromController();
  void onToolButtonClicked();
  void onSizeSliderChanged(int value);
  void onOpacitySliderChanged(int value);

private:
  void rebuildButtons();

  app::bridge::AppController* m_controller {nullptr};
  std::map<core::ToolKind, QToolButton*> m_buttons;
  QSlider* m_sizeSlider {nullptr};
  QSlider* m_opacitySlider {nullptr};
  QLabel* m_sizeValueLabel {nullptr};
  QLabel* m_opacityValueLabel {nullptr};
  bool m_refreshingSliders {false};
};

} // namespace app::panels
