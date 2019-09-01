#ifndef ARGUMENTINFO_H
#define ARGUMENTINFO_H

#include <QtCore/QJsonObject>
#include <QtCore/QStringList>
#include "libraryrule.h"

namespace Ttyh {
namespace Json {
class ArgumentInfo
{
public:
    explicit ArgumentInfo(const QJsonObject &jObject);
    explicit ArgumentInfo(const QString &arg);

    QStringList values;
    QList<Rule> rules;
};
}
}

#endif // ARGUMENTINFO_H
