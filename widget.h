#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QResizeEvent>

#include "chathistoryitem.h"   // SessionInfo

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class ChatWidget;
class SettingsPanel;
class DynamicFrameAssistant;

class Widget : public QWidget
{
    Q_OBJECT
public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

signals:
    void requestReload();

protected:
    void resizeEvent(QResizeEvent* e) override;

private slots:
    void send();
    void toggleSettings();
    void newChat();

    /* Repopulate ui->historyLayout from disk. Safe to call any time. */
    void refreshHistoryPanel();

    /* Open (or switch to) a past session identified by SessionInfo. */
    void openSession(const SessionInfo& info);

private:
    void setInputEnabled(bool on);

    Ui::Widget*            ui              = nullptr;
    ChatWidget*            m_currentChat   = nullptr;
    SettingsPanel*         m_settingsPanel = nullptr;
    DynamicFrameAssistant* m_frameAssistant  = nullptr;
    DynamicFrameAssistant* m_frameAssistant2 = nullptr;

    bool eventFilter(QObject* obj, QEvent* event) override;
};

#endif // WIDGET_H
