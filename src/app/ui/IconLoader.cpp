#include "app/ui/IconLoader.h"

#include <QCoreApplication>
#include <QFile>
#include <QIcon>
#include <QStringList>

namespace app::ui {

static QString findIcon(const QString& name, int size)
{
    const QString appDir = QCoreApplication::applicationDirPath();

    const QStringList candidates {
        QString(":/icons/%1/%2.svg").arg(size).arg(name),
        QString("%1/../../../src/app/resources/icons/%2/%3.svg").arg(appDir).arg(size).arg(name),
        QString("%1/../../src/app/resources/icons/%2/%3.svg").arg(appDir).arg(size).arg(name),
        QString("%1/icons/%2/%3.svg").arg(appDir).arg(size).arg(name)
    };

    for (const auto& p : candidates) {
        if (QFile::exists(p)) return p;
    }

    return {};
}

QIcon icon(const QString& name, int size)
{
    QString path = findIcon(name, size);

    if (!path.isEmpty()) {
        return QIcon(path);
    }

    // fallback sizes
    for (int s : {24,20,16}) {
        path = findIcon(name, s);
        if (!path.isEmpty()) {
            return QIcon(path);
        }
    }

    return QIcon();
}

} // namespace app::ui
