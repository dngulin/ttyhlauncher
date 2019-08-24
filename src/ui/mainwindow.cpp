#include <QtGui/QFontDatabase>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QCheckBox>
#include "mainwindow.h"
#include "aboutdialog.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      actionCreateProfile(nullptr),
      actionEditProfile(nullptr),
      actionRemoveProfile(nullptr)
{
    ui->setupUi(this);
    hideTask();

    auto profilesMenu = new QMenu(this);
    actionCreateProfile = profilesMenu->addAction(tr("Create..."));
    actionEditProfile = profilesMenu->addAction(tr("Edit..."));
    actionRemoveProfile = profilesMenu->addAction(tr("Remove..."));

    ui->buttonProfiles->setMenu(profilesMenu);

    setOnline(false);
    connect(ui->actionPlayOffine, &QAction::triggered,
            [=](bool offline) { emit onlineModeSwitched(!offline); });

    connect(ui->actionUploadSkin, &QAction::triggered, [=](bool) { emit uploadSkinClicked(); });

    ui->textLog->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    ui->textLog->document()->setMaximumBlockCount(500);

    connect(ui->buttonPlay, &QPushButton::clicked, [=]() { emit playClicked(); });
    connect(ui->buttonCancelTask, &QPushButton::clicked, [=]() {
        ui->buttonCancelTask->setEnabled(false);
        emit taskCancelled();
    });

    connect(ui->actionAbout, &QAction::triggered, [=](bool checked) { AboutDialog(this).exec(); });

    connect(actionCreateProfile, &QAction::triggered, [=]() { emit profileCreateClicked(); });
    connect(actionEditProfile, &QAction::triggered, [=]() { emit profileEditClicked(); });
    connect(actionRemoveProfile, &QAction::triggered, [=]() { emit profileRemoveClicked(); });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setLocked(bool locked)
{
    ui->menubar->setEnabled(!locked);
    ui->widgetForm->setEnabled(!locked);
}

void MainWindow::appendLog(const QString &msg)
{
    ui->textLog->moveCursor(QTextCursor::End);

    QRegularExpression urlRegEx(R"((https?://[A-Za-z0-9\.\-\?_=~#/]+))");
    auto matchIterator = urlRegEx.globalMatch(msg);

    if (matchIterator.hasNext()) {
        QString pattern("<a href=\"%1\">%2</a>");
        auto lastIndex = 0;
        auto fmt = ui->textLog->currentCharFormat();

        while (matchIterator.hasNext()) {
            auto match = matchIterator.next();
            auto begin = match.capturedStart(1);
            auto end = match.capturedEnd(1);

            auto pre = msg.mid(lastIndex, begin);
            ui->textLog->insertPlainText(pre);

            auto url = match.captured(1);
            ui->textLog->insertHtml(pattern.arg(url, url.toHtmlEscaped()));
            ui->textLog->setCurrentCharFormat(fmt);

            lastIndex = end;
        }

        ui->textLog->insertPlainText(msg.mid(lastIndex, msg.length() - lastIndex));

    } else {
        ui->textLog->insertPlainText(msg);
    }

    ui->textLog->insertPlainText("\n");

    auto sb = ui->textLog->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void MainWindow::setUserName(const QString &username)
{
    ui->lineEditPlayerName->setText(username);
}

QString MainWindow::getUserName() const
{
    return ui->lineEditPlayerName->text();
}

void MainWindow::setPassword(const QString &password)
{
    ui->lineEditPassword->setText(password);
}

QString MainWindow::getPassword() const
{
    return ui->lineEditPassword->text();
}

void MainWindow::setProfiles(const QStringList &profiles, const QString &selected)
{
    ui->comboBoxProfiles->clear();
    ui->comboBoxProfiles->addItems(profiles);
    ui->comboBoxProfiles->setCurrentText(selected);

    auto hasProfiles = !profiles.isEmpty();
    actionEditProfile->setEnabled(hasProfiles);
    actionRemoveProfile->setEnabled(hasProfiles);
}

QString MainWindow::getSelectedProfile() const
{
    return ui->comboBoxProfiles->currentText();
}

void MainWindow::setSavePassword(bool savePassword)
{
    ui->actionSavePassword->setChecked(savePassword);
}

bool MainWindow::isSavePassword() const
{
    return ui->actionSavePassword->isChecked();
}

void MainWindow::setHideOnRun(bool hideOnRun)
{
    ui->actionHideWindow->setChecked(hideOnRun);
}

bool MainWindow::isHideOnRun() const
{
    return ui->actionHideWindow->isChecked();
}

void MainWindow::setOnline(bool online)
{
    ui->actionPlayOffine->setChecked(!online);
    ui->buttonPlay->setText(online ? tr("Play") : tr("Play (Offline)"));
}

bool MainWindow::isOnline() const
{
    return !ui->actionPlayOffine->isChecked();
}

void MainWindow::showTask(const QString &taskName)
{
    ui->labelTaskTarget->setText("");
    ui->labelTaskTitle->setText(taskName);

    ui->progressBarTask->setRange(0, 100);
    ui->progressBarTask->setValue(0);

    ui->buttonCancelTask->setEnabled(true);
    ui->frameTask->show();
}

void MainWindow::setTaskRange(int min, int max)
{
    ui->progressBarTask->setRange(min, max);
}

void MainWindow::setTaskTarget(int id, const QString &name)
{
    ui->progressBarTask->setValue(id);
    ui->labelTaskTarget->setText(name);
}

void MainWindow::hideTask()
{
    ui->frameTask->hide();
}

void MainWindow::showMessage(const QString &message)
{
    QMessageBox::information(this, tr("Information"), message);
}

void MainWindow::showError(const QString &error)
{
    QMessageBox::critical(this, tr("Oops! Something went wrong"), error);
}

bool MainWindow::askForDownloads(int count, quint64 size)
{
    auto cap = tr("Downloads are required");

    auto msg = tr("Need to download %n files with the total size", "", count);
    auto sze = getSizeString(size);
    auto ask = tr("Do you want to continue?");
    auto result = QMessageBox::question(this, cap, QString("%1 %2.\n%3").arg(msg, sze, ask));

    return result == QMessageBox::Yes;
}

bool MainWindow::askForProfileDeletion(const QString &profileName)
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(tr("Confirm the profile removing"));

    auto msg = tr("All profile data will be lost. Do you want to delete the '%1' profile?");
    box.setText(msg.arg(profileName));

    auto cb = new QCheckBox(tr("I'm sure I want to remove the profile"));
    box.setCheckBox(cb);

    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box.setDefaultButton(QMessageBox::No);

    auto btnYes = box.button(QMessageBox::Yes);
    btnYes->setEnabled(false);
    connect(cb, &QCheckBox::toggled, btnYes, &QPushButton::setEnabled);

    return box.exec() == QMessageBox::Yes;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    emit closed();
    QWidget::closeEvent(event);
}

QString MainWindow::getSizeString(quint64 size)
{
    constexpr auto gb = 1024 * 1024 * 1024;
    constexpr auto mb = 1024 * 1024;
    constexpr auto kb = 1024;

    QString fmt("%1 %2");
    auto l = QLocale::system();

    if (size > gb)
        return fmt.arg(l.toString(((double)size / gb), 'f', 2), tr("GiB"));

    if (size > mb)
        return fmt.arg(l.toString(((double)size / mb), 'f', 2), tr("MiB"));

    if (size > kb)
        return fmt.arg(l.toString(((double)size / kb), 'f', 2), tr("KiB"));

    return fmt.arg(l.toString(size), tr("B"));
}
