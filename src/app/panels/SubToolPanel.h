#pragma once

#include <QWidget>

class QLabel;
class QListWidget;
class QLineEdit;
class QPushButton;
class QString;
class QToolButton;

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
  void onFilterTextChanged(const QString& text);
  void onDuplicateClicked();
  void onRenameClicked();
  void onDeleteClicked();
  void onResetClicked();

private:
  bool m_refreshing {false};
  app::bridge::AppController* m_controller {nullptr};
  QLabel* m_toolNameLabel {nullptr};
  QLabel* m_summaryLabel {nullptr};
  QLineEdit* m_searchEdit {nullptr};
  QPushButton* m_duplicateButton {nullptr};
  QToolButton* m_renameButton {nullptr};
  QToolButton* m_deleteButton {nullptr};
  QToolButton* m_resetButton {nullptr};
  QListWidget* m_subToolList {nullptr};
};

} // namespace app::panels
