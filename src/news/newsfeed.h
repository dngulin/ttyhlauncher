#ifndef NEWSFEED_H
#define NEWSFEED_H

#include <QtCore/QObject>
#include <QtNetwork/QNetworkAccessManager>

#include "logs/logger.h"
#include "logs/namedlogger.h"

namespace Ttyh {
namespace News {
class NewsFeed : public QObject
{
    Q_OBJECT
public:
    NewsFeed(QString newsUrl, QSharedPointer<QNetworkAccessManager> nam,
             const QSharedPointer<Logs::Logger> &logger);

    void requestNews();

signals:
    void newsReceived(const QString &contents);

private:
    const QString feedUrl;
    QSharedPointer<QNetworkAccessManager> nam;
    Logs::NamedLogger log;
};
}
}

#endif // NEWSFEED_H
