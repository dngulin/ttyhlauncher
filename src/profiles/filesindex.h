#ifndef FILESINDEX_H
#define FILESINDEX_H

#include <QtCore/QStringList>
#include <QtCore/QJsonObject>

namespace Ttyh {
namespace Profiles {
class FilesIndex
{
public:
    explicit FilesIndex(const QJsonObject &jObject);

    QStringList installed;

    QJsonObject toJsonObject() const;

private:
    static constexpr const char *keyInstalled = "installed";
};
}
}


#endif // FILESINDEX_H
