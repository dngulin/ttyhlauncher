#ifndef DOWNLOADFILEREPLY_H
#define DOWNLOADFILEREPLY_H

#include <QtCore/QObject>
#include <QtCore/QFile>
#include <QtNetwork/QNetworkReply>

namespace Ttyh {
namespace Utils {
class DownloadFileReply : public QObject
{
    Q_OBJECT
public:
    DownloadFileReply(QNetworkReply *reply, const QString &filePath);
    QString errorString() const;

    void cancel();

signals:
    void bytesWritten(int count);
    void finished(bool cancelled, bool success);

private:
    QNetworkReply *parentReply;
    QFile file;
    bool hasWriteError;
    bool cancelled;
};
}
}

#endif // DOWNLOADFILEREPLY_H
