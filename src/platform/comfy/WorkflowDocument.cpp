#include "platform/comfy/WorkflowDocument.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

namespace platform::comfy {

// ─────────────────────────────────────────────────────────────────────────────
// UI 形式 → API 形式 正規化ヘルパー
// ─────────────────────────────────────────────────────────────────────────────
namespace {

/// ComfyUI GUI からエクスポートした workflow.json（UI 形式）を
/// API 形式（nodeId → {class_type, inputs}）に変換する。
///
/// UI 形式の links 配列: [link_id, src_node, src_slot, dst_node, dst_slot, type]
/// 接続された入力 → inputs[name] = [src_node_id_str, src_slot]
/// widget 入力   → inputs[name] = widgets_values[widget_index] (順番通り)
QJsonObject normalizeUiWorkflow(const QJsonObject& uiRoot) {
    // ── 1. links テーブル構築: link_id → [src_node_id, src_slot] ──────────
    // links 配列の各要素: [id, from_node, from_slot, to_node, to_slot, type]
    QHash<int, QJsonArray> linkMap;  // link_id → [src_node_str, src_slot_int]
    for (const QJsonValue& lv : uiRoot.value(QLatin1String("links")).toArray()) {
        const QJsonArray l = lv.toArray();
        if (l.size() < 6) continue;
        const int linkId   = l.at(0).toInt();
        const int srcNode  = l.at(1).toInt();
        const int srcSlot  = l.at(2).toInt();
        QJsonArray ref;
        ref.append(QString::number(srcNode));
        ref.append(srcSlot);
        linkMap[linkId] = ref;
    }

    // ── 2. 各ノードを変換 ─────────────────────────────────────────────────
    QJsonObject api;
    for (const QJsonValue& nv : uiRoot.value(QLatin1String("nodes")).toArray()) {
        const QJsonObject n  = nv.toObject();
        const QString id     = QString::number(n.value(QLatin1String("id")).toInt());
        const QString cls    = n.value(QLatin1String("type")).toString();
        const QJsonArray wv  = n.value(QLatin1String("widgets_values")).toArray();
        const QJsonArray ins = n.value(QLatin1String("inputs")).toArray();

        QJsonObject inputs;
        int widgetIdx = 0;  // widgets_values の消費カウンタ

        // ComfyUI の seed 系入力は widgets_values を 2 スロット消費する
        // (実値 + control_after_generate トークン 'randomize'/'fixed' 等)。
        static const QStringList kControlTokens{
            "randomize", "fixed", "increment", "decrement"};

        for (const QJsonValue& iv : ins) {
            const QJsonObject inp = iv.toObject();
            const QString name    = inp.value(QLatin1String("name")).toString();
            if (name.isEmpty()) continue;

            const QJsonValue linkVal = inp.value(QLatin1String("link"));
            if (!linkVal.isNull() && linkVal.isDouble()) {
                // ノード接続: リンクテーブルから参照を取得
                const int linkId = linkVal.toInt();
                if (linkMap.contains(linkId))
                    inputs[name] = linkMap[linkId];
            } else {
                // widget 値: widgets_values から順番に取得
                if (inp.contains(QLatin1String("widget"))) {
                    if (widgetIdx < wv.size())
                        inputs[name] = wv.at(widgetIdx++);
                    // seed 系入力は直後に制御トークンを持つ → 追加で 1 スロット消費
                    const bool isSeed = name.contains(QLatin1String("seed"), Qt::CaseInsensitive);
                    if (isSeed && widgetIdx < wv.size()
                            && kControlTokens.contains(wv.at(widgetIdx).toString()))
                        ++widgetIdx;  // control token をスキップ
                }
                // link も widget も持たない入力はスキップ
            }
        }

        // inputs に来なかった widgets_values を class_type 別ヒューリスティックで補完
        // （inputs 配列を持たないノード向け）
        if (ins.isEmpty()) {
            if (cls.contains(QLatin1String("TagEditor"), Qt::CaseInsensitive)) {
                if (!wv.isEmpty() && wv.at(0).isString()) {
                    const QString raw = wv.at(0).toString();
                    inputs[QLatin1String(raw.startsWith(QLatin1Char('{'))
                                            ? "editor_state" : "text")] = raw;
                }
            } else if (cls == QLatin1String("LoadImage")) {
                if (!wv.isEmpty()) inputs[QLatin1String("image")] = wv.at(0);
            } else if (!wv.isEmpty()
                    && (cls.contains(QLatin1String("Checkpoint"), Qt::CaseInsensitive)
                     || cls.contains(QLatin1String("ModelStack"),  Qt::CaseInsensitive)
                     || (cls.contains(QLatin1String("Loader"), Qt::CaseInsensitive)
                         && cls.contains(QLatin1String("Model"), Qt::CaseInsensitive)))) {
                if (wv.at(0).isString())
                    inputs[QLatin1String("ckpt_name")] = wv.at(0);
            }
        }

        QJsonObject node;
        node[QLatin1String("class_type")]     = cls;
        node[QLatin1String("inputs")]         = inputs;
        node[QLatin1String("widgets_values")] = wv;
        api[id] = node;
    }
    return api;
}

} // namespace

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
    // UI 形式（ComfyUI GUI エクスポート）を API 形式に正規化する
    if (doc.m_workflow.contains(QLatin1String("nodes"))
     && doc.m_workflow.value(QLatin1String("nodes")).isArray()) {
        doc.m_workflow = normalizeUiWorkflow(doc.m_workflow);
    }
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

QString WorkflowDocument::findMaskSourceNode() const {
    // ImageToMask ノードの image 入力を逆引きしてマスク供給ノードを特定する。
    // inputs.image は ["nodeId", outputSlot] 形式のノード参照。
    for (const QString& id : m_workflow.keys()) {
        const QJsonObject node = m_workflow.value(id).toObject();
        if (node.value(QLatin1String("class_type")).toString()
                != QLatin1String("ImageToMask"))
            continue;
        const QJsonArray imageInput =
            node.value(QLatin1String("inputs")).toObject()
                .value(QLatin1String("image")).toArray();
        if (!imageInput.isEmpty()) {
            return imageInput.first().toString();
        }
    }
    return {};
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

// ─────────────────────────────────────────────────────────────────────────────
// 全ノードスキャン (class_type 非依存)
// ─────────────────────────────────────────────────────────────────────────────
QStringList WorkflowDocument::findNodesByInputName(const QString& inputName) const {
    QStringList result;
    for (const QString& id : m_workflow.keys()) {
        if (nodeInputs(id).contains(inputName))
            result << id;
    }
    return result;
}

QStringList WorkflowDocument::findSamplerNodes() const {
    static const QStringList kFields{
        "steps", "cfg", "denoise", "seed", "noise_seed", "guidance_scale", "strength"};
    QStringList result;
    for (const QString& id : m_workflow.keys()) {
        const QJsonObject inp = nodeInputs(id);
        int matches = 0;
        for (const QString& f : kFields)
            if (inp.contains(f) && !inp.value(f).isArray()) ++matches;
        if (matches >= 2) { result << id; continue; }
        // class_type ヒューリスティック（UI 形式正規化後でも inputs フィールドが少ない Sampler 系）
        const QString cls = nodeClass(id);
        if ((cls.contains(QLatin1String("Sampler"), Qt::CaseInsensitive)
          || cls.contains(QLatin1String("Diffusion"), Qt::CaseInsensitive))
         && !result.contains(id))
            result << id;
    }
    return result;
}

QStringList WorkflowDocument::findPromptNodes() const {
    static const QStringList kFields{
        "text", "prompt", "positive", "tags", "editor_state"};
    QStringList result;
    for (const QString& id : m_workflow.keys()) {
        const QJsonObject inp = nodeInputs(id);
        for (const QString& f : kFields) {
            if (inp.contains(f)) { result << id; break; }
        }
    }
    return result;
}

QStringList WorkflowDocument::findCheckpointNodes() const {
    static const QStringList kFields{
        "ckpt_name", "checkpoint", "model_name"};
    QStringList result;
    for (const QString& id : m_workflow.keys()) {
        const QJsonObject inp = nodeInputs(id);
        bool matched = false;
        for (const QString& f : kFields) {
            if (inp.contains(f)) { matched = true; break; }
        }
        if (matched) { result << id; continue; }
        // class_type fallback（ckpt_name マッピングが当たらなかった Loader/Checkpoint 系の保険）
        const QString cls = nodeClass(id);
        if (cls.contains(QLatin1String("Checkpoint"), Qt::CaseInsensitive)
         || cls.contains(QLatin1String("ModelStack"),  Qt::CaseInsensitive)
         || (cls.contains(QLatin1String("Loader"), Qt::CaseInsensitive)
             && cls.contains(QLatin1String("Model"), Qt::CaseInsensitive)))
            result << id;
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// 値読み取り (inputs / widgets_values + SDXLTagEditor 対応)
// ─────────────────────────────────────────────────────────────────────────────
namespace {

// SDXLTagEditor の editor_state JSON を解析し {positive, negative} を返す。
QPair<QString, QString> parseEditorState(const QString& raw) {
    const QJsonDocument jdoc = QJsonDocument::fromJson(raw.toUtf8());
    if (jdoc.isNull() || !jdoc.isObject()) return {};
    const QJsonObject obj = jdoc.object();

    QStringList posParts;
    const QString prefix = obj.value("prefix").toString().trimmed();
    if (!prefix.isEmpty()) posParts << prefix;
    for (const QJsonValue& v : obj.value("mainTags").toArray()) {
        const QString t = v.toString().trimmed();
        if (!t.isEmpty()) posParts << t;
    }

    QStringList negParts;
    for (const QJsonValue& v : obj.value("negative").toArray()) {
        const QString t = v.toString().trimmed();
        if (!t.isEmpty()) negParts << t;
    }

    return {posParts.join(", "), negParts.join(", ")};
}

} // namespace

QString WorkflowDocument::readPositiveText(const QString& nodeId) const {
    const QJsonObject node   = m_workflow.value(nodeId).toObject();
    const QJsonObject inputs = node.value("inputs").toObject();

    // editor_state (SDXLTagEditor): prefix + mainTags
    if (inputs.contains("editor_state") && inputs.value("editor_state").isString()) {
        const auto [pos, neg] = parseEditorState(inputs.value("editor_state").toString());
        if (!pos.isEmpty()) return pos;
    }

    // 標準フィールド
    for (const char* key : {"text", "prompt", "positive", "tags"}) {
        const QJsonValue v = inputs.value(QLatin1String(key));
        if (v.isString() && !v.toString().isEmpty()) return v.toString();
    }

    // UI 形式 fallback: widgets_values[0]
    const QJsonArray wv = node.value("widgets_values").toArray();
    if (!wv.isEmpty() && wv.at(0).isString()) return wv.at(0).toString();

    return {};
}

QString WorkflowDocument::readNegativeText(const QString& nodeId) const {
    const QJsonObject node   = m_workflow.value(nodeId).toObject();
    const QJsonObject inputs = node.value("inputs").toObject();

    // editor_state (SDXLTagEditor): negative タグ
    if (inputs.contains("editor_state") && inputs.value("editor_state").isString()) {
        const auto [pos, neg] = parseEditorState(inputs.value("editor_state").toString());
        if (!neg.isEmpty()) return neg;
    }

    // 標準フィールド (negative ノードは text か prompt)
    for (const char* key : {"text", "prompt", "negative"}) {
        const QJsonValue v = inputs.value(QLatin1String(key));
        if (v.isString() && !v.toString().isEmpty()) return v.toString();
    }

    const QJsonArray wv = node.value("widgets_values").toArray();
    if (!wv.isEmpty() && wv.at(0).isString()) return wv.at(0).toString();

    return {};
}

QString WorkflowDocument::readClipText(const QString& nodeId) const {
    return readPositiveText(nodeId);
}

WorkflowDocument::KSamplerParams WorkflowDocument::readKSamplerParams(const QString& nodeId) const {
    KSamplerParams p;
    const QJsonObject node   = m_workflow.value(nodeId).toObject();
    const QJsonObject inputs = node.value("inputs").toObject();

    // フィールド名スキャン (seed/noise_seed / steps / cfg/guidance_scale / denoise/strength)
    auto readInt = [&](std::initializer_list<const char*> keys, int def) -> int {
        for (const char* k : keys) {
            const QJsonValue v = inputs.value(QLatin1String(k));
            if (v.isDouble()) return static_cast<int>(v.toDouble());
        }
        return def;
    };
    auto readDouble = [&](std::initializer_list<const char*> keys, double def) -> double {
        for (const char* k : keys) {
            const QJsonValue v = inputs.value(QLatin1String(k));
            if (v.isDouble()) return v.toDouble();
        }
        return def;
    };

    bool hasSeed    = inputs.contains("seed")    || inputs.contains("noise_seed");
    bool hasSteps   = inputs.contains("steps");
    bool hasCfg     = inputs.contains("cfg")     || inputs.contains("guidance_scale");
    bool hasDenoise = inputs.contains("denoise") || inputs.contains("strength");

    if (hasSeed)    p.seed    = readInt({"seed", "noise_seed"}, -1);
    if (hasSteps)   p.steps   = readInt({"steps"}, 20);
    if (hasCfg)     p.cfg     = readDouble({"cfg", "guidance_scale"}, 7.5);
    if (hasDenoise) p.denoise = readDouble({"denoise", "strength"}, 0.75);

    // widgets_values fallback (KSampler UI 形式: [seed, ctrl, steps, cfg, sampler, sched, denoise])
    const QJsonArray wv = node.value("widgets_values").toArray();
    if (!hasSeed    && wv.size() > 0) p.seed    = static_cast<int>(wv.at(0).toDouble(-1));
    if (!hasSteps   && wv.size() > 2) p.steps   = static_cast<int>(wv.at(2).toDouble(20));
    if (!hasCfg     && wv.size() > 3) p.cfg     = wv.at(3).toDouble(7.5);
    if (!hasDenoise && wv.size() > 6) p.denoise = wv.at(6).toDouble(0.75);

    return p;
}

QString WorkflowDocument::readCheckpointName() const {
    static const QStringList kFields{"ckpt_name", "checkpoint", "model_name"};

    // inputs スキャン (class_type 非依存)
    for (const QString& id : m_workflow.keys()) {
        const QJsonObject inp = nodeInputs(id);
        for (const QString& f : kFields) {
            if (inp.contains(f) && inp.value(f).isString()) {
                const QString v = inp.value(f).toString();
                if (!v.isEmpty()) return v;
            }
        }
    }

    // widgets_values fallback: class 名に Checkpoint / Loader を含むノード
    for (const QString& id : m_workflow.keys()) {
        const QString cls = nodeClass(id);
        if (cls.contains("Checkpoint", Qt::CaseInsensitive)
         || (cls.contains("Loader", Qt::CaseInsensitive)
             && cls.contains("Model", Qt::CaseInsensitive))) {
            const QJsonArray wv = m_workflow.value(id).toObject()
                                      .value("widgets_values").toArray();
            if (!wv.isEmpty() && wv.at(0).isString()) return wv.at(0).toString();
        }
    }
    return {};
}

} // namespace platform::comfy
