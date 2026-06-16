#include "app/panels/WorkflowBindingDialog.h"

#include <algorithm>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QJsonObject>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

namespace app::panels {

WorkflowBindingDialog::WorkflowBindingDialog(const QString& workflowName,
                                              const QJsonObject& workflow,
                                              const WorkflowBindingConfig& current,
                                              QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("ノードバインド — ") + workflowName);
    setMinimumWidth(440);

    // ── ノード一覧を解析 ──────────────────────────────────────────────────────
    for (auto it = workflow.constBegin(); it != workflow.constEnd(); ++it) {
        const QJsonObject node = it.value().toObject();
        const QString classType = node.value(QStringLiteral("class_type")).toString();
        if (!classType.isEmpty()) {
            m_nodes.append({it.key(), classType});
        }
    }
    // ノード ID で昇順ソート（数値なら数値順）
    std::sort(m_nodes.begin(), m_nodes.end(), [](const NodeInfo& a, const NodeInfo& b) {
        bool aOk, bOk;
        const int ai = a.nodeId.toInt(&aOk);
        const int bi = b.nodeId.toInt(&bOk);
        if (aOk && bOk) return ai < bi;
        return a.nodeId < b.nodeId;
    });

    // ── レイアウト ────────────────────────────────────────────────────────────
    auto* root = new QVBoxLayout(this);
    root->setSpacing(8);

    auto* infoLabel = new QLabel(
        QString(QStringLiteral("ワークフロー: %1\n各役割にノード ID を割り当ててください。"))
            .arg(workflowName),
        this);
    infoLabel->setStyleSheet(QStringLiteral("color:#8090b0; font-size:10px;"));
    root->addWidget(infoLabel);

    // ── ノード一覧プレビュー（折りたたみ可） ─────────────────────────────────
    {
        auto* grp = new QGroupBox(
            QString(QStringLiteral("検出されたノード (%1 個)")).arg(m_nodes.size()),
            this);
        auto* lay = new QVBoxLayout(grp);
        lay->setContentsMargins(6, 14, 6, 6);
        lay->setSpacing(2);

        auto* scroll = new QScrollArea(grp);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setMaximumHeight(90);
        scroll->setWidgetResizable(true);
        auto* inner = new QWidget();
        auto* iLay  = new QVBoxLayout(inner);
        iLay->setContentsMargins(0, 0, 0, 0);
        iLay->setSpacing(1);

        for (const NodeInfo& n : m_nodes) {
            auto* lbl = new QLabel(
                QString(QStringLiteral("  %1  —  %2")).arg(n.nodeId, n.classType),
                inner);
            lbl->setStyleSheet(QStringLiteral("color:#6878a0; font-size:10px; font-family:monospace;"));
            iLay->addWidget(lbl);
        }
        iLay->addStretch();
        scroll->setWidget(inner);
        lay->addWidget(scroll);
        root->addWidget(grp);
    }

    // ── バインド設定 ─────────────────────────────────────────────────────────
    {
        auto* grp  = new QGroupBox(QStringLiteral("バインド設定"), this);
        auto* form = new QFormLayout(grp);
        form->setContentsMargins(8, 16, 8, 8);
        form->setLabelAlignment(Qt::AlignRight);
        form->setSpacing(8);

        m_inputCombo    = new QComboBox(grp);
        m_maskCombo     = new QComboBox(grp);
        m_positiveCombo = new QComboBox(grp);
        m_negativeCombo = new QComboBox(grp);
        m_kSamplerCombo = new QComboBox(grp);

        fillCombo(m_inputCombo,
                  {QStringLiteral("LoadImage")},
                  current.inputImageNodeId);
        fillCombo(m_maskCombo,
                  {QStringLiteral("LoadImage")},
                  current.maskNodeId);
        fillCombo(m_positiveCombo,
                  {QStringLiteral("CLIPTextEncode")},
                  current.positiveNodeId);
        fillCombo(m_negativeCombo,
                  {QStringLiteral("CLIPTextEncode")},
                  current.negativeNodeId);
        fillCombo(m_kSamplerCombo,
                  {QStringLiteral("KSampler"), QStringLiteral("KSamplerAdvanced")},
                  current.kSamplerNodeId);

        form->addRow(QStringLiteral("入力画像 (LoadImage):"),  m_inputCombo);
        form->addRow(QStringLiteral("マスク (LoadImage):"),    m_maskCombo);
        form->addRow(QStringLiteral("ポジティブ (CLIP):"),     m_positiveCombo);
        form->addRow(QStringLiteral("ネガティブ (CLIP):"),     m_negativeCombo);
        form->addRow(QStringLiteral("KSampler:"),              m_kSamplerCombo);

        root->addWidget(grp);
    }

    // ── ボタン ────────────────────────────────────────────────────────────────
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}

// ─────────────────────────────────────────────────────────────────────────────
// fillCombo
// ─────────────────────────────────────────────────────────────────────────────
void WorkflowBindingDialog::fillCombo(QComboBox* combo,
                                       const QStringList& preferredClasses,
                                       const QString& currentId)
{
    combo->addItem(QStringLiteral("(なし)"), QString());

    // 一致するクラスを先頭に
    for (const NodeInfo& n : m_nodes) {
        if (preferredClasses.contains(n.classType)) {
            combo->addItem(
                QString(QStringLiteral("%1 — %2")).arg(n.nodeId, n.classType),
                n.nodeId);
        }
    }
    // 区切り + 残り（異なるクラスの全ノード）
    bool hasSep = false;
    for (const NodeInfo& n : m_nodes) {
        if (!preferredClasses.contains(n.classType)) {
            if (!hasSep) {
                combo->insertSeparator(combo->count());
                hasSep = true;
            }
            combo->addItem(
                QString(QStringLiteral("%1 — %2")).arg(n.nodeId, n.classType),
                n.nodeId);
        }
    }

    // 現在値を選択（なければ一致クラスの最初）
    if (!currentId.isEmpty()) {
        const int idx = combo->findData(currentId);
        if (idx >= 0) {
            combo->setCurrentIndex(idx);
            return;
        }
    }
    // デフォルト: 一致クラスの最初（index 1）
    if (combo->count() > 1) {
        combo->setCurrentIndex(1);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// result
// ─────────────────────────────────────────────────────────────────────────────
WorkflowBindingConfig WorkflowBindingDialog::result() const
{
    WorkflowBindingConfig cfg;
    cfg.inputImageNodeId = m_inputCombo->currentData().toString();
    cfg.maskNodeId       = m_maskCombo->currentData().toString();
    cfg.positiveNodeId   = m_positiveCombo->currentData().toString();
    cfg.negativeNodeId   = m_negativeCombo->currentData().toString();
    cfg.kSamplerNodeId   = m_kSamplerCombo->currentData().toString();
    return cfg;
}

} // namespace app::panels
