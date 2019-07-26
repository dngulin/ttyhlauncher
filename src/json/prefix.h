#ifndef PREFIX_H
#define PREFIX_H


#include <QtCore/QJsonObject>

namespace Ttyh
{
namespace Json
{
class Prefix
{
public:
    Prefix();
    explicit Prefix(const QJsonObject &jObject);

    QString type;
    QString about;

    QJsonObject toJsonObject() const;

protected:
    static constexpr const char* keyType = "type";
    static constexpr const char* keyAbout = "about";
};
}
}


#endif //PREFIX_H
