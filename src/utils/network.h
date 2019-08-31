#ifndef NETWORK_H
#define NETWORK_H

#include <QtNetwork/QNetworkReply>

namespace Ttyh {
namespace Utils {
namespace Network {
void createTimeoutTimer(QNetworkReply *reply);
}
}
}

#endif // NETWORK_H
