#include "reply.h"

Reply::Reply(bool state, const QString &errStr, const QByteArray &data)
{
    status = state;
    errorString = errStr;
    replyData = data;
}

bool Reply::isSuccess() {
    return status;
}

QString Reply::getErrorString() {
    return errorString;
}

QByteArray Reply::getData() {
    return replyData;
}
