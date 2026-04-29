#include "chathistoryitem.h"
#include <QStyle>

ChatHistoryItem* ChatHistoryItem::currentSelected = nullptr;
QList<ChatHistoryItem*> ChatHistoryItem::list;

ChatHistoryItem::ChatHistoryItem(const SessionInfo& info, QWidget* parent)
    : QWidget(parent)
    , m_info(info)
    , m_selected(false)
{
    setFixedHeight(56);
    setCursor(Qt::PointingHandCursor);

    setObjectName("historyItem");

    setStyleSheet(R"(
#historyItem {
    background: rgb(56,56,56);
    border-radius:8px;
}

#historyItem:hover {
    background: rgb(30,30,30);
}

#historyItem[selected="true"] {
    background: rgb(60,60,60);
}
)");

    QVBoxLayout* vl = new QVBoxLayout(this);
    vl->setContentsMargins(1, 1, 1, 1);
    vl->setSpacing(1);

    m_timeLabel = new QLabel(info.timestamp.toString("dd MMM hh:mm"), this);
    m_timeLabel->setStyleSheet("QLabel{color: rgb(140,140,140); font-size: 10px; border: none; background: transparent;}");

    QString preview = info.preview;
    if (preview.length() > kPreviewMaxLen)
        preview = preview.left(kPreviewMaxLen - 1) + QChar(0x2026);

    m_previewLabel = new QLabel(preview, this);
    m_previewLabel->setStyleSheet("QLabel{color: rgb(210,210,210); font-size: 12px; border: none; background: transparent;}");
    m_previewLabel->setWordWrap(false);

    vl->addWidget(m_timeLabel);
    vl->addWidget(m_previewLabel);

    list.append(this);
    setProperty("selected", true);
}
ChatHistoryItem::~ChatHistoryItem(){
    list.removeAll(this);

    if (currentSelected == this)
        currentSelected = nullptr;

};
void ChatHistoryItem::setSelected(bool s)
{
    if (m_selected == s) return; // Évite les updates inutiles

    m_selected = s;

    if(m_selected){
        if(currentSelected){
            currentSelected->setSelected(false);
        }
        currentSelected=this;
        style()->polish(this);
    }else{
        style()->unpolish(this);
    }

    update();
}

void ChatHistoryItem::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        SessionInfo inf=m_info;
        emit clicked(inf); // Le parent (la liste) devra appeler setSelected(true)
    }
    QWidget::mousePressEvent(e);
}
