#ifndef PROFILEDATA_H
#define PROFILEDATA_H

#include <QtCore/QJsonObject>

#include "versions/fullversionid.h"

namespace Ttyh {
namespace Profiles {
class ProfileData
{
public:
    explicit ProfileData(const QJsonObject &jObject = QJsonObject());
    QJsonObject toJsonObject() const;

    Versions::FullVersionId version;

    bool useCustomJavaPath;
    QString customJavaPath;

    bool useCustomJavaArgs;
    QString customJavaArgs;


private:
    static constexpr const char *keyVersion = "version";

    static constexpr const char *keyUseCustomJavaPath = "use_custom_java_path";
    static constexpr const char *keyCustomJavaPath = "custom_java_path";

    static constexpr const char *keyUseCustomJavaArgs = "use_custom_java_args";
    static constexpr const char *keyCustomJavaArgs = "custom_java_args";
};
}
}

#endif // PROFILEDATA_H
