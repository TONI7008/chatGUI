#include "assistantbubble.h"

static const QString kBubbleCSS = R"(
    body,p { font-family:'Segoe UI',system-ui,sans-serif; font-size:14px;
              line-height:1.6; margin:0; padding:0; }
    h1,h2,h3 { margin:6px 0 2px; font-size:15px; }
    pre  { background:#0d0d0d; border:1px solid #333; border-radius:6px;
           padding:10px 14px; overflow-x:auto; margin:6px 0; }
    code { font-family:'Cascadia Code','Fira Code',monospace; font-size:13px; color:#cdd6f4; }
    ul,ol{ margin:4px 0; padding-left:20px; }
    li   { margin:2px 0; }
    a    { color:#7aa2f7; }
    strong { color:#e2e8f0; }
    em   { color:#c0caf5; }
    hr   { border:none; border-top:1px solid #2a2a2a; margin:8px 0; }
    blockquote { border-left:3px solid #444; margin:4px 0; padding-left:12px; color:#888; }
)";
#include "md4qt/src/parser.h"
#include "md4qt/src/html.h"



/* ═══════════════════════════════════════════════════════════════
   Shared helpers
   ═══════════════════════════════════════════════════════════════ */

static QString mdToHtml(const QString& md)
{
    MD::Parser parser;
    QString copy = md;
    QTextStream ts(&copy, QIODevice::ReadOnly);
    auto doc = parser.parse(ts, QString(), QString());
    return MD::toHtml(doc);
}
/* ═══════════════════════════════════════════════════════════════
   AssistantBubble — streams tokens live
   ═══════════════════════════════════════════════════════════════ */

AssistantBubble::AssistantBubble(QWidget* parent) : QFrame(parent)
{
    setObjectName("AssistantBubble");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setStyleSheet("AssistantBubble { background: transparent; border:none; }");

    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 4, 0, 4);
    lay->setSpacing(0);

    // Small avatar circle
    QHBoxLayout* header = new QHBoxLayout();
    header->setContentsMargins(0, 0, 0, 6);
    QLabel* avatar = new QLabel(this);
    avatar->setFixedSize(28, 28);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet(R"(
        background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
            stop:0 #7aa2f7, stop:1 #bb9af7);
        border-radius: 14px;
        color: white;
        font-size: 12px;
        font-weight: 700;
    )");
    avatar->setText("AI");
    header->addWidget(avatar);
    header->addSpacing(8);
    QLabel* name = new QLabel("Assistant", this);
    name->setStyleSheet("color:#555; font-size:12px;");
    header->addWidget(name);
    header->addStretch();
    lay->addLayout(header);

    m_label = new QLabel(this);
    m_label->setTextFormat(Qt::RichText);
    m_label->setWordWrap(true);
    m_label->setOpenExternalLinks(true);
    m_label->setTextInteractionFlags(Qt::TextSelectableByMouse
                                     | Qt::LinksAccessibleByMouse);
    m_label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    m_label->setStyleSheet("color:#ececec; font-size:14px; background:rgb(30,30,30);");
    lay->addWidget(m_label);
}

void AssistantBubble::appendChunk(const QString& chunk)
{
    m_accumulated += chunk;
    // During streaming show plain text with cursor — fast and avoids
    // re-parsing partial markdown that would produce broken HTML
    m_label->setText("<style>" + kBubbleCSS + "</style>"
                     + m_accumulated.toHtmlEscaped().replace("\n","<br>")
                     + "<span style='color:#4a9eff'>▌</span>");
    adjustSize();
}

void AssistantBubble::finalise(const QString& fullMarkdown)
{
    m_accumulated = fullMarkdown;
    m_label->setText("<style>" + kBubbleCSS + "</style>" + renderMarkdown(fullMarkdown));
    adjustSize();
}

QString AssistantBubble::renderMarkdown(const QString& md)
{
    return mdToHtml(md);
}
