#include "app/ui/IconLoader.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QStringList>

namespace app::ui {

namespace {

QString firstExistingPath(const QStringList& candidates) {
  for (const QString& path : candidates) {
    if (QFile::exists(path)) {
      return path;
    }
  }
  return {};
}

} // namespace

QIcon icon(const QString& name) {
  const QString appDir = QCoreApplication::applicationDirPath();

  const QStringList candidates {
      QString(":/icons/24/%1.svg").arg(name),
      QString(":/icons/20/%1.svg").arg(name),
      QString(":/icons/16/%1.svg").arg(name),

      QString("%1/../../../src/app/resources/icons/24/%2.svg").arg(appDir, name),
      QString("%1/../../../src/app/resources/icons/20/%2.svg").arg(appDir, name),
      QString("%1/../../../src/app/resources/icons/16/%2.svg").arg(appDir, name),

      QString("%1/../../src/app/resources/icons/24/%2.svg").arg(appDir, name),
      QString("%1/../../src/app/resources/icons/20/%2.svg").arg(appDir, name),
      QString("%1/../../src/app/resources/icons/16/%2.svg").arg(appDir, name),

      QString("%1/icons/24/%2.svg").arg(appDir, name),
      QString("%1/icons/20/%2.svg").arg(appDir, name),
      QString("%1/icons/16/%2.svg").arg(appDir, name)
  };

  const QString resolved = firstExistingPath(candidates);
  if (!resolved.isEmpty()) {
    return QIcon(resolved);
  }

  return QIcon();
}

} // namespace app::ui
