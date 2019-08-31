#ifndef PROFILEDIALOG_H
#define PROFILEDIALOG_H

#include <QDialog>
#include "versions/prefix.h"
#include "profiles/profiledata.h"
#include "profiles/profilesopresults.h"

namespace Ui {
class ProfileDialog;
}

using namespace Ttyh::Versions;
using namespace Ttyh::Profiles;

class ProfileDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProfileDialog(QWidget *parent, const QString &profileName,
                           const ProfileData &profileData, const QHash<QString, Prefix> &prefixes);
    ~ProfileDialog() override;

    void showRenameError(RenameResult result);
    void showCreateError(CreateResult result);
    void showError(const QString &message);

signals:
    void saveClicked(const QString &resultName, const ProfileData &resultData);

private:
    Ui::ProfileDialog *ui;

    void saveClickedInternal();
    WindowSizeMode getSelectedMode() const;
};

#endif // PROFILEDIALOG_H
