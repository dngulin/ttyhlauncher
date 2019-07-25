#include "checkinfo.h"

Ttyh::Json::CheckInfo::CheckInfo()
{
    hash = "";
    size = 0;
}

Ttyh::Json::CheckInfo::CheckInfo(const QJsonObject &jObject)
{
    hash = jObject["hash"].toString();
    size = jObject["size"].toInt();
}
