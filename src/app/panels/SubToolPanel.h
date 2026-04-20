#pragma once

#include <QWidget>

class QLabel;
class QListWidget;

namespace app::bridge {
class AppController;
}

namespace app::panels {

class SubToolPanel : public QWidget {
  Q_OBJECT

public:
  explicit SubToolPanel(QWidget* parent = nullptr);
  void setController(app::bridge::AppController* controller);

private slots:
  void refreshFromController();
  void onCurrentSubToolChanged(int row);

private:
  bool m_refreshing {false};
  app::bridge::AppController* m_controller {nullptr};
  QLabel* m_toolNameLabel {nullptr};
  QListWidget* m_subToolList {nullptr};
};

} // namespace app::panels
