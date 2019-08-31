#include <functional>
#include <QFile>
#include <QDir>
#include <unzip.h>

#include "zip.h"
#include "defer.h"

namespace Ttyh {
namespace Utils {
namespace Zip {

voidpf zOpenFile(voidpf, const void *filename, int mode)
{
    auto file = new QFile(*static_cast<const QString *>(filename));

    QIODevice::OpenMode qMode = QIODevice::NotOpen;
    if (mode & ZLIB_FILEFUNC_MODE_READ) {
        qMode |= QIODevice::ReadOnly;
    }
    if (mode & (ZLIB_FILEFUNC_MODE_WRITE | ZLIB_FILEFUNC_MODE_CREATE)) {
        qMode |= QIODevice::WriteOnly;
    }

    if (file->open(qMode)) {
        return file;
    } else {
        delete file;
        return nullptr;
    }
}

uLong zReadFile(voidpf, voidpf stream, void *buf, uLong size)
{
    auto file = static_cast<QFile *>(stream);
    auto ret = uLong(file->read(static_cast<char *>(buf), qint64(size)));
    return ret;
}

uLong zWriteFile(voidpf, voidpf stream, const void *buf, uLong size)
{
    auto file = static_cast<QFile *>(stream);
    return uLong(file->write(static_cast<const char *>(buf), qint64(size)));
}

ZPOS64_T zTellFile(voidpf, voidpf stream)
{
    auto file = static_cast<QFile *>(stream);
    return ZPOS64_T(file->pos());
}

long zSeekFile(voidpf, voidpf stream, ZPOS64_T offset, int origin)
{
    auto file = static_cast<QFile *>(stream);

    qint64 dest;
    switch (origin) {
    case ZLIB_FILEFUNC_SEEK_SET:
        dest = qint64(offset);
        break;
    case ZLIB_FILEFUNC_SEEK_CUR:
        dest = file->pos() + qint64(offset);
        break;
    case ZLIB_FILEFUNC_SEEK_END:
        dest = file->size() + qint64(offset);
        break;
    default:
        return -1;
    }
    if (file->seek(dest)) {
        return 0;
    } else {
        return -1;
    }
}

int zCloseFile(voidpf, voidpf stream)
{
    auto file = static_cast<QFile *>(stream);

    bool success = file->flush();
    file->close();

    delete file;

    if (success) {
        return 0;
    } else {
        return EOF;
    }
}

int zTestFile(voidpf, voidpf stream)
{
    auto file = static_cast<QFile *>(stream);
    auto err = file->error();
    if (err == QFileDevice::NoError) {
        return 0;
    } else {
        return -1;
    }
}

bool zExtractCurrentFile(unzFile desc, const QString &path)
{
    QFile dest(path);
    if (!dest.open(QIODevice::WriteOnly)) {
        return false;
    }
    Defer closeDest([&dest]() { dest.close(); });

    if (unzOpenCurrentFile(desc) != UNZ_OK) {
        return false;
    }

    Defer closeSrc([desc]() { unzCloseCurrentFile(desc); });

    QByteArray buf(4000, '\0');
    for (;;) {
        auto len = unzReadCurrentFile(desc, buf.data(), unsigned(buf.size()));
        if (len == 0) {
            return dest.flush();
        }
        if (len < 0) {
            return false;
        }
        auto actual = dest.write(buf.constData(), len);
        // Docs are unclear here, but anyone seems to assume that this can not happen normally.
        if (actual != len) {
            return false;
        }
    }
}

bool unzipDir(QString zipPath, const QString &destDir)
{
    zlib_filefunc64_def funcDef = {
        funcDef.zopen64_file = zOpenFile, funcDef.zread_file = zReadFile,
        funcDef.zwrite_file = zWriteFile, funcDef.ztell64_file = zTellFile,
        funcDef.zseek64_file = zSeekFile, funcDef.zclose_file = zCloseFile,
        funcDef.zerror_file = zTestFile,  funcDef.opaque = nullptr
    };

    auto desc = unzOpen2_64(&zipPath, &funcDef);
    if (!desc) {
        return false;
    }

    Defer closeZip([desc]() { unzClose(desc); });

    if (unzGoToFirstFile(desc) != UNZ_OK) {
        return false;
    }

    QDir dir(destDir);

    QByteArray pathBuf(4000, '\0');
    for (;;) {
        auto data = pathBuf.data();
        auto size = uLong(pathBuf.size());
        if (unzGetCurrentFileInfo64(desc, nullptr, data, size, nullptr, 0, nullptr, 0) != UNZ_OK) {
            return false;
        }

        auto path = QString(pathBuf);

        if (path.endsWith("/")) {
            if (!dir.mkpath(dir.filePath(path))) {
                return false;
            }
        } else {
            auto idx = path.lastIndexOf("/");
            if (idx > 0) {
                auto parent = dir.filePath(path.left(idx));
                if (!dir.mkpath(parent)) {
                    return false;
                }
            }

            if (!zExtractCurrentFile(desc, dir.filePath(path))) {
                return false;
            }
        }

        switch (unzGoToNextFile(desc)) {
        case UNZ_OK:
            break;
        case UNZ_END_OF_LIST_OF_FILE:
            return true;
        default:
            return false;
        }
    }
}

}
}
}
