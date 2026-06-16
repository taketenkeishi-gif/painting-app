#pragma once

#include <QHash>
#include <QKeySequence>
#include <QList>
#include <QObject>
#include <QString>

class QAction;

namespace app::commands {

struct CommandDef {
    QString      id;
    QString      displayName;
    QString      category;
    QKeySequence defaultShortcut;
};

/// Central registry for all application commands.
///
/// One place to register id → (displayName, category, defaultShortcut).
/// Use createAction() instead of new QAction() so shortcuts and commandId
/// properties are always consistent.
class CommandRegistry : public QObject {
    Q_OBJECT
public:
    static CommandRegistry& instance();

    void              registerCommand(const CommandDef& def);
    const CommandDef* find(const QString& id) const;
    QList<CommandDef> all() const;
    QList<QString>    categories() const;

    /// Creates a QAction with displayName, commandId property, and effective shortcut.
    QAction* createAction(const QString& id, QObject* parent);

    /// Returns the user-saved override, or the default shortcut if none exists.
    QKeySequence effectiveShortcut(const QString& id) const;

    /// Persists a shortcut override to QSettings and updates the in-memory cache.
    void saveOverride(const QString& id, const QKeySequence& seq);

    /// Loads all saved shortcut overrides from QSettings into the in-memory cache.
    /// Call once at startup before any createAction() calls.
    void loadOverrides();

signals:
    void shortcutChanged(const QString& id, const QKeySequence& seq);

private:
    CommandRegistry();
    QHash<QString, CommandDef>   m_defs;
    QHash<QString, QKeySequence> m_overrides;
};

/// Registers the full set of standard application commands.
/// Called once during MainWindow construction.
void registerAllAppCommands();

} // namespace app::commands
