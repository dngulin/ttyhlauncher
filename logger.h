#ifndef LOGGER_H
#define LOGGER_H

#include <QtCore>

class Logger : public QObject
{
    Q_OBJECT
private:
    explicit Logger(QObject *parent = 0);

    static Logger *myInstance;

    QFile logFile;
    QIODevice::OpenMode mode = QIODevice::Text | QIODevice::Append
                               | QIODevice::WriteOnly;

    Logger &operator=(Logger const &);
    Logger(Logger const &);

public:
    static Logger *logger();
    void appendLine(const QString &sender, const QString &text);

signals:
    void lineAppended(const QString &text);
};

#endif // LOGGER_H
