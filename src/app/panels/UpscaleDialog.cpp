#include "app/panels/UpscaleDialog.h"

#include <QBuffer>
#include <QButtonGroup>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QThread>
#include <QVBoxLayout>

#include "app/bridge/AiService.h"
#include "platform/qt/QtImageConverter.h"

namespace app::panels {

// ─────────────────────────────────────────────────────────────────────────────
// UpscaleWorker
// ─────────────────────────────────────────────────────────────────────────────
void UpscaleWorker::run() {
    core::ai::UpscaleEngine engine({m_modelPath});
    m_result = engine.upscale(m_src, m_scale, [this](int pct) {
        emit progressChanged(pct);
    });
    emit finished();
}

// ─────────────────────────────────────────────────────────────────────────────
// UpscaleDialog — コンストラクタ
// ─────────────────────────────────────────────────────────────────────────────
UpscaleDialog::UpscaleDialog(
    const core::PixelBuffer& src,
    const std::vector<core::ai::UpscaleEngine::ModelInfo>& localModels,
    app::bridge::AiService* aiService,
    QWidget* parent)
    : QDialog(parent)
    , m_src(src)
    , m_localModels(localModels)
    , m_aiService(aiService) {
    setWindowTitle(QString::fromUtf8(u8"AI 高解像度化"));
    setFixedWidth(420);
    setupUi();
    updateOutputSizeLabel();

    if (m_aiService) {
        connect(m_aiService, &app::bridge::AiService::upscaleResult,
                this, &UpscaleDialog::onAiUpscaleResult);
        connect(m_aiService, &app::bridge::AiService::upscaleError,
                this, &UpscaleDialog::onAiUpscaleError);
        connect(m_aiService, &app::bridge::AiService::upscaleModelsReady,
                this, &UpscaleDialog::onAiModelsReady);

        m_statusLabel->setText(QString::fromUtf8(u8"ComfyUI モデルを読み込み中..."));
        m_aiService->fetchUpscaleModels();
    }
}

UpscaleDialog::~UpscaleDialog() {
    if (m_thread && m_thread->isRunning()) {
        m_thread->quit();
        m_thread->wait(3000);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// UI 構築
// ─────────────────────────────────────────────────────────────────────────────
void UpscaleDialog::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setSpacing(10);
    root->setContentsMargins(14, 14, 14, 14);

    // ── モデル選択 ───────────────────────────────────────────────────────────
    {
        auto* row = new QHBoxLayout();
        auto* lbl = new QLabel(QString::fromUtf8(u8"モデル:"), this);
        lbl->setFixedWidth(52);
        m_modelCombo = new QComboBox(this);

        // ComfyUI モデルはあとで populateComfyModels() で追加
        // ローカル ONNX モデル
        for (const auto& m : m_localModels) {
            m_modelCombo->addItem(
                QString("[ONNX]  %1  [×%2]").arg(
                    QString::fromStdString(m.displayName),
                    QString::number(m.nativeScale)),
                QString::fromStdString(m.path));
            m_modelCombo->setItemData(m_modelCombo->count() - 1,
                static_cast<int>(Backend::Onnx), Qt::UserRole + 1);
        }
        // バイリニア（常に末尾）
        m_modelCombo->addItem(
            QString::fromUtf8(u8"スムーズ補間（AI なし）"), QString());
        m_modelCombo->setItemData(m_modelCombo->count() - 1,
            static_cast<int>(Backend::Bilinear), Qt::UserRole + 1);

        if (m_localModels.empty() && !m_aiService) {
            m_modelCombo->setToolTip(
                QString::fromUtf8(
                    u8"ComfyUI 接続 or ONNX モデルを\n"
                    u8"AI → AI モデルフォルダを設定 で登録してください"));
        }
        row->addWidget(lbl);
        row->addWidget(m_modelCombo, 1);
        root->addLayout(row);
    }

    // ── 指定方法 ─────────────────────────────────────────────────────────────
    {
        auto* modeGroup = new QGroupBox(QString::fromUtf8(u8"指定方法"), this);
        auto* modeRow   = new QHBoxLayout(modeGroup);
        m_radioScale    = new QRadioButton(QString::fromUtf8(u8"倍率指定"), modeGroup);
        m_radioSize     = new QRadioButton(QString::fromUtf8(u8"出力サイズ指定"), modeGroup);
        m_radioScale->setChecked(true);
        modeRow->addWidget(m_radioScale);
        modeRow->addWidget(m_radioSize);
        modeRow->addStretch();
        root->addWidget(modeGroup);
    }

    // ── 倍率指定ウィジェット ─────────────────────────────────────────────────
    {
        m_scaleWidget = new QWidget(this);
        auto* row     = new QHBoxLayout(m_scaleWidget);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);

        auto* lbl = new QLabel(QString::fromUtf8(u8"倍率:"), m_scaleWidget);
        lbl->setFixedWidth(40);
        row->addWidget(lbl);

        m_scaleBtns = new QButtonGroup(this);
        m_scaleBtns->setExclusive(true);

        const QString btnStyle =
            "QPushButton { background: #1e2130; border: 1px solid #2e3348; "
            "border-radius: 4px; color: #c8cde0; padding: 4px 18px; font-weight: 600; }"
            "QPushButton:checked { background: #1d4a8a; border-color: #4e8ef7; color: #ffffff; }"
            "QPushButton:hover:!checked { background: #252b40; }";

        for (int s : {2, 4, 8}) {
            auto* btn = new QPushButton(QString("×%1").arg(s), m_scaleWidget);
            btn->setCheckable(true);
            btn->setStyleSheet(btnStyle);
            btn->setChecked(s == 2);
            m_scaleBtns->addButton(btn, s);
            row->addWidget(btn);
        }
        row->addStretch();
        root->addWidget(m_scaleWidget);
    }

    // ── サイズ指定ウィジェット ────────────────────────────────────────────────
    {
        m_sizeWidget = new QWidget(this);
        m_sizeWidget->setVisible(false);
        auto* grid = new QHBoxLayout(m_sizeWidget);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setSpacing(8);

        auto* wLbl = new QLabel(QString::fromUtf8(u8"幅:"), m_sizeWidget);
        wLbl->setFixedWidth(24);
        m_targetWSpin = new QSpinBox(m_sizeWidget);
        m_targetWSpin->setRange(1, 32000);
        m_targetWSpin->setValue(m_src.width() * 2);
        m_targetWSpin->setSuffix(" px");
        m_targetWSpin->setAlignment(Qt::AlignRight);

        auto* hLbl = new QLabel(QString::fromUtf8(u8"高さ:"), m_sizeWidget);
        hLbl->setFixedWidth(32);
        m_targetHSpin = new QSpinBox(m_sizeWidget);
        m_targetHSpin->setRange(1, 32000);
        m_targetHSpin->setValue(m_src.height() * 2);
        m_targetHSpin->setSuffix(" px");
        m_targetHSpin->setAlignment(Qt::AlignRight);

        grid->addWidget(wLbl);
        grid->addWidget(m_targetWSpin, 1);
        grid->addWidget(hLbl);
        grid->addWidget(m_targetHSpin, 1);
        root->addWidget(m_sizeWidget);
    }

    // ── 出力サイズ表示 ────────────────────────────────────────────────────────
    m_outputLabel = new QLabel(this);
    m_outputLabel->setStyleSheet("color: #7a86a3; font-size: 11px;");
    root->addWidget(m_outputLabel);

    root->addSpacing(4);

    // ── プログレス + ステータス ───────────────────────────────────────────────
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFixedHeight(10);
    m_progressBar->setVisible(false);
    root->addWidget(m_progressBar);

    m_statusLabel = new QLabel(QString::fromUtf8(u8"準備完了"), this);
    m_statusLabel->setStyleSheet("color: #7a86a3; font-size: 10px;");
    root->addWidget(m_statusLabel);

    // ── ボタン ────────────────────────────────────────────────────────────────
    auto* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_runBtn = btns->button(QDialogButtonBox::Ok);
    m_runBtn->setText(QString::fromUtf8(u8"実行"));
    m_runBtn->setStyleSheet(
        "QPushButton { background: #1d4a8a; border: 1px solid #4e8ef7; color: #fff;"
        "font-weight: 700; border-radius: 4px; padding: 5px 20px; }"
        "QPushButton:hover { background: #2a60b0; }"
        "QPushButton:disabled { background: #1a1d27; color: #4a5268; border-color: #252a38; }");
    root->addWidget(btns);

    // ── シグナル接続 ─────────────────────────────────────────────────────────
    connect(m_radioScale, &QRadioButton::toggled, this, &UpscaleDialog::onModeChanged);
    connect(m_scaleBtns, &QButtonGroup::idClicked, this, [this](int id) {
        m_selectedScale = id;
        updateOutputSizeLabel();
    });
    connect(m_targetWSpin, &QSpinBox::valueChanged,
            this, &UpscaleDialog::onTargetWidthChanged);
    connect(m_targetHSpin, &QSpinBox::valueChanged,
            this, &UpscaleDialog::onTargetHeightChanged);
    connect(m_runBtn, &QPushButton::clicked, this, &UpscaleDialog::onRunClicked);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

// ─────────────────────────────────────────────────────────────────────────────
// ComfyUI モデルをコンボ先頭に挿入
// ─────────────────────────────────────────────────────────────────────────────
void UpscaleDialog::populateComfyModels(const QStringList& models) {
    int idx = 0;
    for (const QString& name : models) {
        m_modelCombo->insertItem(idx,
            QString("[ComfyUI]  %1").arg(name),
            name);
        m_modelCombo->setItemData(idx, static_cast<int>(Backend::ComfyUI), Qt::UserRole + 1);
        ++idx;
    }
    if (!models.isEmpty()) {
        m_modelCombo->setCurrentIndex(0);   // ComfyUI モデルをデフォルト選択
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ヘルパー
// ─────────────────────────────────────────────────────────────────────────────
UpscaleDialog::Backend UpscaleDialog::currentBackend() const {
    return static_cast<Backend>(
        m_modelCombo->currentData(Qt::UserRole + 1).toInt());
}

int UpscaleDialog::currentScale() const {
    if (m_radioScale->isChecked()) return m_selectedScale;
    if (m_src.width() <= 0) return 2;
    const double ratio = static_cast<double>(m_targetWSpin->value()) / m_src.width();
    return (ratio <= 3.0) ? 2 : 4;
}

// ─────────────────────────────────────────────────────────────────────────────
// モード切り替え
// ─────────────────────────────────────────────────────────────────────────────
void UpscaleDialog::onModeChanged() {
    const bool scaleMode = m_radioScale->isChecked();
    m_scaleWidget->setVisible(scaleMode);
    m_sizeWidget->setVisible(!scaleMode);
    updateOutputSizeLabel();
}

void UpscaleDialog::updateOutputSizeLabel() {
    int outW, outH;
    if (m_radioScale->isChecked()) {
        outW = m_src.width()  * m_selectedScale;
        outH = m_src.height() * m_selectedScale;
    } else {
        outW = m_targetWSpin->value();
        outH = m_targetHSpin->value();
    }
    m_outputLabel->setText(
        QString::fromUtf8(u8"現在: %1 × %2 px  →  出力: %3 × %4 px")
            .arg(m_src.width()).arg(m_src.height()).arg(outW).arg(outH));
}

void UpscaleDialog::onTargetWidthChanged(int w) {
    if (m_ignoreSpinChange || m_src.width() <= 0) return;
    m_ignoreSpinChange = true;
    m_targetHSpin->setValue(static_cast<int>(
        w * static_cast<double>(m_src.height()) / m_src.width() + 0.5));
    m_ignoreSpinChange = false;
    updateOutputSizeLabel();
}

void UpscaleDialog::onTargetHeightChanged(int h) {
    if (m_ignoreSpinChange || m_src.height() <= 0) return;
    m_ignoreSpinChange = true;
    m_targetWSpin->setValue(static_cast<int>(
        h * static_cast<double>(m_src.width()) / m_src.height() + 0.5));
    m_ignoreSpinChange = false;
    updateOutputSizeLabel();
}

// ─────────────────────────────────────────────────────────────────────────────
// 実行ボタン — バックエンドを分岐
// ─────────────────────────────────────────────────────────────────────────────
void UpscaleDialog::onRunClicked() {
    if (m_thread && m_thread->isRunning()) return;

    if (currentBackend() == Backend::ComfyUI) {
        runViaComfyUI();
        return;
    }

    // ONNX / bilinear パス（既存ロジック）
    const int scale           = currentScale();
    const std::string mdlPath = m_modelCombo->currentData(Qt::UserRole).toString().toStdString();

    m_hasResult = false;
    setRunning(true);
    m_statusLabel->setText(QString::fromUtf8(u8"処理中...（×%1）").arg(scale));

    m_thread = new QThread(this);
    m_worker = new UpscaleWorker();
    m_worker->setSrc(m_src);
    m_worker->setScale(scale);
    m_worker->setModelPath(mdlPath);
    m_worker->moveToThread(m_thread);

    connect(m_thread, &QThread::started,  m_worker, &UpscaleWorker::run);
    connect(m_worker, &UpscaleWorker::progressChanged,
            this,  &UpscaleDialog::onProgressChanged, Qt::QueuedConnection);
    connect(m_worker, &UpscaleWorker::finished,
            this,  &UpscaleDialog::onWorkerFinished,  Qt::QueuedConnection);
    m_thread->start();
}

// ─────────────────────────────────────────────────────────────────────────────
// ComfyUI 実行パス（AiService 経由）
// ─────────────────────────────────────────────────────────────────────────────
void UpscaleDialog::runViaComfyUI() {
    if (!m_aiService) {
        m_statusLabel->setText(QString::fromUtf8(u8"AiService が利用できません"));
        return;
    }

    const QString modelName = m_modelCombo->currentData(Qt::UserRole).toString();

    const QImage img = platform::qt::QtImageConverter::toQImage(m_src);
    QByteArray pngBytes;
    QBuffer buf(&pngBytes);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
    buf.close();

    m_hasResult = false;
    setRunning(true);
    m_progressBar->setRange(0, 0);
    m_statusLabel->setText(QString::fromUtf8(u8"ComfyUI: 処理中..."));

    app::bridge::AiService::UpscaleRequest req;
    req.imagePng  = pngBytes;
    req.modelName = modelName;
    m_aiService->upscale(req);
}

// ─────────────────────────────────────────────────────────────────────────────
// AiService シグナルハンドラー
// ─────────────────────────────────────────────────────────────────────────────
void UpscaleDialog::onAiUpscaleResult(const QByteArray& pngData) {
    setRunning(false);
    if (pngData.isEmpty()) {
        m_statusLabel->setText(QString::fromUtf8(u8"画像取得失敗"));
        return;
    }
    QImage img;
    img.loadFromData(pngData, "PNG");
    if (img.isNull()) {
        m_statusLabel->setText(QString::fromUtf8(u8"画像デコード失敗"));
        return;
    }
    m_result    = platform::qt::QtImageConverter::fromQImage(img);
    m_hasResult = m_result.width() > 0;
    m_statusLabel->setText(
        QString::fromUtf8(u8"完了: %1 × %2 px")
            .arg(m_result.width()).arg(m_result.height()));
    if (m_hasResult) accept();
}

void UpscaleDialog::onAiUpscaleError(const QString& message) {
    setRunning(false);
    m_statusLabel->setText(QString::fromUtf8(u8"ComfyUI エラー: ") + message);
}

void UpscaleDialog::onAiModelsReady(const QStringList& models) {
    populateComfyModels(models);
    const QString msg = models.isEmpty()
        ? QString::fromUtf8(u8"ComfyUI: アップスケールモデルなし")
        : QString::fromUtf8(u8"ComfyUI: %1 個のモデルを検出").arg(models.size());
    m_statusLabel->setText(msg);
}

// ─────────────────────────────────────────────────────────────────────────────
// ONNX / bilinear 完了ハンドラー
// ─────────────────────────────────────────────────────────────────────────────
void UpscaleDialog::onProgressChanged(int percent) {
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(percent);
}

void UpscaleDialog::onWorkerFinished() {
    m_result    = m_worker->result();
    m_hasResult = true;
    m_thread->quit();
    m_thread->wait();
    m_worker->deleteLater();  m_worker = nullptr;
    m_thread->deleteLater();  m_thread = nullptr;
    setRunning(false);
    m_statusLabel->setText(
        QString::fromUtf8(u8"完了: %1 × %2 px")
            .arg(m_result.width()).arg(m_result.height()));
    accept();
}

// ─────────────────────────────────────────────────────────────────────────────
// 実行中 UI 切り替え
// ─────────────────────────────────────────────────────────────────────────────
void UpscaleDialog::setRunning(bool running) {
    m_runBtn->setEnabled(!running);
    m_modelCombo->setEnabled(!running);
    m_radioScale->setEnabled(!running);
    m_radioSize->setEnabled(!running);
    m_scaleWidget->setEnabled(!running);
    m_sizeWidget->setEnabled(!running);
    m_progressBar->setVisible(running);
    if (!running) {
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(0);
    }
}

} // namespace app::panels
