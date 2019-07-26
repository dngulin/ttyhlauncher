#ifndef PREFIXESINDEX_H
#define PREFIXESINDEX_H


#include <QtCore/QJsonObject>
#include <QtCore/QHash>
#include "prefixinfo.h"

namespace Ttyh
{
namespace Json
{
class PrefixesIndex
{
public:
    explicit PrefixesIndex(const QJsonObject &jObject);

    QHash<QString, PrefixInfo> prefixes;

    QJsonObject toJsonObject() const;

protected:
    static constexpr const char* keyPrefixes = "prefixes";
};
}
}


#endif //PREFIXESINDEX_H
