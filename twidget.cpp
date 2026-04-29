#include "twidget.h"

TWidget::TWidget(QWidget *parent)
    : QWidget(parent),
    m_enableBackground(false)
{


}

TWidget::~TWidget() {

}

void TWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        emit rightClicked(event->pos());
    }else if(event->button() == Qt::LeftButton){
        emit clicked(event->pos().toPointF());
    }
    QWidget::mousePressEvent(event);

}

void TWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked(event->pos());
    }
}

void TWidget::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);

    if (windowFlags() & Qt::Dialog) {
        QPainter painter(this);
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.fillRect(rect(), Qt::transparent);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath path;
    QRect r = rect();
    int rSize = m_bRadius;


    switch (m_cornerStyle) {
    case CornerStyle::TopOnly:
        path.moveTo(r.bottomLeft());
        path.lineTo(r.topLeft() + QPoint(0, rSize));
        path.arcTo(QRect(r.topLeft(), QSize(2 * rSize, 2 * rSize)), 180, -90);
        path.lineTo(r.topRight() - QPoint(rSize, 0));
        path.arcTo(QRect(r.topRight() - QPoint(2 * rSize, 0), QSize(2 * rSize, 2 * rSize)), 90, -90);
        path.lineTo(r.bottomRight());
        path.lineTo(r.bottomLeft());
        break;

    case CornerStyle::BottomOnly:
        path.moveTo(r.topLeft());
        path.lineTo(r.bottomLeft() - QPoint(0, rSize));
        path.arcTo(QRect(r.bottomLeft() - QPoint(0, 2 * rSize), QSize(2 * rSize, 2 * rSize)), 180, 90);
        path.lineTo(r.bottomRight() - QPoint(rSize, 0));
        path.arcTo(QRect(r.bottomRight() - QPoint(2 * rSize, 2 * rSize), QSize(2 * rSize, 2 * rSize)), 270, 90);
        path.lineTo(r.topRight());
        path.lineTo(r.topLeft());
        break;

    case CornerStyle::LeftOnly:
        path.moveTo(r.topRight());
        path.lineTo(r.topLeft() + QPoint(rSize, 0));
        path.arcTo(QRect(r.topLeft(), QSize(2 * rSize, 2 * rSize)), 90, 90);
        path.lineTo(r.bottomLeft() + QPoint(0, -rSize));
        path.arcTo(QRect(r.bottomLeft() - QPoint(0, 2 * rSize), QSize(2 * rSize, 2 * rSize)), 180, 90);
        path.lineTo(r.bottomRight());
        path.lineTo(r.topRight());
        break;

    case CornerStyle::RightOnly:
        path.moveTo(r.topLeft());
        path.lineTo(r.topRight() - QPoint(rSize, 0));
        path.arcTo(QRect(r.topRight() - QPoint(2 * rSize, 0), QSize(2 * rSize, 2 * rSize)), 90, -90);
        path.lineTo(r.bottomRight() - QPoint(0, rSize));
        path.arcTo(QRect(r.bottomRight() - QPoint(2 * rSize, 2 * rSize), QSize(2 * rSize, 2 * rSize)), 0, -90);
        path.lineTo(r.bottomLeft());
        path.lineTo(r.topLeft());
        break;

    case CornerStyle::BottomLeft:
        // Rounded corner on the bottom-left only
        path.moveTo(r.topRight());
        path.lineTo(r.topLeft());
        path.lineTo(r.bottomLeft() - QPoint(0, rSize));
        path.arcTo(QRect(r.bottomLeft() - QPoint(0, 2 * rSize), QSize(2 * rSize, 2 * rSize)), 180, 90);
        path.lineTo(r.bottomRight());
        path.lineTo(r.topRight());
        break;

    case CornerStyle::BottomRight:
        // Rounded corner on the bottom-right only
        path.moveTo(r.topLeft());
        path.lineTo(r.topRight());
        path.lineTo(r.bottomRight() - QPoint(0, rSize));
        path.arcTo(QRect(path.currentPosition().toPoint() - QPoint(2 * rSize, rSize), QSize(2 * rSize, 2 * rSize)), 0, -90);
        path.lineTo(r.bottomLeft());
        path.lineTo(r.topLeft());
        break;

    case CornerStyle::None:
        path.addRect(r);
        break;

    case CornerStyle::Default:
    default:
        path.addRoundedRect(r, rSize, rSize);
        break;
    }

    painter.setClipPath(path);

    // Draw background if enabled
    if (m_enableBackground && !m_backgroundImage.isNull()) {
        painter.drawPixmap(0, 0, width(), height(), m_backgroundImage);
    }

    // Draw border if enabled
    if (m_enableBorder && m_borderSize > 0) {
        QPen pen(m_borderColor, m_borderSize);
        painter.setPen(pen);
        pen.setJoinStyle(Qt::MiterJoin);
        pen.setCapStyle(Qt::RoundCap);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }
}



void TWidget::setEnableBackground(bool set) {
    if (m_enableBackground != set) {
        m_enableBackground = set;
        update();
    }
}

void TWidget::setBackgroundImage(QString image) {
    QFile image_file(image);
    if (!image_file.exists()) return;
    setBackgroundImage(QPixmap(image));
    repaint();
}

void TWidget::setBackgroundImage(QPixmap path)
{
    m_backgroundImage = QPixmap(path);
    repaint();
}

void TWidget::setBackgroundColor(QColor color)
{
    QSize frameSize = size();

    if (frameSize.isEmpty()) {
        frameSize = QSize(1, 1);
    }

    // Create a QPixmap of the frame's size
    QPixmap colorPixmap(frameSize);
    // Fill the pixmap with the specified color
    colorPixmap.fill(color);

    // Set this newly created pixmap as the background image
    setBackgroundImage(colorPixmap);
}

void TWidget::setBorder(bool b)
{
    m_enableBorder=b;
    repaint();
}


void TWidget::setBorderRadius(short r)
{
    m_bRadius=r;
    repaint();
}

void TWidget::setBorderColor(const QColor &color)
{
    m_borderColor=color;
    repaint();
}

short TWidget::borderRadius()
{
    return m_bRadius;
}

void TWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    emit resizing();
}
