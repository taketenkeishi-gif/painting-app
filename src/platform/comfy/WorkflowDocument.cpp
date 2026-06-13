#include "platform/comfy/WorkflowDocument.h"

#include <QFile>
#include <QJsonDocument>

namespace platform::comfy {

// ─────────────────────────────────────────────────────────────────────────────
// ロード
// ─────────────────────────────────────────────────────────────────────────────
WorkflowDocument WorkflowDocument::load(const QString& filePath, bool* ok) {
    WorkflowDocument doc;
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        doc.m_error = QString("Cannot open: %1").arg(filePath);
        if (ok) *ok = false;
        return doc;
    }
    QJsonParseError err;
    const QJsonDocument jdoc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError) {
        doc.m_error = err.errorString();
        if (ok) *ok = false;
        return doc;
    }
    doc.m_workflow = jdoc.object();
    if (doc.m_workflow.isEmpty()) {
        doc.m_error = "JSON root is not an object";
        if (ok) *ok = false;
        return doc;
    }
    if (ok) *ok = true;
    return doc;
}

WorkflowDocument WorkflowDocument::fromJson(const QJsonObject& obj) {
    WorkflowDocument doc;
    doc.m_workflow = obj;
    return doc;
}

// ─────────────────────────────────────────────────────────────────────────────
// クエリ
// ─────────────────────────────────────────────────────────────────────────────
QStringList WorkflowDocument::nodeIds() const {
    return m_workflow.keys();
}

QStringList WorkflowDocument::findNodesByClass(const QString& classType) const {
    QStringList result;
    for (const QString& id : m_workflow.keys()) {
        if (m_workflow.value(id).toObject().value("class_type").toString() == classType) {
            result << id;
        }
    }
    return result;
}

QString WorkflowDocument::nodeClass(const QString& nodeId) const {
    return m_workflow.value(nodeId).toObject().value("class_type").toString();
}

QJsonObject WorkflowDocument::nodeInputs(const QString& nodeId) const {
    return m_workflow.value(nodeId).toObject().value("inputs").toObject();
}

// ─────────────────────────────────────────────────────────────────────────────
// パッチ操作
// ─────────────────────────────────────────────────────────────────────────────
bool WorkflowDocument::setInput(const QString& nodeId, const QString& key,
                                  const QJsonValue& value) {
    if (!m_workflow.contains(nodeId)) return false;
    QJsonObject node   = m_workflow.value(nodeId).toObject();
    QJsonObject inputs = node.value("inputs").toObject();
    inputs[key] = value;
    node["inputs"] = inputs;
    m_workflow[nodeId] = node;
    return true;
}

bool WorkflowDocument::apply(const WorkflowBinding& binding) {
    // ノードIDが指定されていれば直接使う。なければ classType 検索。
    QString resolvedId = binding.nodeId();

    if (resolvedId.isEmpty()) {
        const QString ct = binding.classType();
        if (ct.isEmpty()) return false;

        // キーのソート順で occurrence 番目を選ぶ（ComfyUI の慣例: 数値昇順）
        QStringList candidates = findNodesByClass(ct);
        // 数値キーとして昇順ソート
        std::sort(candidates.begin(), candidates.end(),
                  [](const QString& a, const QString& b) {
                      bool aOk, bOk;
                      const int ai = a.toInt(&aOk);
                      const int bi = b.toInt(&bOk);
                      if (aOk && bOk) return ai < bi;
                      return a < b;
                  });

        if (binding.occurrence() >= candidates.size()) return false;
        resolvedId = candidates.at(binding.occurrence());
    }

    if (!m_workflow.contains(resolvedId)) return false;

    QJsonObject node   = m_workflow.value(resolvedId).toObject();
    QJsonObject inputs = node.value("inputs").toObject();

    for (auto it = binding.patches().constBegin();
         it != binding.patches().constEnd(); ++it) {
        inputs[it.key()] = it.value();
    }

    node["inputs"] = inputs;
    m_workflow[resolvedId] = node;
    return true;
}

} // namespace platform::comfy
