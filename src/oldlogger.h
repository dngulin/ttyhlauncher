#ifndef OLDLOGGER_H
#define OLDLOGGER_H

#include <QtCore>

class OldLogger : public QObject
{
    Q_OBJECT
private:
    explicit OldLogger(QObject *parent = 0);

    static OldLogger *myInstance;

    QFile logFile;
    QIODevice::OpenMode mode;

    OldLogger &operator=(OldLogger const &);
    OldLogger(OldLogger const &);

public:
    static OldLogger *logger();
    void appendLine(const QString &sender, const QString &text);

signals:
    void lineAppended(const QString &text);
};

#endif //OLDLOGGER_H
