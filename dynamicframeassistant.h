#ifndef DynamicFrameAssistant_H
#define DynamicFrameAssistant_H

#include <QWidget>
#include <QObject>
#include <QEvent>
#include <QPropertyAnimation>
#include <QHash>
#include <functional>

class LambdaEventFilter : public QObject {
    Q_OBJECT
public:
    using FilterFunc = std::function<bool(QObject*, QEvent*)>;
    explicit LambdaEventFilter(FilterFunc func, QObject* parent = nullptr)
        : QObject(parent), m_func(std::move(func)) {}

protected:
    bool eventFilter(QObject* obj, QEvent* event) override {
        return m_func(obj, event);
    }

private:
    FilterFunc m_func;
};

struct ChildWidgetInfo {
    QWidget* widget;
    Qt::Alignment alignment;
    int margin;
    bool visible;
    QPoint originalPosition;
    QPropertyAnimation* animation = nullptr;
};

class DynamicFrameAssistant : public QObject
{
    Q_OBJECT
public:
    explicit DynamicFrameAssistant(QWidget* parent = nullptr);
    ~DynamicFrameAssistant();

    void addChildWidget(QWidget* widget, Qt::Alignment alignment = Qt::AlignHCenter | Qt::AlignBottom, int margin = 20);
    void removeChildWidget(QWidget* widget);
    void setParentWidget(QWidget* widget);

    void setChildWidgetAlignment(QWidget* widget, Qt::Alignment alignment);
    void setChildWidgetMargin(QWidget* widget, int margin);

    bool childWidgetBlocked() const;
    void setChildWidgetBlock(bool newBlock);

    void popWidget(QWidget* widget, bool show, int duration = 300);
    void popAll(bool show, int duration = 300);

    void showAll() {
        for (auto& info : m_childWidgets) {
            if (info.widget) {
                info.widget->show();
                info.visible = true;
            }
        }
    }

    void setBlock(bool newBlock);
    void update();
    bool isPopped(const QWidget*) const;

signals:
    void animationFinished(QWidget* widget, bool visible);

private:
    QWidget* m_parentWidget = nullptr;
    QHash<QWidget*, ChildWidgetInfo> m_childWidgets;
    LambdaEventFilter* m_eventFilter = nullptr;

    bool m_block = false;
    Qt::Alignment m_defaultAlignment = Qt::AlignHCenter | Qt::AlignBottom;
    int m_defaultMargin = 15;

    void handleResize();
    void updatePositions();
    void updateWidgetPosition(QWidget* widget);
    void setupWidgetAnimation(QWidget* widget);
    QPoint calculateHiddenPosition(QWidget* widget, const Qt::Alignment& alignment);
    QPoint calculateVisiblePosition(QWidget* widget, const Qt::Alignment& alignment, int margin);
};

#endif // DynamicFrameAssistant_H
