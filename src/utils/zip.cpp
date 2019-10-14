#include <QFile>
#include <QDir>
#include <zip.h>

#include "zip.h"
#include "defer.h"

namespace Ttyh {
namespace Utils {
namespace Zip {

static bool unzipFile(zip *zip, int idx, const QString &path, quint64 size, const LogFunc &logError)
{
    auto zipFile = zip_fopen_index(zip, idx, ZIP_FL_UNCHANGED);
    if (zipFile == nullptr) {
        logError("Uninitialized zip pointer!");
        return false;
    }
    Defer closeZipFile([zipFile]() { zip_fclose(zipFile); });

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        logError(QString("Failed to open file '%1'").arg(path));
        return false;
    }

    QByteArray buff(4096, '\0');

    quint64 writtenTotal = 0;
    while (writtenTotal < size) {
        auto readLength = zip_fread(zipFile, buff.data(), buff.size());
        if (readLength < 0) {
            logError(QString("Failed to read bytes form file '%1'").arg(path));
            return false;
        }

        auto writtenLength = file.write(buff.constData(), readLength);
        if (writtenLength != readLength) {
            auto msg = QString("IO Error: READ (%1) != WRITTEN (%2); File: '%3'")
                               .arg(readLength)
                               .arg(writtenLength)
                               .arg(path);
            logError(msg);
            return false;
        }

        writtenTotal += writtenLength;
    }

    return true;
}

bool unzipDir(const QString &zipPath, const QString &destDir, const LogFunc &logError)
{
    QDir dir(destDir);
    dir.mkpath(destDir);

    struct zip *archive;
    int errorCode;

    auto byteArray = QDir::toNativeSeparators(zipPath).toUtf8();
    auto cStrZipPath = byteArray.constData();
    auto openFlags = ZIP_CHECKCONS | ZIP_RDONLY;

    if ((archive = zip_open(cStrZipPath, openFlags, &errorCode)) == nullptr) {
        logError(QString("Failed to open the archive! LibZip error code: %1").arg(errorCode));
        return false;
    }
    Defer closeZip([archive]() { zip_close(archive); });

    int entryCount = zip_get_num_entries(archive, ZIP_FL_UNCHANGED);
    for (auto entryIndex = 0; entryIndex < entryCount; entryIndex++) {
        struct zip_stat entryStat {
        };

        auto statFlags = ZIP_FL_UNCHANGED | ZIP_FL_ENC_GUESS;
        if (zip_stat_index(archive, entryIndex, statFlags, &entryStat) == 0) {
            auto statDataMask = (ZIP_STAT_NAME | ZIP_STAT_SIZE);
            if ((entryStat.valid & statDataMask) != statDataMask)
            {
                logError(QString("Failed to stat a zip-entry at index %1").arg(entryIndex));
                return false;
            }

            QString entryName(entryStat.name);

            if (entryName.endsWith("/")) {
                if (!dir.mkpath(dir.filePath(entryName))) {
                    logError(QString("Failed to create a directory '%1'").arg(entryName));
                    return false;
                }
            } else {
                auto filePath = dir.filePath(entryName);
                auto fileSize = entryStat.size;
                if (!unzipFile(archive, entryIndex, filePath, fileSize, logError))
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
