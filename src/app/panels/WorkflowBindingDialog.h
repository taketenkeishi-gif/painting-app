#pragma once

#include <QDialog>
#include <QList>
#include <QString>

#include "app/ai/WorkflowPreset.h"

class QComboBox;

namespace app::panels {

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
