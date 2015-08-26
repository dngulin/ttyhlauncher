#ifndef LOGGER_H
#define LOGGER_H

#include <QtCore>

class Logger : public QObject
{
    Q_OBJECT
private:
    explicit Logger(QObject *parent = 0);

    static Logger* myInstance;

    QFile* logFile;

    Logger& operator=(Logger const&);
    Logger(Logger const&);

public:
    static Logger* logger();
    void append(QString sender, QString text);

signals:
    void textAppended(QString text);

};

#endif // LOGGER_H
