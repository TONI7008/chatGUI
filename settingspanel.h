#ifndef SETTINGSPANEL_H
#define SETTINGSPANEL_H

#include <QWidget>
#include <QLineEdit>

#include "tframe.h"

class CWrapper;

namespace Ui {
class SettingsPanel;
}

class SettingsPanel : public TFrame
{
    Q_OBJECT

public:
    explicit SettingsPanel(QWidget *parent = nullptr);
    ~SettingsPanel();
    void loadFrom(CWrapper* w);
    void saveTo(CWrapper* w);

signals:
    void accepted();
    void cancelled();

private:
    Ui::SettingsPanel *ui;
    QLineEdit* m_ip    = nullptr;
    QLineEdit* m_port  = nullptr;
    QLineEdit* m_key   = nullptr;
};

#endif // SETTINGSPANEL_H
