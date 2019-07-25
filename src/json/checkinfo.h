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
    CheckInfo();
    explicit CheckInfo(const QJsonObject &jObject);

    QString hash;
    int size;

private:
    static const char* keyHash;
    static const char* keySize;
};
}
}


#endif //CHECKINFO_H
