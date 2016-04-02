#ifndef HASHCHECKER_H
#define HASHCHECKER_H

#include <QtCore>

class HashChecker : public QObject
{
    Q_OBJECT

public slots:
    void checkFiles(const QList< QPair< QString, QString > > &list);
    void cancel();

private:
    bool checkFile(const QString &name, const QString &hash);

    bool cancelled;

signals:
    void progress( int percents );
    void verificationFailed(const QString &name);
    void finished();
};

#endif // HASHCHECKER_H
