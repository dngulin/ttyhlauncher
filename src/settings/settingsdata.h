#ifndef SETTINGSDATA_H
#define SETTINGSDATA_H

#include <QtCore/QJsonObject>
#include <QtCore/QSize>

namespace Ttyh {
namespace Settings {
class SettingsData
{
public:
    explicit SettingsData(const QJsonObject &jObject = QJsonObject());
    QJsonObject toJsonObject() const;

    QString username;
    QString password;
    bool savePassword;

    QString profile;
    QString ticket;

    QSize windowSize;
    bool windowMaximized;
    bool hideWindowOnRun;

private:
    static constexpr const char *keyUsername = "username";
    static constexpr const char *keyPassword = "password";
    static constexpr const char *keySavePassword = "save_password";

    static constexpr const char *keyProfile = "profile";
    static constexpr const char *keyTicket = "ticket";

    static constexpr const char *keyWindowSizeW = "window_width";
    static constexpr const char *keyWindowSizeH = "window_height";
    static constexpr const char *keyWindowMaximized = "window_maximized";
    static constexpr const char *keyHideWindowOnRun = "hide_on_run";
};
}
}

#endif // SETTINGSDATA_H
