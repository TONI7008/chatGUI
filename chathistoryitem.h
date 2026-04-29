#ifndef CHATHISTORYITEM_H
#define CHATHISTORYITEM_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QDateTime>

/* ── Plain data bag returned by CWrapper::listSessions() ───────────────────
 *
 *  path      – absolute / relative path to the .cbor file
 *  timestamp – parsed from the filename; used for display and sorting
 *  preview   – first user message (truncated), shown in the sidebar
 * ─────────────────────────────────────────────────────────────────────────── */
struct SessionInfo
{
    QString  path;
    QDateTime timestamp;
    QString  preview;

    // Opérateur d'égalité (const-correct)
    bool operator==(const SessionInfo& other) const {
        return path == other.path &&
               timestamp == other.timestamp &&
               preview == other.preview;
    }

    // Il est aussi de bonne pratique d'ajouter l'opérateur d'inégalité
    bool operator!=(const SessionInfo& other) const {
        return !(*this == other);
    }
};

/* ── Sidebar tile for one past session ────────────────────────────────────── */
class ChatHistoryItem : public QWidget
{
    Q_OBJECT
public:
    explicit ChatHistoryItem(const SessionInfo& info, QWidget* parent = nullptr);
    ~ChatHistoryItem();

    const SessionInfo& sessionInfo() const { return m_info; }
    void setSelected(bool s);
    static QList<ChatHistoryItem*> list;

signals:
    void clicked(const SessionInfo& info);

protected:
    void mousePressEvent(QMouseEvent* e) override;

private:
    SessionInfo m_info;
    QLabel*     m_timeLabel    = nullptr;
    QLabel*     m_previewLabel = nullptr;
    bool isSelected() const;

    bool m_selected=false;

    static constexpr int kPreviewMaxLen = 35;
    static ChatHistoryItem* currentSelected;
};

#endif // CHATHISTORYITEM_H
