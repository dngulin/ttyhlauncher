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

    void save();

private:
    Logs::NamedLogger log;
    QString cfgFilePath;
};
}
}

#endif // SETTINGSMANAGER_H
