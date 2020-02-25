#ifndef PROFILESMANAGER_H
#define PROFILESMANAGER_H

#include "logs/logger.h"
#include "logs/namedlogger.h"
#include "profiledata.h"
#include "profilesopresults.h"

namespace Ttyh {
namespace Profiles {
class ProfilesManager
{
public:
    ProfilesManager(QString workDir, const QSharedPointer<Logs::Logger> &logger);

    bool isEmpty() const;
    QStringList names() const;

    bool contains(const QString &profileName) const;
    ProfileData get(const QString &profileName) const;

    CreateResult create(const QString &profileName, const ProfileData &data);
    RenameResult rename(const QString &oldName, const QString &newName);

    bool update(const QString &profileName, const ProfileData &data);
    bool remove(const QString &profileName);

    bool installFiles(const QString &profileName, const Versions::FullVersionId &version);

private:
    const QString dataPath;
    Logs::NamedLogger log;
    QHash<QString, ProfileData> profiles;

    static bool validateProfileName(const QString &profileName);
    static bool checkFile(const QString &path, int size, const QString &hash, bool editable);

    const char *profileIndexName = "profile.json";
    const char *filesIndexName = "files.json";
};
}
}

#endif // PROFILESMANAGER_H
