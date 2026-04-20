#pragma once

#include <map>

#include <QWidget>

#include "core/tools/ToolType.h"

class QToolButton;

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

private:
  void rebuildButtons();

  app::bridge::AppController* m_controller {nullptr};
  std::map<core::ToolKind, QToolButton*> m_buttons;
};

} // namespace app::panels
