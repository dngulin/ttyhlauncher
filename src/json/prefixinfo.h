#ifndef PREFIXINFO_H
#define PREFIXINFO_H


#include <QtCore/QJsonObject>

namespace Ttyh
{
namespace Json
{
class PrefixInfo
{
public:
    PrefixInfo();
    explicit PrefixInfo(const QJsonObject &jObject);

    QString type;
    QString about;

    QJsonObject toJsonObject() const;

protected:
    static constexpr const char* keyType = "type";
    static constexpr const char* keyAbout = "about";
};
}
}


#endif //PREFIXINFO_H
