#ifndef PROFILERUNNER_H
#define PROFILERUNNER_H

#include <QtCore/QObject>
#include <QtCore/QProcess>

#include "logs/namedlogger.h"
#include "logs/logger.h"
#include "profiledata.h"

namespace Ttyh {
namespace Profiles {
using namespace Ttyh::Logs;

class ProfileRunner : public QObject
{
    Q_OBJECT
public:
    ProfileRunner(const QString &dirName, const QSharedPointer<Logger> &logger);

    bool run(const QString &name, const ProfileData &data, const QString &userName);
    bool run(const QString &name, const ProfileData &data, const QString &userName,
             const QString &clientToken, const QString &accessToken);

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
