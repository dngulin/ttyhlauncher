#ifndef MIGRATIONS_H
#define MIGRATIONS_H

#include <QtCore/QSharedPointer>

#include "settings/settingsmanager.h"
#include "logs/logger.h"

namespace Ttyh {
namespace Utils {
namespace Migrations {
void restoreLoginSettings(const QSharedPointer<Settings::SettingsManager> &settings,
                          const QSharedPointer<Logs::Logger> &logger);
}
}
}

#endif // MIGRATIONS_H
