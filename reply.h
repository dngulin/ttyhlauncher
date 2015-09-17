#ifndef SERVERREPLY_H
#define SERVERREPLY_H

#include <QtCore>

class Reply
{
public:
    Reply(bool state, const QString & errStr, const QByteArray & data);

    bool isSuccess();
    QString getErrorString();
    QByteArray getData();

private:
    bool status;
    QString errorString;
    QByteArray replyData;
};

#endif // SERVERREPLY_H
