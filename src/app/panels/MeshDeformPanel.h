#pragma once

#include <QWidget>

class QSlider;
class QComboBox;
class QPushButton;
class QCheckBox;
class QLabel;

namespace app::bridge {
class AppController;
}

namespace app::panels {

/// メッシュ変形操作パネル。
/// グリッド解像度・変形モード・ジェネレータ種別を設定し、
/// Confirm / Cancel / Regenerate をコントローラ経由で実行する。
class MeshDeformPanel : public QWidget {
  Q_OBJECT

public:
  explicit MeshDeformPanel(app::bridge::AppController* controller,
                           QWidget* parent = nullptr);

signals:
  void sessionEnded();

public slots:
  /// コントローラの現在状態から UI を再同期する。
  void updateFromController();
  /// コントローラを後から設定する（遅延初期化用）。
  void setController(app::bridge::AppController* controller);

private slots:
  void onConfirm();
  void onCancel();
  void onRegenerateMesh();
  void onModeChanged(int index);
  void onGeneratorChanged(int index);

private:
  app::bridge::AppController* m_controller {nullptr};

  // Grid resolution
  QLabel*  m_rowsLabel   {nullptr};
  QSlider* m_rowsSlider  {nullptr};   // range 4..64, default 16
  QLabel*  m_colsLabel   {nullptr};
  QSlider* m_colsSlider  {nullptr};   // range 4..64, default 16

  // Mode / generator
  QComboBox* m_modeCombo      {nullptr};  // Similarity, Rigid
  QComboBox* m_generatorCombo {nullptr};  // Grid, EdgeAdaptive

  // Display
  QCheckBox* m_wireframeCheck {nullptr};

  // Actions
  QPushButton* m_regenerateBtn {nullptr};
  QPushButton* m_confirmBtn    {nullptr};
  QPushButton* m_cancelBtn     {nullptr};
};

} // namespace app::panels
