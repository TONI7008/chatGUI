#ifndef USERBUBBLE_H
#define USERBUBBLE_H

#include<QFrame>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>

/* ── Static user bubble ─────────────────────────────────────────────────── */
class UserBubble : public QFrame
{
    Q_OBJECT
public:
    explicit UserBubble(const QString& text, QWidget* parent = nullptr);
};


#endif // USERBUBBLE_H
