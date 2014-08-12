#include <QtCore>
#include <QObject>
#include <QStandardPaths>

#include "settings.h"

Settings* Settings::myInstance = 0;
Settings* Settings::instance() {
    if (myInstance == 0) myInstance = new Settings();
    return myInstance;
}

Settings::Settings() : QObject()
{
    QString setPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    settings = new QSettings(setPath + "/ttyhlauncher/config.ini", QSettings::IniFormat);

    clientStrIDs = new QStringList();
    clientNames = new QStringList();

    appendClient("classic", "Классический клиент");
    appendClient("industrial", "Индустриальный клиент");
}

void Settings::appendClient(QString strid, QString name) {
    clientStrIDs->append(strid);
    clientNames->append(name);
}

QStringList Settings::getClientsNames() { return *clientNames; }
int Settings::getClientId(QString name) { return clientNames->indexOf(name); }
int Settings::strIDtoID(QString strid) { return clientStrIDs->indexOf(strid); }
QString Settings::getClientName(int id) { return clientNames->at(id); }
QString Settings::getClientStrId(int id) { return clientStrIDs->at(id); }

int Settings::loadActiveClientId() {
    QString strid = settings->value("launcher/client", clientStrIDs->at(0)).toString();
    return strIDtoID(strid);
}

void Settings::saveActiveClientId(int id) {
    QString client = getClientStrId(id);
    settings->setValue("launcher/client", client);
}

QString Settings::loadLogin() { return settings->value("launcher/login", "Player").toString(); }
void Settings::saveLogin(QString login) { settings->setValue("launcher/login", login); }

bool Settings::loadPassStore() { return settings->value("launcher/save_password", false).toBool(); }
void Settings::savePassStore(bool state) { settings->setValue("launcher/save_password", state); }

// Password stored as base64-encoded string
QString Settings::loadPassword() {
    QString encodedPass = settings->value("launcher/password", "").toString();
    QByteArray ba; ba.append(encodedPass);
    return QByteArray::fromBase64(ba);
}

void Settings::savePassword(QString password) {
    QByteArray ba; ba.append(password);
    settings->setValue("launcher/password", QString(ba.toBase64()));
}

QRect Settings::loadWindowGeometry() {return qvariant_cast<QRect>(settings->value("launcher/window_geometry", QRect(-1, -1, 600, 400))); }
void Settings::saveWindowGeometry(QRect geom) { settings->setValue("launcher/window_geometry", geom); }

bool Settings::loadMaximizedState() {return settings->value("launcher/window_maximized", false).toBool();}
void Settings::saveMaximizedState(bool state) { settings->setValue("launcher/window_maximized", state); }
