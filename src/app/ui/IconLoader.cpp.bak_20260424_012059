#include "app/ui/IconLoader.h"

#include <QFile>

namespace app::ui {

QIcon icon(const QString& name, int preferredSize) {
  const QString normalized = name.trimmed();
  if (normalized.isEmpty()) {
    return {};
  }

  const QString preferredPath = QString(":/icons/%1/%2.svg").arg(preferredSize).arg(normalized);
  if (QFile::exists(preferredPath)) {
    return QIcon(preferredPath);
  }

  for (const int size : {20, 24, 16}) {
    const QString fallbackPath = QString(":/icons/%1/%2.svg").arg(size).arg(normalized);
    if (QFile::exists(fallbackPath)) {
      return QIcon(fallbackPath);
    }
  }
  return {};
}

} // namespace app::ui

