#include "utils/network.h"
#include "newsfeed.h"

Ttyh::News::NewsFeed::NewsFeed(QString newsUrl, QSharedPointer<QNetworkAccessManager> nam,
                                 const QSharedPointer<Logs::Logger> &logger)
    : QObject(nullptr), feedUrl(std::move(newsUrl)), nam(std::move(nam)), log(logger, "NewsFeed")
{
}

void Ttyh::News::NewsFeed::requestNews()
{
    log.info("Requesting news...");
    auto reply = nam->get(QNetworkRequest(feedUrl));
    Utils::Network::createTimeoutTimer(reply);

    connect(reply, &QNetworkReply::finished, [=]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            log.error("Failed to receive news: " + reply->errorString());
            return;
        }

        log.info("News are successfully received!");
        emit newsReceived(QString::fromUtf8(reply->readAll()));
    });
}
