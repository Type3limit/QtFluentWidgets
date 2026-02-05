#include "Fluent/FluentSplitter.h"

#include "Fluent/FluentTheme.h"

#include <QEvent>
#include <QPainter>
#include <QSplitterHandle>
#include <QVariantAnimation>
#include <qevent.h>

namespace Fluent {

namespace {

class FluentSplitterHandleImpl final : public QSplitterHandle
{
public:
    explicit FluentSplitterHandleImpl(Qt::Orientation orientation, QSplitter *parent)
        : QSplitterHandle(orientation, parent)
    {
        setMouseTracking(true);
        setCursor(orientation == Qt::Horizontal ? Qt::SplitHCursor : Qt::SplitVCursor);

        m_hoverAnim = new QVariantAnimation(this);
        m_hoverAnim->setDuration(120);
        m_hoverAnim->setEasingCurve(QEasingCurve::OutCubic);
        connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
            m_hoverLevel = v.toReal();
            update();
        });
    }

protected:
    void enterEvent(QEvent *event) override
    {
        QSplitterHandle::enterEvent(event);
        startHover(1.0);
    }

    void leaveEvent(QEvent *event) override
    {
        QSplitterHandle::leaveEvent(event);
        startHover(0.0);
    }

    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);

        QPainter p(this);
        if (!p.isActive()) {
            return;
        }

        p.setRenderHint(QPainter::Antialiasing, true);

        const auto &colors = ThemeManager::instance().colors();

        // Handle is kept wide enough to grab, but visuals stay minimal.
        const QRectF r = QRectF(rect());

        const bool splitHorizontal = (orientation() == Qt::Horizontal); // left/right panels => vertical handle

        // Always-on subtle separator line.
        {
            QColor line = colors.border;
            const int baseA = 110;
            const int hoverA = 165;
            line.setAlpha(qBound(0, int(baseA + (hoverA - baseA) * m_hoverLevel), 255));
            p.setPen(QPen(line, 1));
            p.setBrush(Qt::NoBrush);

            if (splitHorizontal) {
                const qreal x = r.center().x();
                p.drawLine(QPointF(x + 0.5, r.top() + 6.0), QPointF(x + 0.5, r.bottom() - 6.0));
            } else {
                const qreal y = r.center().y();
                p.drawLine(QPointF(r.left() + 6.0, y + 0.5), QPointF(r.right() - 6.0, y + 0.5));
            }
        }

        // Hover affordance: small pill + grip dots in the center (Win11-ish).
        if (m_hoverLevel > 0.001) {
            const qreal t = qBound<qreal>(0.0, m_hoverLevel, 1.0);

            QRectF pill;
            if (splitHorizontal) {
                const qreal w = 7.0;
                const qreal h = 44.0;
                pill = QRectF(r.center().x() - w / 2.0, r.center().y() - h / 2.0, w, h);
            } else {
                const qreal w = 44.0;
                const qreal h = 7.0;
                pill = QRectF(r.center().x() - w / 2.0, r.center().y() - h / 2.0, w, h);
            }

            QColor pillFill = colors.hover;
            pillFill.setAlpha(qBound(0, int(std::lround(70.0 * t)), 90));
            p.setPen(Qt::NoPen);
            p.setBrush(pillFill);
            p.drawRoundedRect(pill, 4.0, 4.0);

            QColor dot = colors.subText;
            dot.setAlpha(qBound(0, int(std::lround(150.0 * t)), 170));
            p.setPen(Qt::NoPen);
            p.setBrush(dot);

            const qreal dotR = 1.35;
            const qreal gap = 5.0;
            const qreal cy = r.center().y();
            const qreal cx = r.center().x();

            if (splitHorizontal) {
                p.drawEllipse(QPointF(cx, cy - gap), dotR, dotR);
                p.drawEllipse(QPointF(cx, cy), dotR, dotR);
                p.drawEllipse(QPointF(cx, cy + gap), dotR, dotR);
            } else {
                p.drawEllipse(QPointF(cx - gap, cy), dotR, dotR);
                p.drawEllipse(QPointF(cx, cy), dotR, dotR);
                p.drawEllipse(QPointF(cx + gap, cy), dotR, dotR);
            }
        }
    }

private:
    void startHover(qreal end)
    {
        if (!m_hoverAnim) {
            m_hoverLevel = end;
            update();
            return;
        }

        m_hoverAnim->stop();
        m_hoverAnim->setStartValue(m_hoverLevel);
        m_hoverAnim->setEndValue(end);
        m_hoverAnim->start();
    }

    qreal m_hoverLevel = 0.0;
    QVariantAnimation *m_hoverAnim = nullptr;
};

} // namespace

FluentSplitter::FluentSplitter(Qt::Orientation orientation, QWidget *parent)
    : QSplitter(orientation, parent)
{
    setChildrenCollapsible(false);
    setHandleWidth(8);

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentSplitter::applyTheme);
}

FluentSplitter::FluentSplitter(QWidget *parent)
    : FluentSplitter(Qt::Horizontal, parent)
{
}

QSplitterHandle *FluentSplitter::createHandle()
{
    return new FluentSplitterHandleImpl(orientation(), this);
}

void FluentSplitter::applyTheme()
{
    // Keep splitter background transparent; handle paints itself.
    setStyleSheet(QStringLiteral("QSplitter { background: transparent; }"));
}

} // namespace Fluent
