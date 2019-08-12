#ifndef TTYHCLIENT_H
#define TTYHCLIENT_H

#include <QtCore/QObject>
#include <QtNetwork/QNetworkAccessManager>

#include "logs/logger.h"
#include "logs/namedlogger.h"
#include "loginreplydata.h"

namespace Ttyh {
namespace Master {
enum class RequestResult { Success, ConnectionError, LoginError, RequestError, ReplyError };

class TtyhClient : public QObject
{
    Q_OBJECT
public:
    TtyhClient(const QString &masterUrl, QString ticket, QSharedPointer<QNetworkAccessManager> nam,
               const QSharedPointer<Logs::Logger> &logger);

    void login(const QString &login, const QString &pass);
    void uploadSkin(const QString &login, const QString &pass, const QByteArray &data, bool slim);

signals:
    void loginFinished(RequestResult result, const QString &accessToken,
                       const QString &clientToken);
    void skinUploadFinished(RequestResult result);

private:
    const QString urlPattern;
    const QString ticket;

    QSharedPointer<QNetworkAccessManager> nam;
    Logs::NamedLogger log;

    QNetworkReply *post(const QString &action, const QJsonObject &payload);
};
}
}

#endif // TTYHCLIENT_H
