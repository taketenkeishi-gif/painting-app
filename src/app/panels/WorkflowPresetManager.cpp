#include "app/panels/WorkflowPresetManager.h"

#include <QCryptographicHash>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>

namespace app::panels {

namespace {
const char kPresetsKey[]    = "ai/presets";
const char kLastIdxKey[]    = "ai/lastPresetIndex";
const char kWfCachePrefix[] = "ai/wfcache/";

// 旧フォーマットのキー (移行後も読み取るが書き込まない)
const char kLegacyRecentKey[]  = "ai/recentWorkflows";
const char kLegacyBindPrefix[] = "ai/binding/";
const char kLegacyLastPathKey[]= "ai/lastWorkflowPath";

QString cacheSettingsKey(const QString& path) {
    return QString::fromLatin1(kWfCachePrefix)
         + QString::number(qHash(path));
}
} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// WorkflowPreset serialization
// ─────────────────────────────────────────────────────────────────────────────

QJsonObject WorkflowPreset::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("name")]         = name;
    obj[QStringLiteral("type")]         = static_cast<int>(type);
    obj[QStringLiteral("workflowPath")] = workflowPath;

    QJsonObject b;
    b[QStringLiteral("inputImageNodeId")] = binding.inputImageNodeId;
    b[QStringLiteral("maskNodeId")]       = binding.maskNodeId;
    b[QStringLiteral("positiveNodeId")]   = binding.positiveNodeId;
    b[QStringLiteral("negativeNodeId")]   = binding.negativeNodeId;
    b[QStringLiteral("kSamplerNodeId")]   = binding.kSamplerNodeId;
    obj[QStringLiteral("binding")] = b;
    return obj;
}

WorkflowPreset WorkflowPreset::fromJson(const QJsonObject& obj) {
    WorkflowPreset p;
    p.name         = obj[QStringLiteral("name")].toString();
    p.type         = static_cast<WorkflowType>(obj[QStringLiteral("type")].toInt(0));
    p.workflowPath = obj[QStringLiteral("workflowPath")].toString();

    const QJsonObject b = obj[QStringLiteral("binding")].toObject();
    p.binding.inputImageNodeId = b[QStringLiteral("inputImageNodeId")].toString();
    p.binding.maskNodeId       = b[QStringLiteral("maskNodeId")].toString();
    p.binding.positiveNodeId   = b[QStringLiteral("positiveNodeId")].toString();
    p.binding.negativeNodeId   = b[QStringLiteral("negativeNodeId")].toString();
    p.binding.kSamplerNodeId   = b[QStringLiteral("kSamplerNodeId")].toString();
    return p;
}

// ─────────────────────────────────────────────────────────────────────────────
// WorkflowPresetManager
// ─────────────────────────────────────────────────────────────────────────────

void WorkflowPresetManager::load() {
    QSettings s;

    // ── 新フォーマット ────────────────────────────────────────────────────────
    const QByteArray raw = s.value(QLatin1String(kPresetsKey)).toByteArray();
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (err.error == QJsonParseError::NoError && doc.isArray()) {
        m_presets.clear();
        for (const QJsonValue& v : doc.array()) {
            if (v.isObject()) {
                WorkflowPreset p = WorkflowPreset::fromJson(v.toObject());
                if (p.isValid()) m_presets.append(p);
            }
        }
    }

    m_lastUsedIndex = s.value(QLatin1String(kLastIdxKey), -1).toInt();
    if (m_lastUsedIndex >= m_presets.size()) m_lastUsedIndex = -1;

    // ── 旧フォーマット移行（初回のみ） ────────────────────────────────────────
    if (m_presets.isEmpty()) {
        const QStringList oldRecent = s.value(QLatin1String(kLegacyRecentKey)).toStringList();
        for (const QString& path : oldRecent) {
            const QString hash = QString::fromLatin1(
                QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Md5).toHex());
            s.beginGroup(QLatin1String(kLegacyBindPrefix) + hash);
            WorkflowPreset p;
            p.name                    = QFileInfo(path).fileName();
            p.workflowPath            = path;
            p.binding.inputImageNodeId= s.value(QStringLiteral("inputImageNode")).toString();
            p.binding.maskNodeId      = s.value(QStringLiteral("maskNode")).toString();
            p.binding.positiveNodeId  = s.value(QStringLiteral("positiveNode")).toString();
            p.binding.negativeNodeId  = s.value(QStringLiteral("negativeNode")).toString();
            p.binding.kSamplerNodeId  = s.value(QStringLiteral("kSamplerNode")).toString();
            s.endGroup();
            p.type = p.inferType();
            m_presets.append(p);
        }

        // 旧形式の最終選択パスから index を復元
        const QString lastPath = s.value(QLatin1String(kLegacyLastPathKey)).toString();
        m_lastUsedIndex = indexByPath(lastPath);

        if (!m_presets.isEmpty()) save();  // 新フォーマットに書き出す
    }
}

void WorkflowPresetManager::save() const {
    QSettings s;

    QJsonArray arr;
    for (const WorkflowPreset& p : m_presets) arr.append(p.toJson());
    s.setValue(QLatin1String(kPresetsKey),
               QJsonDocument(arr).toJson(QJsonDocument::Compact));
    s.setValue(QLatin1String(kLastIdxKey), m_lastUsedIndex);

    // キャッシュされたワークフロー JSON を書き出す
    for (auto it = m_workflowCache.cbegin(); it != m_workflowCache.cend(); ++it) {
        s.setValue(cacheSettingsKey(it.key()),
                   QJsonDocument(it.value()).toJson(QJsonDocument::Compact));
    }
}

void WorkflowPresetManager::addOrUpdate(const WorkflowPreset& preset) {
    const int existing = indexByPath(preset.workflowPath);
    if (existing >= 0) {
        m_presets[existing] = preset;
    } else {
        m_presets.prepend(preset);
        while (m_presets.size() > kMaxPresets) m_presets.removeLast();
    }
    save();
}

void WorkflowPresetManager::removeAt(int index) {
    if (index < 0 || index >= m_presets.size()) return;
    m_presets.removeAt(index);
    if (m_lastUsedIndex >= m_presets.size()) m_lastUsedIndex = -1;
    save();
}

int WorkflowPresetManager::indexByPath(const QString& path) const {
    for (int i = 0; i < m_presets.size(); ++i) {
        if (m_presets[i].workflowPath == path) return i;
    }
    return -1;
}

int WorkflowPresetManager::indexByName(const QString& name) const {
    for (int i = 0; i < m_presets.size(); ++i) {
        if (m_presets[i].name == name) return i;
    }
    return -1;
}

void WorkflowPresetManager::setLastUsedIndex(int i) {
    m_lastUsedIndex = i;
    QSettings{}.setValue(QLatin1String(kLastIdxKey), i);
}

QJsonObject WorkflowPresetManager::cachedWorkflow(const QString& path) const {
    if (m_workflowCache.contains(path)) return m_workflowCache.value(path);

    QSettings s;
    const QByteArray raw = s.value(cacheSettingsKey(path)).toByteArray();
    if (raw.isEmpty()) return {};
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (err.error == QJsonParseError::NoError && doc.isObject()) {
        m_workflowCache.insert(path, doc.object());
        return doc.object();
    }
    return {};
}

void WorkflowPresetManager::cacheWorkflow(const QString& path, const QJsonObject& json) {
    m_workflowCache.insert(path, json);
    QSettings{}.setValue(cacheSettingsKey(path),
                         QJsonDocument(json).toJson(QJsonDocument::Compact));
}

} // namespace app::panels
