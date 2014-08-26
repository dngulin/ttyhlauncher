#include "serverreply.h"

ServerReply::ServerReply(bool state, QString errStr, QByteArray data)
{
    status = state;
    errorString = errStr;
    replyData = data;
}

bool ServerReply::isOK() {
    return status;
}

QString ServerReply::getErrorString() {
    return errorString;
}

QByteArray ServerReply::reply() {
    return replyData;
}
