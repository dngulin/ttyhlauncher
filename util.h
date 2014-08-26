#ifndef UTIL_H
#define UTIL_H

#include <QtCore>
#include "serverreply.h"

namespace Util {

ServerReply makeGet(QString url);
ServerReply makePost(QString url, QByteArray postData);

void removeAll(QString filePath);

void unzipArchive(QString zipFilePath, QString extractionPath);

}

#endif // UTIL_H
