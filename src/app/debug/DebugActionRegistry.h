#pragma once
/**
 * DebugActionRegistry — debug action 登録・ディスパッチ責務を AppController から分離。
 *
 * PAINT_DEBUG_SERVER ビルド専用。
 * AppController は initDebugActions() でハンドラーを登録し、
 * executeDebugAction() はレジストリに委譲するだけになる。
 *
 * ハンドラーは AppController* this をキャプチャするラムダとして登録されるため、
 * AppController のプライベートメンバーへのアクセスは変わらず可能。
 */

#ifdef PAINT_DEBUG_SERVER

#include <functional>
#include <unordered_map>

#include <QString>
#include <QJsonObject>

namespace app::debug {

// ── DebugActionResult ────────────────────────────────────────────────────────
// AppController::DebugActionResult は本型の using エイリアスとして定義される。
struct DebugActionResult {
    bool        success {false};
    QString     message;
    QJsonObject data;
};

// ── DebugHandler ─────────────────────────────────────────────────────────────
// 各 action ハンドラーのシグネチャ。
using DebugHandler = std::function<DebugActionResult(const QString& target,
                                                     const QJsonObject& opts)>;

// ── DebugActionRegistry ──────────────────────────────────────────────────────
class DebugActionRegistry {
public:
    /// action type を登録する。同名は上書き。
    void registerAction(const QString& type, DebugHandler handler);

    /// type にマッチするハンドラーを呼び出す。
    /// 未登録の場合は { success=false, "Unknown debug action: <type>" } を返す。
    DebugActionResult dispatch(const QString& type,
                               const QString& target,
                               const QJsonObject& opts) const;

private:
    std::unordered_map<std::string, DebugHandler> m_handlers;
};

} // namespace app::debug

#endif // PAINT_DEBUG_SERVER
