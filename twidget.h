#ifndef TWIDGET_H
#define TWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QFile>
#include <QShowEvent>

class TWidget : public QWidget
{
    Q_OBJECT
public:
    enum class CornerStyle {
        Default,    // Rounded on all corners
        TopOnly,    // Rounded only on the top
        BottomOnly, // Rounded only on the bottom
        LeftOnly,   // Rounded only on the left
        RightOnly,  // Rounded only on the right
        BottomLeft, // Rounded only on the bottom left
        BottomRight,// Rounded only on the bottom right
        None        // No rounded corners
    };

    explicit TWidget(QWidget *parent = nullptr);
    ~TWidget();
    void setEnableBackground(bool);
    void setBackgroundImage(QString);
    void setBackgroundImage(QPixmap);
    void setBackgroundColor(QColor);
    void setBorder(bool);
    void setBorderRadius(short);
    void setBorderColor(const QColor& color=QColor(71,158,245));
    short borderRadius();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent* event) override;

signals:
    void clicked(const QPointF& pos);
    void rightClicked(const QPoint& pos);
    void doubleClicked(const QPoint& pos);


    void resizing();

private:
    bool m_enableBackground;
    bool m_enableBorder;
    short m_borderSize=0;
    QPoint m_clickPos;
    short m_bRadius=0;
    QPixmap m_backgroundImage;
    QColor m_borderColor=QColor(71,158,245);
    CornerStyle m_cornerStyle = CornerStyle::Default;

};

#endif // TWIDGET_H
