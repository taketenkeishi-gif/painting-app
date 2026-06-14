#include "app/bridge/ComfyProvider.h"

#include "platform/comfy/ComfyClient.h"

namespace app::bridge {

ComfyProvider::ComfyProvider(QObject* parent) : QObject(parent) {}

void ComfyProvider::setUrl(const QString& url) {
  m_url = url;
  if (m_client)
    m_client->setBaseUrl(QUrl(url));
}

platform::comfy::ComfyClient* ComfyProvider::client() {
  if (!m_client) {
    m_client = new platform::comfy::ComfyClient(this);
    m_client->setBaseUrl(QUrl(m_url));
  }
  return m_client;
}

} // namespace app::bridge
