#include <QtCore/QJsonArray>
#include "versionindex.h"
#include "assetdownloadinfo.h"

Ttyh::Json::VersionIndex::VersionIndex(const QJsonObject &jObject)
    : id(jObject["id"].toString()),
      releaseTime(QDateTime::fromString(jObject["releaseTime"].toString(), Qt::ISODate)),
      mainClass(jObject["mainClass"].toString())
{
    const QString assetIndexKey = "assetIndex";
    if (jObject.contains(assetIndexKey)) {
        auto assetsInfo = AssetDownloadInfo(jObject[assetIndexKey].toObject());
        assetsIndex = QString("%1/%2").arg(assetsInfo.sha1, assetsInfo.id);
    } else {
        assetsIndex = jObject["assets"].toString();
    }

    auto jLibraries = jObject["libraries"].toArray();
    foreach (auto jLibraryInfo, jLibraries) {
        libraries << LibraryInfo(jLibraryInfo.toObject());
    }

    const QString argumentsKey = "arguments";
    if (jObject.contains(argumentsKey)) {
        foreach (auto token, jObject[argumentsKey].toObject()["game"].toArray()) {
            if (token.isString())
                gameArguments << token.toString();
        }
    } else {
        gameArguments = jObject["minecraftArguments"].toString().split(' ');
    }
}

bool Ttyh::Json::VersionIndex::isValid() const
{
    return !id.isEmpty() && !assetsIndex.isEmpty() && !mainClass.isEmpty() && !libraries.isEmpty()
            && !gameArguments.isEmpty();
}
