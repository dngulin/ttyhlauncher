#ifndef PROFILERUNNER_H
#define PROFILERUNNER_H

#include <QtCore/QObject>
#include <QtCore/QProcess>

#include "logs/namedlogger.h"
#include "logs/logger.h"
#include "profiledata.h"
#include "profileinfo.h"

namespace Ttyh {
namespace Profiles {
using namespace Ttyh::Logs;

class ProfileRunner : public QObject
{
    Q_OBJECT
public:
    ProfileRunner(const QString &dirName, const QSharedPointer<Logger> &logger);

    bool run(const ProfileInfo &data, const QString &userName, const QSize &launcherSize);
    bool run(const ProfileInfo &info, const QString &userName, const QString &accessToken,
             const QString &clientToken, const QSize &launcherSize);

signals:
    void finished(bool result);

private:
    const QString dataPath;
    NamedLogger log;
    QProcess game;
};
}
}

#endif // PROFILERUNNER_H
