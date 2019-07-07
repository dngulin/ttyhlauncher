#include <QtCore>
#include "installinfo.h"

InstallInfo::InstallInfo()
{
    qRegisterMetaType<InstallInfo>("InstallInfo");

    path = "";
    srcPath = "";
    hash = "";
    action = InstallAction::Update;
}
