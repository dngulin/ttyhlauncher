#include <QtNetwork/QNetworkReply>
#include <QtCore/QTimer>

#include "network.h"

namespace Ttyh {
namespace Utils {
namespace Network {
void createTimeoutTimer(QNetworkReply *reply) {
    auto timer = new QTimer(reply);
    timer->setSingleShot(true);
    timer->start(3000);

    QObject::connect(timer, &QTimer::timeout, [=](){ reply->abort(); });
    QObject::connect(reply, &QNetworkReply::readyRead, [=](){ timer->stop(); });
}
}
}
}
