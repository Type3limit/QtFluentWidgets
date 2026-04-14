#pragma once

#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

class QAbstractScrollArea;
class QScrollBar;

namespace Fluent {

struct FluentAnnotatedScrollBarRange {
    int start = 0;
    int end = 0;
    QString text;
};

inline bool operator==(const FluentAnnotatedScrollBarRange &lhs, const FluentAnnotatedScrollBarRange &rhs)
{
    return lhs.start == rhs.start && lhs.end == rhs.end && lhs.text == rhs.text;
}

struct FluentAnnotatedScrollBarSource {
    QString group;
    QString text;
    int start = 0;
    int end = 0;
};

inline bool operator==(const FluentAnnotatedScrollBarSource &lhs, const FluentAnnotatedScrollBarSource &rhs)
{
    return lhs.group == rhs.group && lhs.text == rhs.text && lhs.start == rhs.start && lhs.end == rhs.end;
}

class FluentAnnotatedScrollBar final : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool showToolTipOnScroll READ showToolTipOnScroll WRITE setShowToolTipOnScroll)
    Q_PROPERTY(int toolTipDuration READ toolTipDuration WRITE setToolTipDuration)

public:
    explicit FluentAnnotatedScrollBar(QWidget *parent = nullptr);

    void setScrollArea(QAbstractScrollArea *area);
    QAbstractScrollArea *scrollArea() const;

    void setScrollBar(QScrollBar *scrollBar);
    QScrollBar *scrollBar() const;

    void setAnnotatedRanges(const QVector<FluentAnnotatedScrollBarRange> &ranges);
    QVector<FluentAnnotatedScrollBarRange> annotatedRanges() const;
    void clearAnnotatedRanges();

    void setSources(const QVector<FluentAnnotatedScrollBarSource> &sources);
    QVector<FluentAnnotatedScrollBarSource> sources() const;
    void addSource(const FluentAnnotatedScrollBarSource &source);
    void addSources(const QVector<FluentAnnotatedScrollBarSource> &sources);
    void clearSources();

    QStringList groups() const;

    int currentRangeIndex() const;
    QString currentRangeText() const;
    void setCurrentRangeIndex(int index);

    int currentGroupIndex() const;
    QString currentGroup() const;
    void setCurrentGroup(const QString &group);

    int currentSourceIndex() const;
    FluentAnnotatedScrollBarSource currentSource() const;
    void setCurrentSourceIndex(int index);

    void setShowToolTipOnScroll(bool enabled);
    bool showToolTipOnScroll() const;

    void setToolTipDuration(int msec);
    int toolTipDuration() const;

    QSize sizeHint() const override;

signals:
    void annotatedRangesChanged();
    void sourcesChanged();
    void currentRangeChanged(int index, const QString &text);
    void currentGroupChanged(int index, const QString &group);
    void currentSourceChanged(int index, const QString &group, const QString &text);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QRectF railRect() const;
    QRectF handleRect() const;
    QRectF labelBoundsRect() const;
    QVector<QRectF> layoutLabelRects() const;
    int rangeIndexForViewport(int topValue, int bottomValue) const;
    int rangeIndexForValue(int value) const;
    int sourceIndexForViewport(int topValue, int bottomValue) const;
    int sourceIndexForValue(int value) const;
    int rangeIndexAt(const QPoint &pos) const;
    int valueForPosition(const QPoint &pos) const;
    int valueForHandleTop(qreal handleTop) const;
    int targetValueForRange(int index) const;
    int targetValueForSource(int index) const;
    int groupIndexForSourceIndex(int sourceIndex) const;
    int firstSourceIndexForRange(int rangeIndex) const;
    void reconnectScrollBar(QScrollBar *scrollBar);
    void handleLinkedScrollBarChanged(bool showToolTip);
    void setSourcesInternal(const QVector<FluentAnnotatedScrollBarSource> &sources);
    void rebuildRangesFromSources();
    void applyCurrentIndexes(int rangeIndex, int sourceIndex, bool emitSignals, bool showToolTip);
    QString toolTipTextForCurrentState() const;
    void maybeShowCurrentToolTip();
    void hideToolTip();
    void updateCursorShape();

    bool m_usingSources = false;
    QPointer<QAbstractScrollArea> m_scrollArea;
    QPointer<QScrollBar> m_scrollBar;
    QVector<FluentAnnotatedScrollBarRange> m_ranges;
    QVector<FluentAnnotatedScrollBarSource> m_sources;
    QVector<int> m_groupFirstSource;
    QVector<int> m_groupLastSource;
    int m_hoverRange = -1;
    int m_currentRange = -1;
    int m_currentSource = -1;
    bool m_hoverHandle = false;
    bool m_draggingHandle = false;
    qreal m_handleDragOffset = 0.0;
    bool m_showToolTipOnScroll = true;
    int m_toolTipDuration = 900;
};

} // namespace Fluent