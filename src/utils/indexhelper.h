#ifndef INDEXHELPER_H
#define INDEXHELPER_H

#include <QtCore/QString>
#include <QtCore/QJsonDocument>

namespace Ttyh {
namespace Utils {
namespace IndexHelper {
template<typename T>
T load(const QString &path)
{
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    return T(QJsonDocument::fromJson(file.readAll()).object());
}

template<typename T>
bool save(const T &index, const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    file.write(QJsonDocument(index.toJsonObject()).toJson());
    return true;
}
}
}
}

#endif // INDEXHELPER_H
