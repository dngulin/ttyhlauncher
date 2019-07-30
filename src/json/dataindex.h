#ifndef DATAINDEX_H
#define DATAINDEX_H


#include <QtCore/QJsonObject>
#include <QtCore/QHash>
#include <QtCore/QStringList>
#include "checkinfo.h"

namespace Ttyh
{
namespace Json
{
class DataIndex
{
public:
    explicit DataIndex(const QJsonObject &jObject);

    CheckInfo main;
    QHash<QString, CheckInfo> libs;
    QHash<QString, CheckInfo> files;
    QStringList mutableFiles;
};
}
}


#endif //DATAINDEX_H
