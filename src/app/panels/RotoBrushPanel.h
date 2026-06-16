#pragma once

#include <QWidget>

class QPushButton;
class QSlider;
class QLabel;

namespace app::bridge {
class AppController;
}

namespace app::panels {

/// Roto ブラシパネル。
/// 前景 / 背景ブラシの切り替えと、ブラシ半径調整、
/// ストロークのクリアボタンを提供する。
class RotoBrushPanel : public QWidget {
  Q_OBJECT

public:
  explicit RotoBrushPanel(app::bridge::AppController* controller,
                          QWidget* parent = nullptr);

public slots:
  void setController(app::bridge::AppController* controller);

private slots:
  void onFgClicked();
  void onBgClicked();
  void onClearClicked();
  void onRadiusChanged(int value);

private:
  void updateButtonStates();

  app::bridge::AppController* m_controller {nullptr};
  bool m_isForeground {true};

  QPushButton* m_fgBtn    {nullptr};
  QPushButton* m_bgBtn    {nullptr};
  QPushButton* m_clearBtn {nullptr};
  QSlider*     m_radiusSlider {nullptr};
  QLabel*      m_radiusLabel  {nullptr};
};

} // namespace app::panels
