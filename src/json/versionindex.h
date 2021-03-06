#ifndef VERSIONINDEX_H
#define VERSIONINDEX_H

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QDateTime>
#include "libraryinfo.h"
#include "argumentinfo.h"

namespace Ttyh {
namespace Json {
class VersionIndex
{
public:
    explicit VersionIndex(const QJsonObject &jObject);

    QString id;
    QDateTime releaseTime;

    QString assetsIndex;
    QList<LibraryInfo> libraries;

    QString mainClass;
    QStringList gameArguments;
    QList<ArgumentInfo> javaArguments;

    bool isValid() const;
};
}
}

#endif // VERSIONINDEX_H
