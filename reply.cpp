#include "reply.h"

Reply::Reply(bool state, QString errStr, QByteArray data)
{
    status = state;
    errorString = errStr;
    replyData = data;
}

bool Reply::isOK() {
    return status;
}

QString Reply::getErrorString() {
    return errorString;
}

QByteArray Reply::reply() {
    return replyData;
}
