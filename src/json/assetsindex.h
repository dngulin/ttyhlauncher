#ifndef ASSETSINDEX_H
#define ASSETSINDEX_H

#include <QtCore/QJsonObject>
#include <QtCore/QHash>
#include "checkinfo.h"

namespace Ttyh {
namespace Json {
class AssetsIndex
{
public:
    explicit AssetsIndex(const QJsonObject &jObject);

    QHash<QString, CheckInfo> objects;

    bool isValid() const;
};
}
}

#endif // ASSETSINDEX_H
