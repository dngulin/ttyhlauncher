#ifndef PROFILEINFO_H
#define PROFILEINFO_H

#include <QtCore/QString>
#include "profiledata.h"

namespace Ttyh {
namespace Profiles {
struct ProfileInfo {
    QString name;
    ProfileData data;
};
}
}

#endif // PROFILEINFO_H
