#include "chatwidget.h"

#include "assistantbubble.h"
#include "userbubble.h"
#include "cwrapper.h"

#include <QScrollArea>
#include <QScrollBar>
#include <QCborValue>
#include <QCborArray>
#include <QCborMap>

/* ── shared thread setup ──────────────────────────────────────────────────── */

void ChatWidget::setupThread()
{
    m_chatLayout = new QVBoxLayout(this);
    m_chatLayout->setAlignment(Qt::AlignTop);
    m_chatLayout->setSpacing(0);
    m_chatLayout->setContentsMargins(0, 24, 0, 120);
    setLayout(m_chatLayout);

    setEnableBackground(true);
    setBackgroundColor(QColor(30, 30, 30));
    setBorderRadius(18);
    setStyleSheet("background: rgb(30,30,30);");

    /* ── CWrapper on worker thread ─────────────────────────────────────────
     *
     *  IMPORTANT — connection types:
     *
     *  requestChat  →  m_wrapper->chat()
     *    Must be Qt::QueuedConnection (or AutoConnection across threads).
     *    Using a direct lambda on `this` (GUI thread) and calling
     *    m_wrapper->chat() inside it crosses thread affinity and causes:
     *      "Cannot create children for a parent that is in a different thread"
     *    because QNetworkReply is parented to _nam which lives on the worker.
     *
     *  The cleanest fix: connect the signal directly to the slot with
     *  AutoConnection — Qt will queue it automatically because sender and
     *  receiver live on different threads.
     * ─────────────────────────────────────────────────────────────────────── */
    m_workerThread = new QThread(this);
    m_wrapper->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::finished, m_wrapper, &QObject::deleteLater);
    connect(m_workerThread, &QThread::started,  m_wrapper, &CWrapper::init);

    // Worker → GUI  (AutoConnection, will be queued automatically)
    connect(m_wrapper, &CWrapper::responseReceived, this, &ChatWidget::onResponseReceived);
    connect(m_wrapper, &CWrapper::responseChunk,    this, &ChatWidget::onResponseChunk);
    connect(m_wrapper, &CWrapper::streamStarted,    this, &ChatWidget::onStreamStarted);
    connect(m_wrapper, &CWrapper::streamFinished,   this, &ChatWidget::onStreamFinished);
    connect(m_wrapper, &CWrapper::errorOccurred,    this, &ChatWidget::onErrorOccurred);
    connect(m_wrapper, &CWrapper::modelLoaded,      this, &ChatWidget::onModelLoaded);

    // GUI → worker: use direct signal-slot so Qt queues across threads.
    // Do NOT call m_wrapper->chat() from a GUI-thread lambda — that runs
    // the method body on the GUI thread regardless of thread affinity.
    connect(this,      &ChatWidget::sendChatRequest, m_wrapper, &CWrapper::chat);
    connect(this,      &ChatWidget::requestReload,   m_wrapper, &CWrapper::reloadModel);

    m_workerThread->start();
}

/* ── constructors ─────────────────────────────────────────────────────────── */

ChatWidget::ChatWidget(QWidget* parent)
    : TWidget{parent}
    , m_wrapper(new CWrapper())
{
    setupThread();

    // Wire public requestChat signal: add bubble on GUI side, then queue to worker
    connect(this, &ChatWidget::requestChat, this, [this](const QString& text) {
        addUserBubble(text);
        emit sendChatRequest(text);   // queued across threads — safe
    });
}

ChatWidget::ChatWidget(const SessionInfo& session, QWidget* parent)
    : TWidget{parent}
    , m_wrapper(new CWrapper())
{
    m_wrapper->loadSessionFromPath(session.path);

    setupThread();

    connect(this, &ChatWidget::requestChat, this, [this](const QString& text) {
        addUserBubble(text);
        emit sendChatRequest(text);
    });

    replayHistory();
}

