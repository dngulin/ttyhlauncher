#include <QtWidgets/QMessageBox>
#include <QtCore/QStandardPaths>
#include <QtWidgets/QFileDialog>
#include "profiledialog.h"
#include "ui_profiledialog.h"

ProfileDialog::ProfileDialog(QWidget *parent, const QString &profileName,
                             const ProfileData &profileData, const QHash<QString, Prefix> &prefixes)
    : QDialog(parent), ui(new Ui::ProfileDialog)
{
    ui->setupUi(this);

    ui->editName->setText(profileName);

    auto prefixList = prefixes.values();
    std::sort(prefixList.begin(), prefixList.end(), Prefix::less);

    for (const auto& prefix : prefixList) {
        ui->comboPrefix->addItem(prefix.name, prefix.id);
    }
    ui->comboPrefix->setCurrentText(prefixes[profileData.version.prefix].name);
    connect(ui->comboPrefix, &QComboBox::currentTextChanged, [=](const QString &) {
        auto prefixId = ui->comboPrefix->currentData().toString();
        if (!prefixes.contains(prefixId))
            return;

        ui->comboVersion->clear();
        for (const auto& version : prefixes[prefixId].versions) {
            if (version == Prefix::latestVersionAlias) {
                auto latestName = tr("Latest version");
                auto latestId = prefixes[prefixId].latestVersionId;
                if (latestId.isEmpty() || latestId == Prefix::latestVersionAlias)
                    latestId = tr("unknown");

                ui->comboVersion->addItem(QString("%1 (%2)").arg(latestName, latestId), version);
            } else {
                ui->comboVersion->addItem(version, version);
            }
        }
    });

    connect(ui->buttonJavaPath, &QToolButton::clicked, [=](bool) {
        auto dir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        auto path = QFileDialog::getOpenFileName(this, tr("Select the java executable"), dir);
        if (!path.isEmpty())
            ui->editJavaPath->setText(path);
    });

    connect(ui->saveButton, &QPushButton::clicked, [=](bool) { saveClickedInternal(); });

    ui->checkBoxJavaPath->setChecked(profileData.useCustomJavaPath);
    ui->checkBoxJavaArgs->setChecked(profileData.useCustomJavaArgs);

    ui->editJavaPath->setText(profileData.customJavaPath);
    ui->editJavaArgs->setText(profileData.customJavaArgs);

    ui->boxWindowSize->setChecked(profileData.setWindowSizeOnRun);
    switch (profileData.windowSizeMode) {

    case WindowSizeMode::FullScreen:
        ui->radioFullscreen->setChecked(true);
        break;
    case WindowSizeMode::LauncherLike:
        ui->radioLauncherSize->setChecked(true);
        break;
    case WindowSizeMode::Specified:
        ui->radioSpecifiedSize->setChecked(true);
        break;
    }
    ui->spinWidth->setValue(profileData.windowSize.width());
    ui->spinHeight->setValue(profileData.windowSize.height());

    // Set the initial state calling slots
    ui->comboPrefix->currentTextChanged(ui->comboPrefix->currentText());
    ui->checkBoxJavaPath->toggled(ui->checkBoxJavaPath->isChecked());
    ui->checkBoxJavaArgs->toggled(ui->checkBoxJavaArgs->isChecked());
    ui->radioSpecifiedSize->toggled(ui->radioSpecifiedSize->isChecked());

    // Select the current version
    for (int idx = 0; idx < ui->comboVersion->count(); idx++) {
        auto version = ui->comboVersion->itemData(idx).toString();
        if (version == profileData.version.id) {
            ui->comboVersion->setCurrentIndex(idx);
            break;
        }
    }

    // Adjust widget sizes
    auto fm = ui->editJavaArgs->fontMetrics();
    ui->editJavaArgs->setMinimumWidth(fm.averageCharWidth() * 32);
    adjustSize();
}

ProfileDialog::~ProfileDialog()
{
    delete ui;
}

void ProfileDialog::saveClickedInternal()
{
    ProfileData data;
    data.version = FullVersionId(ui->comboPrefix->currentData().toString(),
                                 ui->comboVersion->currentData().toString());

    data.useCustomJavaPath = ui->checkBoxJavaPath->isChecked();
    data.customJavaPath = ui->editJavaPath->text().trimmed();

    data.useCustomJavaArgs = ui->checkBoxJavaArgs->isChecked();
    data.customJavaArgs = ui->editJavaArgs->text().trimmed();

    data.setWindowSizeOnRun = ui->boxWindowSize->isChecked();
    data.windowSizeMode = getSelectedMode();
    data.windowSize = QSize(ui->spinWidth->value(), ui->spinHeight->value());

    emit saveClicked(ui->editName->text().trimmed(), data);
}

WindowSizeMode ProfileDialog::getSelectedMode() const
{
    if (ui->radioLauncherSize->isChecked())
        return WindowSizeMode::LauncherLike;

    if (ui->radioSpecifiedSize->isChecked())
        return WindowSizeMode::Specified;

    return WindowSizeMode::FullScreen;
}

void ProfileDialog::showRenameError(RenameResult result)
{
    switch (result) {
    case RenameResult::Success:
        showError(tr("Developer is crazy!"));
        break;
    case RenameResult::OldNameDoesNotExist:
        showError(tr("Selected profile does not exist!"));
        break;
    case RenameResult::NewNameAlreadyExists:
        showError(tr("There is another profile with the same name!"));
        break;
    case RenameResult::InvalidName:
        showError(tr("Invalid profile name!"));
        break;
    case RenameResult::IOError:
        showError(tr("Failed to write profile data!"));
        break;
    default:
        showError(tr("Unknown error!"));
    }
}

void ProfileDialog::showCreateError(CreateResult result)
{
    switch (result) {

    case CreateResult::Success:
        showError(tr("Developer is crazy!"));
        break;
    case CreateResult::AlreadyExists:
        showError(tr("There is another profile with the same name!"));
        break;
    case CreateResult::InvalidName:
        showError(tr("Invalid profile name!"));
        break;
    case CreateResult::IOError:
        showError(tr("Failed to write profile data!"));
        break;
    }
}

void ProfileDialog::showError(const QString &message)
{
    QMessageBox::critical(this, tr("Error"), message);
}
