#pragma once

#include <QWidget>

class QLabel;
class QListWidget;
class QLineEdit;
class QResizeEvent;
class QString;
class QToolButton;
class QBoxLayout;
class QMenu;
class QModelIndex;

namespace app::bridge {
class AppController;
}

namespace app::panels {

class SubToolPanel : public QWidget {
  Q_OBJECT

public:
  explicit SubToolPanel(QWidget* parent = nullptr);
  void setController(app::bridge::AppController* controller);

protected:
  void resizeEvent(QResizeEvent* event) override;

private slots:
  void refreshFromController();
  void onCurrentSubToolChanged(int row);
  void onRowsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row);
  void onFilterTextChanged(const QString& text);
  void onCreateClicked();
  void onDuplicateClicked();
  void onSaveClicked();
  void onRenameClicked();
  void onDeleteClicked();
  void onResetClicked();

private:
  void applyResponsiveLayout();

  bool m_refreshing {false};
  app::bridge::AppController* m_controller {nullptr};
  QBoxLayout* m_searchRowLayout {nullptr};
  QBoxLayout* m_compactActionsLayout {nullptr};
  QLabel* m_toolNameLabel {nullptr};
  QLabel* m_summaryLabel {nullptr};
  QLineEdit* m_searchEdit {nullptr};
  QToolButton* m_createButton {nullptr};
  QToolButton* m_settingsButton {nullptr};
  QMenu* m_settingsMenu {nullptr};
  QListWidget* m_subToolList {nullptr};
  bool m_internalReorder {false};
};

} // namespace app::panels
