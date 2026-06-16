#include "app/debug/DebugActionRegistry.h"

#ifdef PAINT_DEBUG_SERVER

namespace app::debug {

void DebugActionRegistry::registerAction(const QString& type, DebugHandler handler)
{
    m_handlers[type.toStdString()] = std::move(handler);
}

DebugActionResult DebugActionRegistry::dispatch(
    const QString& type, const QString& target, const QJsonObject& opts) const
{
    auto it = m_handlers.find(type.toStdString());
    if (it == m_handlers.end()) {
        return {false, QLatin1String("Unknown debug action: ") + type, {}};
    }
    return it->second(target, opts);
}

} // namespace app::debug

#endif // PAINT_DEBUG_SERVER
