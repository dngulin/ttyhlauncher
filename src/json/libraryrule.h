#ifndef LIBRARYRULE_H
#define LIBRARYRULE_H

#include <QtCore/QJsonObject>

namespace Ttyh {
namespace Json {
class Rule
{
public:
    explicit Rule(const QJsonObject &jObject);

    QString osName;
    QString osVersion;
    QString osArch;
    QString action;
};
}
}

#endif // LIBRARYRULE_H
