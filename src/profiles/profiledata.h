#ifndef PROFILEDATA_H
#define PROFILEDATA_H

#include <QtCore/QJsonObject>
#include <QtCore/QSize>

#include "versions/fullversionid.h"

namespace Ttyh {
namespace Profiles {
enum class WindowSizeMode { FullScreen, LauncherLike, Specified };

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

    bool setWindowSizeOnRun;
    WindowSizeMode windowSizeMode;
    QSize windowSize;

private:
    static WindowSizeMode stringToSizeMode(const QString &stringMode);
    static QString sizeModeToString(WindowSizeMode mode);

    static constexpr const char *keyVersion = "version";

    static constexpr const char *keyUseCustomJavaPath = "use_custom_java_path";
    static constexpr const char *keyCustomJavaPath = "custom_java_path";

    static constexpr const char *keyUseCustomJavaArgs = "use_custom_java_args";
    static constexpr const char *keyCustomJavaArgs = "custom_java_args";

    static constexpr const char *keySetWindowSize = "set_window_size";
    static constexpr const char *keyWindowSizeMode = "window_size_mode";
    static constexpr const char *keyWindowWidth = "window_width";
    static constexpr const char *keyWindowHeight = "window_height";

    static constexpr const char *modeFullScreen = "full-screen";
    static constexpr const char *modeLauncherLike = "launcher-like";
    static constexpr const char *modeSpecifiedSize = "specified-size";
};
}
}

#endif // PROFILEDATA_H
