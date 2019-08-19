#include <QtGui/QFontDatabase>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QScrollBar>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), onlineMode(false)
{
    ui->setupUi(this);
    hideTask();

    ui->textLog->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    ui->textLog->document()->setMaximumBlockCount(500);

    connect(ui->buttonPlay, &QPushButton::clicked, [=]() { emit playClicked(); });
    connect(ui->buttonCancelTask, &QPushButton::clicked, [=]() {
        ui->buttonCancelTask->setEnabled(false);
        emit taskCancelled();
    });
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
}

QString MainWindow::getSelectedProfile() const
{
    return ui->comboBoxProfiles->currentText();
}

bool MainWindow::isSavePasswordEnabled() const
{
    return true;
}

bool MainWindow::isOnline() const
{
    return onlineMode;
}

void MainWindow::setOnline(bool online)
{
    onlineMode = online;
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
    auto strCount = QString::number(count);
    auto strSize = QString::number(size);

    auto title = tr("Downloads are required");
    auto msg = tr("Need to download %1 files with the total size %2. Continue?", "", count);
    auto result = QMessageBox::question(this, title, msg.arg(strCount, strSize));

    return result == QMessageBox::Yes;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    emit closed();
    QWidget::closeEvent(event);
}
