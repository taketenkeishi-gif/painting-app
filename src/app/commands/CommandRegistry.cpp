#include "app/commands/CommandRegistry.h"

#include <QAction>
#include <QSet>
#include <QSettings>

namespace app::commands {

CommandRegistry& CommandRegistry::instance() {
    static CommandRegistry reg;
    return reg;
}

CommandRegistry::CommandRegistry() = default;

void CommandRegistry::registerCommand(const CommandDef& def) {
    m_defs.insert(def.id, def);
}

const CommandDef* CommandRegistry::find(const QString& id) const {
    auto it = m_defs.constFind(id);
    return it != m_defs.constEnd() ? &it.value() : nullptr;
}

QList<CommandDef> CommandRegistry::all() const {
    return m_defs.values();
}

QList<QString> CommandRegistry::categories() const {
    QSet<QString> cats;
    for (const auto& def : m_defs) {
        cats.insert(def.category);
    }
    return QList<QString>(cats.begin(), cats.end());
}

QKeySequence CommandRegistry::effectiveShortcut(const QString& id) const {
    auto it = m_overrides.constFind(id);
    if (it != m_overrides.constEnd()) {
        return it.value();
    }
    const CommandDef* def = find(id);
    return def ? def->defaultShortcut : QKeySequence{};
}

void CommandRegistry::saveOverride(const QString& id, const QKeySequence& seq) {
    if (id.isEmpty()) {
        return;
    }
    m_overrides.insert(id, seq);
    QSettings settings("taketenkeishi", "LayeredPaintApp");
    settings.beginGroup("shortcuts");
    settings.setValue(id, seq.toString(QKeySequence::PortableText));
    settings.endGroup();
    emit shortcutChanged(id, seq);
}

void CommandRegistry::loadOverrides() {
    QSettings settings("taketenkeishi", "LayeredPaintApp");
    settings.beginGroup("shortcuts");
    const QStringList keys = settings.childKeys();
    for (const QString& key : keys) {
        if (!m_defs.contains(key)) {
            continue;
        }
        const QString saved = settings.value(key).toString();
        m_overrides.insert(key, QKeySequence::fromString(saved, QKeySequence::PortableText));
    }
    settings.endGroup();
}

QAction* CommandRegistry::createAction(const QString& id, QObject* parent) {
    const CommandDef* def = find(id);
    const QString text = def ? def->displayName : id;
    auto* action = new QAction(text, parent);
    action->setProperty("commandId", id);
    action->setShortcut(effectiveShortcut(id));
    return action;
}

// ── Standard command catalogue ───────────────────────────────────────────────
//
// Each entry: { id, displayName, category, defaultShortcut }
// When adding a new command, append here — do NOT create QAction directly.

void registerAllAppCommands() {
    auto& reg = CommandRegistry::instance();

    // ── select ────────────────────────────────────────────────────────────────
    reg.registerCommand({"select.quick_mask",
                         QString::fromUtf8(u8"クイックマスクモード"),
                         "select",
                         QKeySequence(Qt::Key_Q)});
    reg.registerCommand({"select.all",
                         QString::fromUtf8(u8"すべて選択"),
                         "select",
                         QKeySequence::SelectAll});
    reg.registerCommand({"select.deselect",
                         QString::fromUtf8(u8"選択解除"),
                         "select",
                         QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D)});
    reg.registerCommand({"select.invert",
                         QString::fromUtf8(u8"選択範囲を反転"),
                         "select",
                         QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I)});
    reg.registerCommand({"select.clear",
                         QString::fromUtf8(u8"選択範囲をクリア"),
                         "select",
                         QKeySequence(Qt::CTRL | Qt::Key_D)});
    reg.registerCommand({"select.expand",
                         QString::fromUtf8(u8"選択範囲を拡張..."),
                         "select",
                         QKeySequence{}});
    reg.registerCommand({"select.contract",
                         QString::fromUtf8(u8"選択範囲を縮小..."),
                         "select",
                         QKeySequence{}});

    // ── layer ─────────────────────────────────────────────────────────────────
    reg.registerCommand({"layer.merge_down",
                         QString::fromUtf8(u8"下のレイヤーと結合"),
                         "layer",
                         QKeySequence(Qt::CTRL | Qt::Key_E)});
    reg.registerCommand({"layer.merge_selected",
                         QString::fromUtf8(u8"選択中のレイヤーを結合"),
                         "layer",
                         QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E)});
    reg.registerCommand({"layer.merge_visible",
                         QString::fromUtf8(u8"表示レイヤーを結合"),
                         "layer",
                         QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_E)});
    reg.registerCommand({"layer.add_raster",
                         QString::fromUtf8(u8"新規ラスターレイヤー"),
                         "layer",
                         QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N)});
    reg.registerCommand({"layer.add_vector",
                         QString::fromUtf8(u8"新規ベクターレイヤー"),
                         "layer",
                         QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_N)});
    reg.registerCommand({"layer.add_folder",
                         QString::fromUtf8(u8"新規フォルダーレイヤー"),
                         "layer",
                         QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G)});
    reg.registerCommand({"layer.duplicate",
                         QString::fromUtf8(u8"レイヤーを複製"),
                         "layer",
                         QKeySequence(Qt::CTRL | Qt::Key_J)});
    reg.registerCommand({"layer.delete",
                         QString::fromUtf8(u8"レイヤーを削除"),
                         "layer",
                         QKeySequence(Qt::CTRL | Qt::Key_Delete)});
    reg.registerCommand({"layer.move_up",
                         QString::fromUtf8(u8"レイヤーを上へ移動"),
                         "layer",
                         QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Up)});
    reg.registerCommand({"layer.move_down",
                         QString::fromUtf8(u8"レイヤーを下へ移動"),
                         "layer",
                         QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Down)});
    reg.registerCommand({"layer.toggle_visible",
                         QString::fromUtf8(u8"表示/非表示を切替"),
                         "layer",
                         QKeySequence(Qt::Key_V)});
    reg.registerCommand({"layer.clip_toggle",
                         QString::fromUtf8(u8"クリッピングを切替"),
                         "layer",
                         QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_C)});
    reg.registerCommand({"layer.mask_toggle",
                         QString::fromUtf8(u8"マスクを切替"),
                         "layer",
                         QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_M)});
    reg.registerCommand({"layer.mask_remove",
                         QString::fromUtf8(u8"マスクを削除"),
                         "layer",
                         QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_M)});
    reg.registerCommand({"layer.lock_toggle",
                         QString::fromUtf8(u8"ロックを切替"),
                         "layer",
                         QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_L)});
    reg.registerCommand({"layer.alpha_lock_toggle",
                         QString::fromUtf8(u8"透明保護を切替"),
                         "layer",
                         QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_L)});
    reg.registerCommand({"layer.position_lock_toggle",
                         QString::fromUtf8(u8"位置固定を切替"),
                         "layer",
                         QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_P)});
    reg.registerCommand({"layer.rasterize",
                         QString::fromUtf8(u8"ラスタライズ"),
                         "layer",
                         QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R)});

    // ── ai ───────────────────────────────────────────────────────────────────
    reg.registerCommand({"ai.generative_fill",
                         QString::fromUtf8(u8"AI 生成塗りつぶし..."),
                         "ai",
                         QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_G)});
    reg.registerCommand({"ai.connect_comfy",
                         QString::fromUtf8(u8"ComfyUI に接続..."),
                         "ai",
                         QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Y)});
    reg.registerCommand({"ai.upscale",
                         QString::fromUtf8(u8"AI 高解像度化..."),
                         "ai",
                         QKeySequence{}});
}

} // namespace app::commands
