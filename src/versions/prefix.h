#ifndef PREFIX_H
#define PREFIX_H

#include <QtCore/QString>
#include <QtCore/QStringList>

namespace Ttyh {
namespace Versions {
struct Prefix {
    static constexpr const char *latestVersionAlias = "latest";

    Prefix() = default;
    explicit Prefix(QString id, QString name) : id(std::move(id)), name(std::move(name))
    {
        versions << latestVersionAlias;
    }

    QString id;
    QString name;
    QStringList versions;
    QString latestVersionId;
};
}
}

#endif // PREFIX_H
