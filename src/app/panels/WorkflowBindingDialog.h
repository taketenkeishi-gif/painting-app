#pragma once

#include <QDialog>
#include <QJsonObject>
#include <QList>
#include <QString>

class QComboBox;

namespace app::panels {

// ─────────────────────────────────────────────────────────────────────────────
// WorkflowBindingConfig
//
// ワークフロー内のノード役割 → ノード ID の対応。
// AiPanel が保持し、QSettings に保存する。
// ─────────────────────────────────────────────────────────────────────────────
struct WorkflowBindingConfig {
    QString inputImageNodeId;   // LoadImage  — キャンバス合成画像
    QString maskNodeId;         // LoadImage  — 選択マスク
    QString positiveNodeId;     // CLIPTextEncode — ポジティブプロンプト
    QString negativeNodeId;     // CLIPTextEncode — ネガティブプロンプト
    QString kSamplerNodeId;     // KSampler / KSamplerAdvanced
};

// ─────────────────────────────────────────────────────────────────────────────
// WorkflowBindingDialog
//
// workflow.json を解析し、5 役割へのノード ID 割当を行うダイアログ。
// ─────────────────────────────────────────────────────────────────────────────
class WorkflowBindingDialog : public QDialog {
    Q_OBJECT
public:
    explicit WorkflowBindingDialog(const QString& workflowName,
                                   const QJsonObject& workflow,
                                   const WorkflowBindingConfig& current,
                                   QWidget* parent = nullptr);

    WorkflowBindingConfig result() const;

private:
    struct NodeInfo {
        QString nodeId;
        QString classType;
    };

    QList<NodeInfo> m_nodes;

    QComboBox* m_inputCombo    {nullptr};
    QComboBox* m_maskCombo     {nullptr};
    QComboBox* m_positiveCombo {nullptr};
    QComboBox* m_negativeCombo {nullptr};
    QComboBox* m_kSamplerCombo {nullptr};

    void fillCombo(QComboBox* combo,
                   const QStringList& preferredClasses,
                   const QString& currentId);
};

} // namespace app::panels
