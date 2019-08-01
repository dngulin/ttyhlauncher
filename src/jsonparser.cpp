#include "jsonparser.h"

#include <QFile>
#include <QJsonArray>

#include "oldsettings.h"

JsonParser::JsonParser(QObject *parent) : QObject(parent)
{
    errorString = "No JSON parse error";
}

QString JsonParser::getParserError() const
{
    return errorString;
}

bool JsonParser::setJson(const QByteArray &json)
{
    QJsonParseError error;
    jsonObject = QJsonDocument::fromJson(json, &error).object();

    if (error.error == QJsonParseError::NoError)
    {
        return true;
    }
    else
    {
        QString msg = error.errorString();
        int pos = error.offset;

        errorString = QString("%1 at position %2.").arg(msg).arg(pos);

        return false;
    }
}

bool JsonParser::setJsonFromFile(const QString &fileName)
{
    QFile jsonFile(fileName);

    if ( jsonFile.exists() )
    {
        if ( jsonFile.open(QIODevice::ReadOnly) )
        {
            bool setJsonResult = setJson( jsonFile.readAll() );
            jsonFile.close();
            return setJsonResult;
        }
        else
        {
            errorString = jsonFile.errorString();
            return false;
        }
    }
    else
    {
        errorString = QString("File '%1' does not exists!").arg(fileName);
        return false;
    }
}

bool JsonParser::hasStringKey(const QString &key) const
{
    return jsonObject[key].isString();
}

QString JsonParser::getStringKey(const QString &key) const
{
    return jsonObject[key].toString();
}

bool JsonParser::hasServerResponseError() const
{
    return hasStringKey("error");
}

QString JsonParser::getServerResponseError() const
{
    return jsonObject["error"].toString();
}

bool JsonParser::hasClientToken() const
{
    return hasStringKey("clientToken");
}

QString JsonParser::getClientToken() const
{
    return getStringKey("clientToken");
}

bool JsonParser::hasAccessToken() const
{
    return hasStringKey("accessToken");
}

QString JsonParser::getAccessToken() const
{
    return getStringKey("accessToken");
}

bool JsonParser::hasVersionList() const
{
    return jsonObject["versions"].isArray();
}

QStringList JsonParser::getReleaseVersonList() const
{
    QStringList result;
    QJsonArray versions = jsonObject["versions"].toArray();

    foreach (QJsonValue value, versions)
    {
        QJsonObject version = value.toObject();
        if (version["type"].toString() == "release")
        {
            result << version["id"].toString();
        }
    }

    return result;
}

bool JsonParser::hasLatestReleaseVersion() const
{
    return jsonObject["latest"].toObject()["release"].isString();
}

QString JsonParser::getLatestReleaseVersion() const
{
    return jsonObject["latest"].toObject()["release"].toString();
}

bool JsonParser::hasPrefixesList() const
{
    return jsonObject["prefixes"].isObject();
}

QHash<QString, QString> JsonParser::getPrefixesList() const
{
    QHash<QString, QString> result;
    QJsonObject prefixes = jsonObject["prefixes"].toObject();

    foreach ( QString key, prefixes.keys() )
    {
        QJsonObject prefix = prefixes[key].toObject();
        if (prefix["type"] == "public")
        {
            result[key] = prefix["about"].toString();
        }
    }

    return result;
}

bool JsonParser::hasReleaseTime() const
{
    return hasStringKey("releaseTime");
}

QDateTime JsonParser::getReleaseTime() const
{
    return QDateTime::fromString(getStringKey("releaseTime"), Qt::ISODate);
}

bool JsonParser::hasReleaseType() const
{
    return hasStringKey("type");
}

QString JsonParser::getReleaseType() const
{
    return getStringKey("type");
}

bool JsonParser::hasMainClass() const
{
    return hasStringKey("mainClass");
}

QString JsonParser::getMainClass() const
{
    return getStringKey("mainClass");
}

bool JsonParser::hasAssetsVersion() const
{
    return hasStringKey("assets");
}

QString JsonParser::getAssetsVesrsion() const
{
    return getStringKey("assets");
}

bool JsonParser::hasMinecraftArgs() const
{
    return hasStringKey("minecraftArguments");
}

QString JsonParser::getMinecraftArgs() const
{
    return getStringKey("minecraftArguments");
}

bool JsonParser::hasLibraryList() const
{
    return jsonObject["libraries"].isArray();
}

