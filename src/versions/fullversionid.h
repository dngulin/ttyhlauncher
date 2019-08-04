#ifndef FULLVERSIONID_H
#define FULLVERSIONID_H

#include <QtCore/QString>
#include <QtCore/QJsonObject>

namespace Ttyh {
namespace Versions {
class FullVersionId
{
public:
    explicit FullVersionId(const QJsonObject &jObject);
    FullVersionId(QString prefixId, QString versionId);

    QString prefixId;
    QString versionId;

    QJsonObject toJsonObject() const;

private:
    static constexpr const char *keyPrefix = "prefix";
    static constexpr const char *keyVersion = "version";
};
}
}

#endif // FULLVERSIONID_H
