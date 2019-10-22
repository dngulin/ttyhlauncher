#ifndef PREFIX_H
#define PREFIX_H

#include <QtCore/QString>
#include <QtCore/QStringList>

namespace Ttyh {
namespace Versions {
struct Prefix {
    static constexpr const char *latestVersionAlias = "latest";

    Prefix() : id(""), name("__UNKNOWN_PREFIX"), latestVersionId(latestVersionAlias)
    {
        versions << latestVersionAlias;
    }

    Prefix(QString id, QString name)
        : id(std::move(id)), name(std::move(name)), latestVersionId(latestVersionAlias)
    {
        versions << latestVersionAlias;
    }

    QString id;
    QString name;
    QStringList versions;
    QString latestVersionId;

    static bool less(const Prefix &a, const Prefix &b) {
        return a.id < b.id;
    }
};
}
}

#endif // PREFIX_H
