#include "checkinfo.h"

const char* Ttyh::Json::CheckInfo::keyHash = "hash";
const char* Ttyh::Json::CheckInfo::keySize = "size";

Ttyh::Json::CheckInfo::CheckInfo()
{
    hash = "";
    size = 0;
}

Ttyh::Json::CheckInfo::CheckInfo(const QJsonObject &jObject)
{
    hash = jObject[keyHash].toString();
    size = jObject[keySize].toInt();
}
