#ifndef PREFIXVERSIONSINDEX_H
#define PREFIXVERSIONSINDEX_H

#include <QtCore/QJsonObject>
#include <QtCore/QStringList>

namespace Ttyh {
namespace Json {
class PrefixVersionsIndex
{
public:
    explicit PrefixVersionsIndex(const QJsonObject &jObject);

    QString latest;
    QStringList versions;
};
}
}

#endif // PREFIXVERSIONSINDEX_H