ChatWidget::~ChatWidget()
{
    m_workerThread->quit();
    m_workerThread->wait();
}

/* ── history replay ───────────────────────────────────────────────────────── */

void ChatWidget::replayHistory()
{
    QFile f(m_wrapper->sessionPath());
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) return;

    const QCborValue root = QCborValue::fromCbor(f.readAll());
    if (!root.isArray()) return;

    for (const QCborValue& entry : root.toArray()) {
        if (!entry.isMap()) continue;
        const QCborMap  m    = entry.toMap();
        const QString   role = m[QStringLiteral("role")].toString();
        const QString   text = m[QStringLiteral("content")].toString();

        if (role == QLatin1String("user")) {
            addUserBubble(text);
        } else if (role == QLatin1String("assistant")) {
            AssistantBubble* b = addAssistantBubble();
            b->finalise(text);
        }
    }
    scrollToBottom();
}

/* ── Bubble factories ─────────────────────────────────────────────────────── */

void ChatWidget::addUserBubble(const QString& text)
{
    QWidget* row = new QWidget(this);
    row->setStyleSheet("background:transparent;");
    row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QHBoxLayout* hl = new QHBoxLayout(row);
    hl->setContentsMargins(kChatMargin, 0, kChatMargin, 0);
    hl->setSpacing(0);

    UserBubble* b = new UserBubble(text, row);
    hl->addWidget(b);
    row->setLayout(hl);

    m_chatLayout->addWidget(row);
}

AssistantBubble* ChatWidget::addAssistantBubble()
{
    QWidget* row = new QWidget(this);
    row->setStyleSheet("background:transparent;");
    row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QHBoxLayout* hl = new QHBoxLayout(row);
    hl->setContentsMargins(kChatMargin, 0, kChatMargin, 0);
    hl->setSpacing(0);

    AssistantBubble* b = new AssistantBubble(row);
    hl->addWidget(b);

    m_chatLayout->addWidget(row);
    return b;
}

/* ── slots ────────────────────────────────────────────────────────────────── */

void ChatWidget::onStreamStarted()
{
    m_liveBubble = addAssistantBubble();
    scrollToBottom();
}

void ChatWidget::onResponseChunk(const QString& chunk)
{
    if (m_liveBubble) {
        m_liveBubble->appendChunk(chunk);
        scrollToBottom();
    }
}

void ChatWidget::onStreamFinished()
{
    emit streamFinished();
}

void ChatWidget::onResponseReceived(const QString& full)
{
    if (m_liveBubble) {
        m_liveBubble->finalise(full);
        m_liveBubble = nullptr;
    }
    emit streamFinished();
    scrollToBottom();
}

void ChatWidget::onErrorOccurred(const QString& error)
{
    if (m_liveBubble) {
        m_liveBubble->finalise("**Error:** " + error.toHtmlEscaped());
        m_liveBubble = nullptr;
    } else {
        AssistantBubble* b = addAssistantBubble();
        b->finalise("**Error:** " + error.toHtmlEscaped());
    }
}

void ChatWidget::onModelLoaded(bool /*ok*/)
{
    emit streamFinished();
}

CWrapper* ChatWidget::wrapper()
{
    return m_wrapper;
}

/* ── scrollToBottom ───────────────────────────────────────────────────────────
 *
 *  Parent chain: ChatWidget → QStackedWidget (viewport child) → QScrollArea
 *  We walk up until we find the QScrollArea rather than hard-coding depth,
 *  so this survives any future reparenting.
 * ─────────────────────────────────────────────────────────────────────────── */
void ChatWidget::scrollToBottom()
{
    QWidget* w = parentWidget();
    while (w) {
        if (QScrollArea* area = qobject_cast<QScrollArea*>(w)) {
            QScrollBar* sb = area->verticalScrollBar();
            //sb->setValue(sb->maximum());
            return;
        }
        w = w->parentWidget();
    }
}
