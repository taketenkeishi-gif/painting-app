#pragma once

#include <map>
#include <vector>

#include <QWidget>

#include "core/tools/ToolType.h"

class QGridLayout;
class QToolButton;
class QSlider;
class QLabel;
class QVBoxLayout;

namespace app::bridge {
class AppController;
}

namespace app::panels {

class ToolPanel : public QWidget {
  Q_OBJECT

public:
  explicit ToolPanel(QWidget* parent = nullptr);
  void setController(app::bridge::AppController* controller);

protected:
  void resizeEvent(QResizeEvent* event) override;

private slots:
  void refreshFromController();
  void onToolButtonClicked();
  void onSizeSliderChanged(int value);
  void onOpacitySliderChanged(int value);

private:
  void rebuildButtons();
  void relayoutButtons();
  int columnCountForWidth(int width) const noexcept;

  app::bridge::AppController* m_controller {nullptr};
  std::map<core::ToolKind, QToolButton*> m_buttons;
  std::vector<QToolButton*> m_buttonOrder;
  QWidget* m_buttonGridHost {nullptr};
  QGridLayout* m_buttonGrid {nullptr};
  QWidget* m_quickHost {nullptr};
  QVBoxLayout* m_rootLayout {nullptr};
  QSlider* m_sizeSlider {nullptr};
  QSlider* m_opacitySlider {nullptr};
  QLabel* m_sizeValueLabel {nullptr};
  QLabel* m_opacityValueLabel {nullptr};
  bool m_refreshingSliders {false};
};

} // namespace app::panels
