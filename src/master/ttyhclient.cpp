#include <QApplication>
#include <QtCore/QJsonDocument>
#include "utils/network.h"
#include "utils/platform.h"
#include "ttyhclient.h"

Ttyh::Master::TtyhClient::TtyhClient(const QString &masterUrl, QString ticket,
                                     QSharedPointer<QNetworkAccessManager> nam,
                                     const QSharedPointer<Logs::Logger> &logger)
    : urlPattern([&masterUrl]() { return QString("%1/index.php?act=%2").arg(masterUrl); }()),
      ticket(std::move(ticket)),
      nam(std::move(nam)),
      log(logger, "TtyhClient")
{
}

void Ttyh::Master::TtyhClient::login(const QString &login, const QString &pass)
{
    QJsonObject payload, agent, platform;

    agent.insert("name", "Minecraft");
    agent.insert("version", 1);

    platform.insert("os", Utils::Platform::getOsName());
    platform.insert("version", Utils::Platform::getOsVersion());
    platform.insert("word", Utils::Platform::getWordSize());

    payload.insert("agent", agent);
    payload.insert("platform", platform);
    payload.insert("username", login);
    payload.insert("password", pass);
    payload.insert("ticket", ticket);
    payload.insert("launcherVersion", QApplication::applicationVersion());

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    auto uuid = QSysInfo::machineUniqueId();
    if (!uuid.isEmpty())
        payload.insert("uuid", QString::fromUtf8(uuid));
#endif

    log.info(QString("Logging as %1...").arg(login));
    auto reply = post("login", payload);

    connect(reply, &QNetworkReply::finished, [=]() {
        reply->deleteLater();
        auto error = reply->error();

        if (error != QNetworkReply::NoError) {
            log.error("Failed to login: " + reply->errorString());

            auto result = error == QNetworkReply::AuthenticationRequiredError
                    ? RequestResult::LoginError
                    : RequestResult::ConnectionError;

            emit logged(result, QString(), QString());
            return;
        }

        LoginReplyData data(QJsonDocument::fromJson(reply->readAll()).object());
        if (!data.isValid()) {
            log.error("Bad reply!");
            emit logged(RequestResult::ReplyError, QString(), QString());
            return;
        }

        if (!data.error.isEmpty()) {
            log.error(QString("%1: '%2'").arg(data.error, data.errorMessage));
            emit logged(RequestResult::RequestError, QString(), QString());
            return;
        }

        log.info(QString("Success! at: '%1', ct: '%2'").arg(data.accessToken, data.clientToken));
        emit logged(RequestResult::Success, data.accessToken, data.clientToken);
    });
}

void Ttyh::Master::TtyhClient::uploadSkin(const QString &login, const QString &pass,
                                          const QByteArray &data, bool slim)
{
    QJsonObject payload;
    payload.insert("username", login);
    payload.insert("password", pass);
    payload.insert("skinData", QString::fromUtf8(data.toBase64()));

    if (slim) {
        payload.insert("skinModel", "slim");
    }

    log.info("Uploading a skin...");
    auto reply = post("setskin", payload);

    connect(reply, &QNetworkReply::finished, [=]() {
        reply->deleteLater();
        auto error = reply->error();

        if (error != QNetworkReply::NoError) {
            log.error("Failed to upload skin: " + reply->errorString());

            auto result = error == QNetworkReply::AuthenticationRequiredError
                    ? RequestResult::LoginError
                    : RequestResult::ConnectionError;

            emit skinUploaded(result);
            return;
        }

        auto doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            log.error("Bad reply!");
            emit skinUploaded(RequestResult::ReplyError);
            return;
        }

        auto data = ReplyData(doc.object());
        if (!data.error.isEmpty()) {
            log.error(QString("%1: '%2'").arg(data.error, data.errorMessage));
            emit skinUploaded(RequestResult::RequestError);
            return;
        }

        log.info("Success!");
        emit skinUploaded(RequestResult::Success);
    });
}

QNetworkReply *Ttyh::Master::TtyhClient::post(const QString &action, const QJsonObject &payload)
{
    auto data = QJsonDocument(payload).toJson();

    QNetworkRequest request(QUrl(urlPattern.arg(action)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, data.size());

    log.info(QString("Sending a POST request '%1'...").arg(request.url().toString()));
    auto reply = nam->post(request, data);
    Utils::Network::createTimeoutTimer(reply);

    return reply;
}
