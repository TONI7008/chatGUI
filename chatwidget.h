#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QThread>

#include "twidget.h"
#include "cwrapper.h"
#include "chathistoryitem.h"

class AssistantBubble;

class ChatWidget : public TWidget
{
    Q_OBJECT
public:
    /* New blank session */
    explicit ChatWidget(QWidget* parent = nullptr);

    /* Restored session — loads history from disk and replays bubbles */
    explicit ChatWidget(const SessionInfo& session, QWidget* parent = nullptr);

    ~ChatWidget();

    CWrapper* wrapper();

signals:
    /* Public: emit this to send a message (GUI thread) */
    void requestChat(const QString& message);
    void requestReload();
    void streamFinished();

signals:
    /* Internal: queued connection to CWrapper::chat on the worker thread.
     * Declared private so nothing outside the class accidentally emits it. */
    void sendChatRequest(const QString& message);

private slots:
    void onStreamStarted();
    void onResponseChunk(const QString& chunk);
    void onStreamFinished();
    void onResponseReceived(const QString& full);
    void onErrorOccurred(const QString& error);
    void onModelLoaded(bool ok);

private:
    void             setupThread();
    void             replayHistory();
    void             addUserBubble(const QString& text);
    AssistantBubble* addAssistantBubble();
    void             scrollToBottom();

    static constexpr int kChatMargin = 16;

    QVBoxLayout*     m_chatLayout   = nullptr;
    CWrapper*        m_wrapper      = nullptr;
    QThread*         m_workerThread = nullptr;
    AssistantBubble* m_liveBubble   = nullptr;
};

#endif // CHATWIDGET_H
