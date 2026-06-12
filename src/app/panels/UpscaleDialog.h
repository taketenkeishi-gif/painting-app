#pragma once

#include <vector>

#include <QDialog>

#include "core/ai/UpscaleEngine.h"
#include "core/buffer/PixelBuffer.h"

class QButtonGroup;
class QComboBox;
class QLabel;
class QProgressBar;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QThread;

namespace app::bridge { class ComfyUiClient; }

namespace app::panels {

// ─────────────────────────────────────────────────────────────────────────────
// UpscaleWorker  — 別スレッドで UpscaleEngine (ONNX / bilinear) を実行
// ─────────────────────────────────────────────────────────────────────────────
class UpscaleWorker : public QObject {
    Q_OBJECT
public:
    void setSrc(core::PixelBuffer src)   { m_src   = std::move(src); }
    void setScale(int scale)             { m_scale = scale; }
    void setModelPath(std::string path)  { m_modelPath = std::move(path); }

    const core::PixelBuffer& result() const { return m_result; }

public slots:
    void run();

signals:
    void progressChanged(int percent);
    void finished();

private:
    core::PixelBuffer m_src;
    int               m_scale     {2};
    std::string       m_modelPath;
    core::PixelBuffer m_result;
};

// ─────────────────────────────────────────────────────────────────────────────
// UpscaleDialog  — AI 高解像度化ダイアログ
//
// バックエンド優先順:
//   1. ComfyUI（接続中 & safetensors モデル選択時）
//   2. ONNX（ローカル .onnx モデル選択時）
//   3. バイリニア補間（AI なし）
// ─────────────────────────────────────────────────────────────────────────────
class UpscaleDialog : public QDialog {
    Q_OBJECT

    enum class Backend { Bilinear = 0, Onnx = 1, ComfyUI = 2 };

public:
    explicit UpscaleDialog(const core::PixelBuffer& src,
                           const std::vector<core::ai::UpscaleEngine::ModelInfo>& localModels,
                           app::bridge::ComfyUiClient* comfyClient = nullptr,
                           QWidget* parent = nullptr);
    ~UpscaleDialog() override;

    bool hasResult() const { return m_hasResult; }
    const core::PixelBuffer& result() const { return m_result; }

private slots:
    void onRunClicked();
    void onProgressChanged(int percent);
    void onWorkerFinished();
    void onModeChanged();
    void onTargetWidthChanged(int w);
    void onTargetHeightChanged(int h);
    // ComfyUI
    void onComfyComplete(const QString& promptId, const QStringList& outputs);
    void onComfyError   (const QString& promptId, const QString& message);
    void onComfyProgress(const QString& promptId, int step, int total, const QString& nodeId);

private:
    void setupUi();
    void populateComfyModels(const QStringList& models);
    void updateOutputSizeLabel();
    void setRunning(bool running);
    int     currentScale()   const;
    Backend currentBackend() const;
    void runViaComfyUI();

    const core::PixelBuffer& m_src;
    std::vector<core::ai::UpscaleEngine::ModelInfo> m_localModels;
    app::bridge::ComfyUiClient* m_comfyClient {nullptr};
    QString m_comfyPromptId;

    // UI
    QComboBox*    m_modelCombo   {nullptr};
    QRadioButton* m_radioScale   {nullptr};
    QRadioButton* m_radioSize    {nullptr};
    QWidget*      m_scaleWidget  {nullptr};
    QWidget*      m_sizeWidget   {nullptr};
    QButtonGroup* m_scaleBtns    {nullptr};
    QSpinBox*     m_targetWSpin  {nullptr};
    QSpinBox*     m_targetHSpin  {nullptr};
    QLabel*       m_outputLabel  {nullptr};
    QProgressBar* m_progressBar  {nullptr};
    QLabel*       m_statusLabel  {nullptr};
    QPushButton*  m_runBtn       {nullptr};

    // 状態
    int  m_selectedScale    {2};
    bool m_hasResult        {false};
    bool m_ignoreSpinChange {false};
    core::PixelBuffer m_result;

    // スレッド（ONNX / bilinear 用）
    QThread*       m_thread {nullptr};
    UpscaleWorker* m_worker {nullptr};
};

} // namespace app::panels
