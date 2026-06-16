#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

namespace platform::comfy { class ComfyClient; }

namespace app::bridge {

// ─────────────────────────────────────────────────────────────────────────────
// ComfyProvider
//
// ComfyClient の生成・設定を担う薄いラッパー。
// AiService が直接 platform::comfy::ComfyClient を知らなくてよいようにする。
// ─────────────────────────────────────────────────────────────────────────────
class ComfyProvider : public QObject {
  Q_OBJECT
public:
  explicit ComfyProvider(QObject* parent = nullptr);

  void    setUrl(const QString& url);
  QString url() const { return m_url; }

  // AiGenerationController に渡す ComfyClient を返す（遅延初期化）。
  platform::comfy::ComfyClient* client();

private:
  QString                        m_url {"http://localhost:8188"};
  platform::comfy::ComfyClient*  m_client {nullptr};
};

} // namespace app::bridge
