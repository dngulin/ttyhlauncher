#ifndef FULLVERSIONID_H
#define FULLVERSIONID_H

#include <QtCore/QString>
#include <QtCore/QJsonObject>

namespace Ttyh {
namespace Versions {
class FullVersionId
{
public:
    explicit FullVersionId(const QJsonObject &jObject = QJsonObject());
    FullVersionId(QString prefixId, QString versionId);

    QString prefix;
    QString id;

    QJsonObject toJsonObject() const;
    QString toString() const;

private:
    static constexpr const char *keyPrefix = "prefix";
    static constexpr const char *keyVersion = "version";
};
}
}

#endif // FULLVERSIONID_H
