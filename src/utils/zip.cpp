#include <QFile>
#include <QDir>
#include <zip.h>

#include "zip.h"
#include "defer.h"

namespace Ttyh {
namespace Utils {
namespace Zip {

static bool unzipFile(zip *archive, int entryIndex, const QString &filePath, quint64 fileSize)
{
    auto zipFile = zip_fopen_index(archive, entryIndex, ZIP_FL_UNCHANGED);
    if (zipFile == nullptr)
        return false;
    Defer closeZipFile([zipFile]() { zip_fclose(zipFile); });

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QByteArray buff(4096, '\0');

    quint64 writtenTotal = 0;
    while (writtenTotal < fileSize) {
        auto readLength = zip_fread(zipFile, buff.data(), buff.size());
        if (readLength < 0)
            return false;

        auto writtenLength = file.write(buff.constData(), readLength);
        if (writtenLength != readLength)
            return false;

        writtenTotal += writtenLength;
    }

    return true;
}

bool unzipDir(const QString &zipPath, const QString &destDir)
{
    QDir dir(destDir);
    dir.mkpath(destDir);

    struct zip *archive;
    int errorCode;

    auto cStrZipPath = QDir::toNativeSeparators(zipPath).toUtf8().constData();
    auto openFlags = ZIP_CHECKCONS | ZIP_RDONLY;

    if ((archive = zip_open(cStrZipPath, openFlags, &errorCode)) == nullptr)
        return false;
    Defer closeZip([archive]() { zip_close(archive); });

    int entryCount = zip_get_num_entries(archive, ZIP_FL_UNCHANGED);
    for (auto entryIndex = 0; entryIndex < entryCount; entryIndex++) {
        struct zip_stat entryStat {
        };

        auto statFlags = ZIP_FL_UNCHANGED | ZIP_FL_ENC_GUESS;
        if (zip_stat_index(archive, entryIndex, statFlags, &entryStat) == 0) {
            auto statDataMask = (ZIP_STAT_NAME | ZIP_STAT_SIZE);
            if ((entryStat.valid & statDataMask) != statDataMask)
                return false;

            QString entryName(entryStat.name);

            if (entryName.endsWith("/")) {
                if (!dir.mkpath(dir.filePath(entryName)))
                    return false;
            } else {
                auto filePath = dir.filePath(entryName);
                auto fileSize = entryStat.size;
                if (!unzipFile(archive, entryIndex, filePath, fileSize))
                    return false;
            }

        } else {
            return false;
        }
    }

    return true;
}

}
}
}
