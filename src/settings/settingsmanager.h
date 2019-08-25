#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QtCore/QString>
#include <QtCore/QSharedPointer>
#include "logs/logger.h"
#include "logs/namedlogger.h"
#include "settingsdata.h"

namespace Ttyh {
namespace Settings {
class SettingsManager
{
public:
    SettingsManager(const QString &dirName, const QSharedPointer<Logs::Logger> &logger);
    ~SettingsManager();

    SettingsData data;

    bool isFreshRun();
    void save();

private:
    const QString cfgFilePath;
    const bool freshRun;
    Logs::NamedLogger log;

    void xorBytes(QByteArray &bytes);
};
}
}

#endif // SETTINGSMANAGER_H
