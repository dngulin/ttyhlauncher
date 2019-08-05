#ifndef CHECKINFO_H
#define CHECKINFO_H

#include <QtCore/QJsonObject>

namespace Ttyh {
namespace Json {
class CheckInfo
{
public:
    CheckInfo();
    explicit CheckInfo(const QJsonObject &jObject);

    QString hash;
    int size;

    bool isValid() const;
};
}
}

#endif // CHECKINFO_H
