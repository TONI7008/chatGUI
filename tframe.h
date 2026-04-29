#ifndef TFRAME_H
#define TFRAME_H

#include <QFrame>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QMouseEvent>

class TFrame : public QFrame
{
    Q_OBJECT
public:
    enum class CornerStyle {
        Default,// Rounded on all corners
        TopOnly,     // Rounded only on the top
        BottomOnly,  // Rounded only on the bottom
        LeftOnly,    // Rounded only on the left
        RightOnly,   // Rounded only on the right
        BottomLeft, // rounded only on the bottom left
        BottomRight,    // rounded only on the bottom right
        None         // No rounded corners
    };

    explicit TFrame(QWidget* parent=nullptr);
    ~TFrame();

    void setBackgroundImage(const QString&);
    void setBackgroundImage(const QPixmap&);
    void setBackgroundColor(const QColor &color);
    void setEnableBackground(bool b);
    void disableBackground();

    void setBorderRadius(short int);
    void setBorder(bool);
    void setFollowBorder(bool);
    void setBorderSize(short size);
    void setBorderColor(const QColor& color=QColor(71,158,245)); // New method to set border color
    QColor borderColor() const; // New method to get border color

    void setCornerStyle(CornerStyle style);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

signals:
    void clicked(const QPoint& pos);
    void rightClicked(const QPoint& pos);
    void doubleClicked(const QPoint& pos);
    void resizing();

private:
    QPixmap m_backgroundImage = QPixmap("/home/hacker/Documents/C++/Connect-4/Images/ima.png");

    short int roundness = 15;
    bool enable_background = true;
    bool enable_border = false;
    qreal m_bratio=0.5;
    bool m_fBorder=false;
    short int borderSize = 0;
    QColor borderColorValue = QColor(71, 158, 245); // Default border color
    CornerStyle cornerStyle = CornerStyle::Default;

};

#endif // TFRAME_H
