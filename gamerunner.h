#ifndef GAMERUNNER_H
#define GAMERUNNER_H

#include <QThread>
#include <QNetworkAccessManager>

#include "settings.h"
#include "logger.h"


class GameRunner : public QThread
{
    Q_OBJECT
public:
    GameRunner(const QString &playerLogin,  const QString &playerPassword,
               const QString &gamePrefix, bool onlineMode,
               QObject *parent = 0);

    void run();

signals:
    void error(const QString &message);
    void needUpdate(const QString &message);
    void finished(int exitCode);

private:
    QString name;
    QString password;
    QString prefix;
    bool isOnline;

    Settings* settings;
    Logger* logger;
    QNetworkAccessManager* nam;

    void emitError(const QString &message);
    void emitNeedUpdate(const QString &message);
};

#endif // GAMERUNNER_H
