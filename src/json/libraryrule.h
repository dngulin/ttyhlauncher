#ifndef LIBRARYRULE_H
#define LIBRARYRULE_H


#include <QtCore/QJsonObject>

namespace Ttyh
{
namespace Json
{
class LibraryRule
{
public:
    explicit LibraryRule(const QJsonObject &jObject);

    QString os;
    QString action;
};
}
}

#endif //LIBRARYRULE_H
