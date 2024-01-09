#include <QtCore/QRegularExpression>
#include "platform.h"

bool Ttyh::Utils::Platform::checkRules(const QList<Json::Rule> &rules)
{
    if (rules.isEmpty())
        return true;

    auto allowed = false;

    auto checkOsVersion = [] (const QString &version) {
        QRegularExpression re(version);
        return re.match(QSysInfo::productVersion()).hasMatch();
    };

    for (const auto& rule : rules) {
        auto matchOsName = rule.osName.isEmpty() || rule.osName == getOsName();
        auto matchOsVersion = rule.osVersion.isEmpty() || checkOsVersion(rule.osVersion);
        auto matchOsArch = rule.osArch.isEmpty() || rule.osArch == getArch();

        auto match = matchOsName && matchOsVersion && matchOsArch;
        if (!match)
            continue;

        allowed = rule.action == "allow";
    }

    return allowed;
}

// From: "package.dot.separated:name:version[:classifier]"
// To:   "package/slash/separated/name/version/name-version[-classifier].jar"
Ttyh::Utils::Platform::LibPathInfo
Ttyh::Utils::Platform::getLibraryPathInfo(const Json::LibraryInfo &libInfo)
{
    auto tokens = libInfo.name.split(':');

    auto package = tokens[0].replace('.', '/');
    auto name = tokens[1];
    auto version = tokens[2];

    QString classifier;

    if (tokens.count() > 3) {
        classifier = tokens[3];
    } else {
        auto os = getOsName();
        if (libInfo.natives.contains(os)) {
            classifier = libInfo.natives[os];
        }
    }

    if (!classifier.isEmpty()) {
        classifier.replace("${arch}", getWordSize()); // QString have only a mutable `replace`
        return {
            QString("%1/%2/%3/%2-%3-%4.jar").arg(package, name, version, classifier),
            true,
        };
    }

    return {
        QString("%1/%2/%3/%2-%3.jar").arg(package, name, version),
        false,
    };
}

QString Ttyh::Utils::Platform::getOsName()
{
#ifdef Q_OS_LINUX
    return "linux";
#endif
#ifdef Q_OS_OSX
    return "osx";
#endif
#ifdef Q_OS_WIN
    return "windows";
#endif
}

QString Ttyh::Utils::Platform::getOsVersion()
{
    return QSysInfo::prettyProductName();
}

QString Ttyh::Utils::Platform::getWordSize()
{
    return QString::number(QSysInfo::WordSize);
}

QString Ttyh::Utils::Platform::getArch()
{
#if Q_PROCESSOR_WORDSIZE == 4
    return "x86";
#elif Q_PROCESSOR_WORDSIZE == 8
    return "x86_64";
#else
    return "unknown";
#endif
}

QChar Ttyh::Utils::Platform::getClassPathSeparator()
{
#ifdef Q_OS_WIN
    return ';';
#else
    return ':';
#endif
}
