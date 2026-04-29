#ifndef ASSISTANTBUBBLE_H
#define ASSISTANTBUBBLE_H
#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>


/* ── Live assistant bubble (streams tokens in) ──────────────────────────── */
class AssistantBubble : public QFrame
{
    Q_OBJECT
public:
    explicit AssistantBubble(QWidget* parent = nullptr);
    void appendChunk(const QString& chunk);
    void finalise(const QString& fullMarkdown); // re-render as proper HTML

private:
    QString   m_accumulated;
    QLabel*   m_label = nullptr;

    QString renderMarkdown(const QString& md);
};


#endif // ASSISTANTBUBBLE_H