QList<LibraryInfo> JsonParser::getLibraryList() const
{
    QList<LibraryInfo> result;

    QJsonArray libraries = jsonObject["libraries"].toArray();
    foreach (QJsonValue libValue, libraries)
    {
        // I. RENAME LIBRARY

        // Change <package>:<name>:<version> to
        // <package>/<name>/<version>/<name>-<version>
        // Chahge <backage> format from a.b.c to a/b/c

        QJsonObject library = libValue.toObject();
        QStringList entryName = library["name"].toString().split(':');

        // Get package and change format
        QString libName = entryName.at(0);
        libName.replace('.', '/');

        // Append "/name" + "/version" + "/name-version"
        libName += "/" + entryName.at(1)
                   + "/" + entryName.at(2)
                   + "/" + entryName.at(1) + "-" + entryName.at(2);

        // II. CHECK LIBRARY RULES
        QJsonArray liraryRules = library["rules"].toArray();

        // Get OS info for allow/disallow rules and natives
        QString osName = OldSettings::instance()->getOsName();
        QString osArch = OldSettings::instance()->getWordSize();

        // Allow library if rules not defined
        bool libraryAllowed = true;
        if ( !liraryRules.isEmpty() )
        {
            // Disallow libray until allow-rule founded
            libraryAllowed = false;
            foreach (QJsonValue ruleValue, liraryRules)
            {
                QJsonObject rule = ruleValue.toObject();
                QString ruleAction = rule["action"].toString();
                QString ruleOsName = rule["os"].toObject()["name"].toString();

                // Make allow-list: all or specified
                if (ruleAction == "allow")
                {
                    if (ruleOsName.isEmpty() || ruleOsName == osName)
                    {
                        libraryAllowed = true;
                    }
                }

                // Make exclusions from allow-list
                if (ruleAction == "disallow")
                {
                    if (ruleOsName == osName)
                    {
                        libraryAllowed = false;
                    }
                }
            }
        }

        // III. CHECK NATIVE STATE
        if (libraryAllowed)
        {
            // Native laibrary
            if ( library.contains("natives") )
            {
                QJsonObject natives = library["natives"].toObject();
                QString nativesSuffix = natives[osName].toString();

                nativesSuffix.replace("${arch}", osArch);

                if ( nativesSuffix.isEmpty() )
                {
                    libName += ".jar";
                }
                else
                {
                    libName += "-" + nativesSuffix + ".jar";
                }

                result << LibraryInfo(libName, true);
            }
            // Not native laibrary
            else
            {
                libName += ".jar";
                result << LibraryInfo(libName, false);
            }
        }
    }

    return result;
}

bool JsonParser::hasJarFileInfo() const
{
    return jsonObject["main"].isObject();
}

FileInfo JsonParser::getJarFileInfo() const
{
    QJsonObject const mainObject = jsonObject["main"].toObject();
    FileInfo result;
    result.hash = mainObject["hash"].toString();
    result.size = mainObject["size"].toInt();
    return result;
}

bool JsonParser::hasLibsFileInfo() const
{
    return jsonObject["libs"].isObject();
}

QList<FileInfo> JsonParser::getLibsFileInfo() const
{
    QList<FileInfo> result;

    QJsonObject libs = jsonObject["libs"].toObject();
    foreach ( QString key, libs.keys() )
    {
        FileInfo info;
        info.name = key;
        info.hash = libs[key].toObject()["hash"].toString();
        info.size = libs[key].toObject()["size"].toInt();

        result << info;
    }

    return result;
}

bool JsonParser::hasLibFileInfo(const QString &lib) const
{
    return jsonObject["libs"].toObject()[lib].isObject();
}

FileInfo JsonParser::getLibFileInfo(const QString &lib) const
{
    QJsonObject libs = jsonObject["libs"].toObject();

    FileInfo info;
    info.name = lib;
    info.hash = libs[lib].toObject()["hash"].toString();
    info.size = libs[lib].toObject()["size"].toInt();

    return info;
}

bool JsonParser::hasAddonsFilesInfo() const
{
    return jsonObject["files"].toObject()["index"].isObject()
           && jsonObject["files"].toObject()["mutables"].isArray();
}

QList<FileInfo> JsonParser::getAddonsFilesInfo() const
{
    QList<FileInfo> result;

    QJsonArray mutables = jsonObject["files"].toObject()["mutables"].toArray();
    QStringList mutablesList;
    foreach (QJsonValue value, mutables)
    {
        mutablesList << value.toString();
    }

    QJsonObject files = jsonObject["files"].toObject()["index"].toObject();
    foreach ( QString key, files.keys() )
    {
        FileInfo info;
        info.name = key;
        info.hash = files[key].toObject()["hash"].toString();
        info.size = files[key].toObject()["size"].toInt();

        if ( mutablesList.contains(key) )
        {
            info.isMutable = true;
        }

        result << info;
    }

    return result;
}

QHash<QString, FileInfo> JsonParser::getAddonsFilesInfoHashMap() const
{
    QHash<QString, FileInfo> result;

    QJsonArray mutables = jsonObject["files"].toObject()["mutables"].toArray();
    QStringList mutablesList;
    foreach (QJsonValue value, mutables)
    {
        mutablesList << value.toString();
    }

    QJsonObject files = jsonObject["files"].toObject()["index"].toObject();
    foreach ( QString key, files.keys() )
    {
        FileInfo info;
        info.name = key;
        info.hash = files[key].toObject()["hash"].toString();
        info.size = files[key].toObject()["size"].toInt();

        if ( mutablesList.contains(key) )
        {
            info.isMutable = true;
        }

        result.insert(key, info);
    }

    return result;
}

bool JsonParser::hasAssetsList() const
{
    return jsonObject["objects"].isObject();
}

QList<FileInfo> JsonParser::getAssetsList() const
{
    QList<FileInfo> result;

    QJsonObject files = jsonObject["objects"].toObject();
    foreach ( QString key, files.keys() )
    {
        QString hash = files[key].toObject()["hash"].toString();
        QString name = hash.mid(0, 2) + "/" + hash;
        quint64 size = files[key].toObject()["size"].toInt();

        FileInfo info;
        info.hash = hash;
        info.name = name;
        info.size = size;

        result << info;
    }

    return result;
}
