#include "widget.h"
#include "./ui_widget.h"

#include <QHBoxLayout>
#include <QScrollBar>
#include <QTimer>
#include <QTextStream>
#include <QSizePolicy>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QDebug>
#include <QKeyEvent>
#include <QSpacerItem>

#include "settingspanel.h"
#include "chatwidget.h"
#include "chathistoryitem.h"
#include "cwrapper.h"
#include "dynamicframeassistant.h"

/* ═══════════════════════════════════════════════════════════════
   Widget
   ═══════════════════════════════════════════════════════════════ */

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    newChat();
    refreshHistoryPanel();

    /* ── DynamicFrameAssistant ──────────────────────────────────── */
    m_frameAssistant = new DynamicFrameAssistant(ui->scrollArea);
    ui->textFrame->setParent(ui->chatWidget);
    m_frameAssistant->addChildWidget(ui->textFrame,
                                     Qt::AlignBottom | Qt::AlignCenter, 20);
    m_frameAssistant->popWidget(ui->textFrame, true, 500);
    m_frameAssistant->setBlock(false);

    m_frameAssistant2 = new DynamicFrameAssistant(this);
    m_settingsPanel = new SettingsPanel(this);
    m_settingsPanel->setFixedWidth(width() - width() / 4);
    m_frameAssistant2->addChildWidget(m_settingsPanel, Qt::AlignHCenter, Qt::AlignVCenter);
    m_frameAssistant2->popWidget(m_settingsPanel, false);

    /* ── Scroll area ────────────────────────────────────────────── */
    ui->scrollArea->setWidgetResizable(true);

    /* ── When the stacked widget switches page, sync its height to the
     *    incoming page and reset the scroll position to the top.
     *
     *    Layout: QScrollArea -> (its widget) -> QStackedWidget -> ChatWidget
     *    Do NOT call scrollArea->setWidget() — that rips the stacked widget
     *    out of its parent and causes a segfault.
     *    Instead: resize the stacked widget to the new page's content height
     *    so the scroll area recomputes its scrollable range, then reset to 0. */
    connect(ui->stackedWidget, &QStackedWidget::currentChanged,
            this, [this](int /*index*/) {
                QWidget* current = ui->stackedWidget->currentWidget();
                if (!current) return;

                // Adopt the new page's preferred height (at least the viewport height
                // so there is no dead space when the chat is short).
                ui->stackedWidget->setMinimumHeight(
                    qMax(current->sizeHint().height(),
                         ui->scrollArea->viewport()->height()));

                // Reset to top; ChatWidget::scrollToBottom() fires during streaming.
                ui->scrollArea->verticalScrollBar()->setValue(0);
            });

    /* ── Settings panel ─────────────────────────────────────────── */
    m_settingsPanel->loadFrom(m_currentChat->wrapper());

    connect(m_settingsPanel, &SettingsPanel::accepted, this, [this]() {
        m_settingsPanel->saveTo(m_currentChat->wrapper());
        emit requestReload();
        toggleSettings();
    });
    connect(m_settingsPanel, &SettingsPanel::cancelled, this, [this]() {
        toggleSettings();
    });

    /* ── Wire UI buttons ────────────────────────────────────────── */
    connect(ui->sendButton,      &QPushButton::clicked, this, &Widget::send);

    connect(ui->hidePanelButton, &QPushButton::clicked, this, [this](bool show) {
        ui->panelFrame->setVisible(!show);
        ui->hidePanelButton->setIcon(
            QIcon(show ? "://Icons/collapserw.svg" : "://Icons/collapselw.svg"));
    });

    connect(ui->newchatButton,   &QPushButton::clicked, this, &Widget::newChat);

    connect(m_currentChat, &ChatWidget::streamFinished, this, [this] {
        setInputEnabled(true);
    });

    connect(ui->settingsButton, &QPushButton::clicked, this, [this] {
        m_frameAssistant2->popAll(true);
    });

    ui->textEdit->installEventFilter(this);
}

Widget::~Widget()
{
    delete ui;
    delete m_frameAssistant;
    delete m_frameAssistant2;
}

void Widget::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    m_settingsPanel->setFixedWidth(width() - width() / 4);
}

/* ── Helpers ─────────────────────────────────────────────────── */

void Widget::setInputEnabled(bool on)
{
    ui->sendButton->setEnabled(on);
    ui->textEdit->setReadOnly(!on);
}

void Widget::newChat()
{
    m_currentChat = new ChatWidget(ui->stackedWidget);
    ui->stackedWidget->addWidget(m_currentChat);
    ui->stackedWidget->setCurrentWidget(m_currentChat);
    // currentChanged fires → stacked widget resized, scroll reset to 0

    connect(m_currentChat, &ChatWidget::streamFinished, this, [this] {
        setInputEnabled(true);
    });

    connect(m_currentChat, &ChatWidget::streamFinished,
            this, &Widget::refreshHistoryPanel,
            Qt::UniqueConnection);

    refreshHistoryPanel();
}

/* ── refreshHistoryPanel ──────────────────────────────────────── */

void Widget::refreshHistoryPanel()
{
    while (QLayoutItem* item = ui->historyLayout->takeAt(0)) {
        if (QWidget* w = item->widget())
            w->deleteLater();
        delete item;
    }

    const QList<SessionInfo> sessions = CWrapper::listSessions(10);

    for (const SessionInfo& info : sessions) {
        ChatHistoryItem* item = new ChatHistoryItem(info,ui->historyDisplay);
        ui->historyLayout->addWidget(item);

        connect(item, &ChatHistoryItem::clicked, this, &Widget::openSession);
    }

    ui->historyLayout->addStretch();
}

/* ── openSession ──────────────────────────────────────────────── */

void Widget::openSession(const SessionInfo& info)
{
    // Re-use if already open
    for (auto &item : ChatHistoryItem::list){
        if(item->sessionInfo()==info){
            item->setSelected(true);
            break;
        }
    }
    for (int i = 0; i < ui->stackedWidget->count(); ++i) {
        ChatWidget* cw = qobject_cast<ChatWidget*>(ui->stackedWidget->widget(i));
        if (cw && cw->wrapper()->sessionPath() == info.path) {
            ui->stackedWidget->setCurrentWidget(cw);
            // currentChanged fires → height synced, scroll reset to 0
            m_currentChat = cw;
            return;
        }
    }

    ChatWidget* restored = new ChatWidget(info, ui->stackedWidget);
    ui->stackedWidget->addWidget(restored);
    ui->stackedWidget->setCurrentWidget(restored);


    m_currentChat = restored;

    connect(m_currentChat, &ChatWidget::streamFinished, this, [this] {
        setInputEnabled(true);
    });
}

/* ── Slots ───────────────────────────────────────────────────── */

void Widget::send()
{
    const QString text = ui->textEdit->toPlainText().trimmed();
    if (text.isEmpty()) return;

    ui->textEdit->clear();
    setInputEnabled(false);
    m_currentChat->requestChat(text);
}

void Widget::toggleSettings()
{
    m_frameAssistant2->popWidget(m_settingsPanel, !m_frameAssistant2->isPopped(m_settingsPanel));
}

/* ── Event filter ────────────────────────────────────────────── */

bool Widget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == ui->textEdit && event->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(event);
        if ((ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)
            && (ke->modifiers() & Qt::ControlModifier)) {
            send(); return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}
