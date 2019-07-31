#ifndef LIBRARYINFO_H
#define LIBRARYINFO_H

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QHash>
#include "libraryrule.h"

namespace Ttyh {
namespace Json {
class LibraryInfo
{
public:
    explicit LibraryInfo(const QJsonObject &jObject);

    QString name;
    QList<LibraryRule> rules;
    QHash<QString, QString> natives;
};
}
}

#endif // LIBRARYINFO_H
