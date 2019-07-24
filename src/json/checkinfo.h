#ifndef CHECKINFO_H
#define CHECKINFO_H


#include <QtCore/QJsonObject>

namespace Ttyh
{
namespace Json
{
class CheckInfo
{
public:
    explicit CheckInfo(const QJsonObject &jObject);
    QJsonObject toJObject() const;

    QString hash;
    int size;

private:
    const QString keyHash = "hash";
    const QString keySize = "size";

};
}
}


#endif //CHECKINFO_H
