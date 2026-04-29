#include "settingspanel.h"
#include "ui_settingspanel.h"

#include "cwrapper.h"
/* ═══════════════════════════════════════════════════════════════
   SettingsPanel
   ═══════════════════════════════════════════════════════════════ */

SettingsPanel::SettingsPanel(QWidget *parent)
    : TFrame(parent)
    , ui(new Ui::SettingsPanel)
{
    ui->setupUi(this);

    connect(ui->save,   &QPushButton::clicked, this, &SettingsPanel::accepted);
    connect(ui->cancel, &QPushButton::clicked, this, &SettingsPanel::cancelled);
    connect(ui->close, &QToolButton::clicked, this, &SettingsPanel::cancelled);


    //setModal();
    setWindowModality(Qt::WindowModal);
    setBorder(true);
    setBorderSize(5);
    setBorderColor();
}

SettingsPanel::~SettingsPanel()
{
    delete ui;
}

void SettingsPanel::loadFrom(CWrapper* w)
{
    ui->ip->setText(w->getServerIP());
    ui->port->setText(QString::number(w->getServerPort()));
    ui->key->setText(w->getApiKey());
}

void SettingsPanel::saveTo(CWrapper* w)
{
    w->setServerIP(ui->ip->text().trimmed());
    w->setServerPort(ui->port->text().trimmed().toInt());
    w->setApiKey(ui->key->text().trimmed());
}

