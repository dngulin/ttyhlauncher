#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

#include "libraryinfo.h"
#include "fileinfo.h"

class JsonParser : public QObject
{
    Q_OBJECT
public:
    explicit JsonParser(QObject *parent = 0);

    // Generic methods
    QString getParserError() const;
    bool setJson(const QByteArray &json);
    bool setJsonFromFile(const QString &fileName);

    bool hasStringKey(const QString &key) const;
    QString getStringKey(const QString &key) const;

    // Parse master-server responces
    bool hasServerResponseError() const;
    QString getServerResponseError() const;

    bool hasClientToken() const;
    QString getClientToken() const;

    bool hasAccessToken() const;
    QString getAccessToken() const;

    // Parse versions json
    bool hasVersionList() const;
    QStringList getReleaseVersonList() const;

    bool hasLatestReleaseVersion() const;
    QString getLatestReleaseVersion() const;

    // Parse prefixes json
    bool hasPrefixesList() const;
    QHash<QString, QString> getPrefixesList() const;

    // Parse version.json
    bool hasReleaseTime() const;
    QDateTime getReleaseTime() const;

    bool hasReleaseType() const;
    QString getReleaseType() const;

    bool hasMainClass() const;
    QString getMainClass() const;

    bool hasAssetsVersion() const;
    QString getAssetsVesrsion() const;

    bool hasMinecraftArgs() const;
    QString getMinecraftArgs() const;

    bool hasLibraryList() const;
    QList<LibraryInfo> getLibraryList() const;

    // Parse data.json
    bool hasJarFileInfo() const;
    FileInfo getJarFileInfo() const;

    bool hasLibsFileInfo() const;
    QList<FileInfo> getLibsFileInfo() const;

    bool hasLibFileInfo(const QString &lib) const;
    FileInfo getLibFileInfo(const QString &lib) const;

    bool hasAddonsFilesInfo() const;
    QList<FileInfo> getAddonsFilesInfo() const;

    QHash<QString, FileInfo> getAddonsFilesInfoHashMap() const;

    // Parse assets json
    bool hasAssetsList() const;
    QList<FileInfo> getAssetsList() const;

private:
    QString errorString;
    QJsonObject jsonObject;
};

#endif // JSONPARSER_H
