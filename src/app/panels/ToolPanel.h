#pragma once

#include <map>
#include <vector>

#include <QWidget>

#include "core/tools/ToolType.h"

class QGridLayout;
class QScrollArea;
class QToolButton;
class QSlider;
class QLabel;
class QFrame;
class QColor;
class QVBoxLayout;

namespace app::bridge {
class AppController;
}

namespace app::panels {

class ToolPanel : public QWidget {
  Q_OBJECT

public:
  enum class Section : unsigned {
    Buttons = 0x1,
    QuickSliders = 0x2
  };
  using Sections = unsigned;
  static constexpr Sections ButtonsOnly = static_cast<Sections>(Section::Buttons);
  static constexpr Sections QuickSlidersOnly = static_cast<Sections>(Section::QuickSliders);
  static constexpr Sections Combined = ButtonsOnly | QuickSlidersOnly;

  explicit ToolPanel(QWidget* parent = nullptr);
  void setController(app::bridge::AppController* controller);
  void setSections(Sections sections) noexcept;

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
  void updateQuickSliderVisuals(const QColor& color);
  int columnCountForWidth(int width) const noexcept;

  app::bridge::AppController* m_controller {nullptr};
  std::map<core::ToolKind, QToolButton*> m_buttons;
  std::vector<QToolButton*> m_buttonOrder;
  QScrollArea* m_buttonScrollArea {nullptr};
  QWidget* m_buttonGridHost {nullptr};
  QGridLayout* m_buttonGrid {nullptr};
  QWidget* m_quickHost {nullptr};
  QVBoxLayout* m_rootLayout {nullptr};
  QSlider* m_sizeSlider {nullptr};
  QSlider* m_opacitySlider {nullptr};
  QLabel* m_sizeValueLabel {nullptr};
  QLabel* m_opacityValueLabel {nullptr};
  QLabel* m_sizeUnitLabel {nullptr};
  QLabel* m_opacityUnitLabel {nullptr};
  QFrame* m_sizeChip {nullptr};
  QFrame* m_opacityChip {nullptr};
  bool m_refreshingSliders {false};
  Sections m_sections {Combined};
};

} // namespace app::panels
