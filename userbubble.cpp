#include "userbubble.h"

/* ═══════════════════════════════════════════════════════════════
   UserBubble
   ═══════════════════════════════════════════════════════════════ */

UserBubble::UserBubble(const QString& text, QWidget* parent) : QFrame(parent)
{
    setObjectName("UserBubble");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setStyleSheet("UserBubble { background:transparent; border:none; }");

    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 4, 0, 4);
    lay->setSpacing(0);

    // Header
    QHBoxLayout* header = new QHBoxLayout();
    header->setContentsMargins(0, 0, 0, 6);
    header->addStretch();
    QLabel* name = new QLabel("You", this);
    name->setStyleSheet("color:#555; font-size:12px;");
    header->addWidget(name);
    header->addSpacing(8);
    QLabel* avatar = new QLabel(this);
    avatar->setFixedSize(28, 28);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet(R"(
        background: #2a2a2a;
        border-radius: 14px;
        color: #888;
        font-size: 12px;
        font-weight: 700;
    )");
    avatar->setText("U");
    header->addWidget(avatar);
    lay->addLayout(header);

    // Message — right-aligned pill
    QHBoxLayout* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->addStretch(1);

    QLabel* bubble = new QLabel(this);
    bubble->setTextFormat(Qt::PlainText);
    bubble->setWordWrap(true);
    bubble->setText(text);
    bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bubble->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    bubble->setMaximumWidth(520);
    bubble->setStyleSheet(R"(
        background: #2f2f2f;
        color: #ececec;
        border-radius: 12px;
        border-bottom-right-radius: 3px;
        padding: 10px 14px;
        font-size: 14px;
        line-height: 1.5;
    )");
    row->addWidget(bubble, 0, Qt::AlignRight | Qt::AlignTop);
    lay->addLayout(row);
}
