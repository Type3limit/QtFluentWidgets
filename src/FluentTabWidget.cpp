#include "Fluent/FluentTabWidget.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QVariantAnimation>
#include <QEasingCurve>
#include <QTabBar>
#include <QToolButton>
#include <QTimer>
#include <QPointer>

namespace Fluent {

namespace {

class FluentTabFrameOverlay final : public QWidget
{
public:
    explicit FluentTabFrameOverlay(QWidget *host)
        : QWidget(host)
        , m_host(host)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)
        if (!m_host) {
            return;
        }

        const auto &colors = ThemeManager::instance().colors();

        QPainter painter(this);
        if (!painter.isActive()) {
            return;
        }
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QRectF outer = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        const qreal radius = 10.0;

        // Mask the area outside the rounded rect so child widgets can't visually square-off the corners.
        QPainterPath cut;
        cut.setFillRule(Qt::OddEvenFill);
        cut.addRect(outer);
        cut.addRoundedRect(outer, radius, radius);

        QColor outside = colors.background;
        if (!m_host->isEnabled()) {
            outside = Style::mix(colors.background, colors.hover, 0.35);
        }
        painter.fillPath(cut, outside);

        QColor stroke = colors.border;
        if (!m_host->isEnabled()) {
            stroke = Style::mix(colors.border, colors.disabledText, 0.35);
        }
        painter.setPen(QPen(stroke, 1.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(outer, radius, radius);
    }

private:
    QPointer<QWidget> m_host;
};

class FluentTabBar final : public QTabBar
{
public:
    explicit FluentTabBar(QWidget *parent = nullptr)
        : QTabBar(parent)
    {
        setDocumentMode(true);
        setDrawBase(false);
        setExpanding(false);
        setElideMode(Qt::ElideRight);
        setUsesScrollButtons(true);
        setMouseTracking(true);

        updateLayoutPadding();

        // Scroll buttons may be created lazily; patch them after show/layout.
        QTimer::singleShot(0, this, [this]() { updateScrollButtons(); });
        connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
            updateScrollButtons();
            update();
        });

        m_indicatorAnim = new QVariantAnimation(this);
        m_indicatorAnim->setDuration(240);
        m_indicatorAnim->setEasingCurve(QEasingCurve::InOutCubic);
        m_indicatorAnim->setStartValue(0.0);
        m_indicatorAnim->setEndValue(1.0);
        connect(m_indicatorAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            const qreal t = value.toReal();
            m_indicatorRect = lerpRect(m_indicatorStartRect, m_indicatorTargetRect, t);
            update();
        });

        connect(this, &QTabBar::currentChanged, this, [this](int index) {
            animateTo(index);
        });
    }

    void animateTo(int index)
    {
        const QRect rect = tabRect(index);
        if (!rect.isValid()) {
            // QTabBar can be lazy about layout; retry after the event loop.
            QTimer::singleShot(0, this, [this, index]() { animateTo(index); });
            return;
        }

        // If we don't have a valid current rect yet (startup), snap first.
        if (!m_indicatorRect.isValid()) {
            syncIndicatorToCurrent();
        }

        m_indicatorStartRect = m_indicatorRect;
        m_indicatorTargetRect = QRectF(rect);
        m_indicatorAnim->stop();
        m_indicatorAnim->setStartValue(0.0);
        m_indicatorAnim->setEndValue(1.0);
        m_indicatorAnim->start();
    }

    void syncIndicatorToCurrent()
    {
        const QRect rect = tabRect(currentIndex());
        if (!rect.isValid()) {
            return;
        }
        m_indicatorRect = QRectF(rect);
        m_indicatorStartRect = m_indicatorRect;
        m_indicatorTargetRect = m_indicatorRect;
    }

    static QRectF lerpRect(const QRectF &a, const QRectF &b, qreal t)
    {
        const qreal tt = qBound<qreal>(0.0, t, 1.0);
        return QRectF(
            a.x() + (b.x() - a.x()) * tt,
            a.y() + (b.y() - a.y()) * tt,
            a.width() + (b.width() - a.width()) * tt,
            a.height() + (b.height() - a.height()) * tt);
    }

protected:
    bool event(QEvent *event) override
    {
        // QTabBar updates tab rects lazily; keep underline synced with layout changes.
        if (event->type() == QEvent::LayoutRequest || event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange) {
            updateLayoutPadding();
            // Layout changed; keep cached geometry in sync.
            syncIndicatorToCurrent();
            update();
        }
        return QTabBar::event(event);
    }

    void showEvent(QShowEvent *event) override
    {
        QTabBar::showEvent(event);
        updateLayoutPadding();
        updateScrollButtons();
        syncIndicatorToCurrent();
        update();
    }

    void tabLayoutChange() override
    {
        QTabBar::tabLayoutChange();
        updateLayoutPadding();
        syncIndicatorToCurrent();
        update();
    }

    QSize tabSizeHint(int index) const override
    {
        QSize sz = QTabBar::tabSizeHint(index);
        const auto sh = shape();
        const bool vertical = (sh == QTabBar::RoundedWest) || (sh == QTabBar::RoundedEast) || (sh == QTabBar::TriangularWest)
            || (sh == QTabBar::TriangularEast);
        if (vertical) {
            // Enforce a uniform navigation-item height.
            const int h = qMax(36, fontMetrics().height() + 14);
            sz.setHeight(h);
            sz.setWidth(qMax(sz.width() + 22, 128));
        } else {
            sz.setHeight(qMax(sz.height(), 36));
            sz.setWidth(qMax(sz.width() + 12, 72));
        }
        return sz;
    }

    QSize minimumTabSizeHint(int index) const override
    {
        // Keep layout stable; avoid per-item height variance.
        return tabSizeHint(index);
    }

    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)
        const auto &colors = ThemeManager::instance().colors();

        const auto sh = shape();
        const bool vertical = (sh == QTabBar::RoundedWest) || (sh == QTabBar::RoundedEast) || (sh == QTabBar::TriangularWest)
            || (sh == QTabBar::TriangularEast);

        QPainter painter(this);
        if (!painter.isActive()) {
            return;
        }
        painter.setRenderHint(QPainter::Antialiasing, true);

        // Clear the full area every frame (animated indicator/background moves).
        // We must repaint all pixels; otherwise stale frames can cover text.
        QColor bgFill = colors.surface;
        if (!isEnabled()) {
            bgFill = Style::mix(colors.surface, colors.hover, 0.35);
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(bgFill);
        painter.drawRect(rect());

        // Ensure we have valid cached geometry.
        if (!m_indicatorRect.isValid()) {
            syncIndicatorToCurrent();
        }

        // Selected background (vertical navigation): slide like Win11 Settings.
        // Paint it first so text/hover/pressed states stay crisp.
        if (vertical && count() > 0 && currentIndex() >= 0 && m_indicatorRect.isValid()) {
            const QRect tabR = m_indicatorRect.toRect();
            if (tabR.isValid()) {
                QRect bg = tabR.adjusted(10, 2, -10, -2);
                QColor fill = Style::mix(colors.surface, colors.hover, 0.18);
                if (!isEnabled()) {
                    fill = Style::mix(colors.surface, colors.hover, 0.35);
                }
                painter.setPen(Qt::NoPen);
                painter.setBrush(fill);
                painter.drawRoundedRect(bg, 9, 9);
            }
        }

        // Tabs
        for (int i = 0; i < count(); ++i) {
            const QRect tabR = tabRect(i);
            if (!tabR.isValid()) continue;

            const bool selected = (i == currentIndex());
            const bool hovered = (i == m_hoverIndex);
            const bool pressed = (i == m_pressedIndex);

            // For vertical navigation, keep the background nearly full-height to avoid large gaps.
            QRect bg = vertical ? tabR.adjusted(10, 2, -10, -2) : tabR.adjusted(4, 8, -4, -8);
            QColor fill = Qt::transparent;
            QColor stroke = Qt::transparent;

            if (selected) {
                if (!vertical) {
                    fill = Style::mix(colors.surface, colors.hover, 0.10);
                    stroke = Qt::transparent;
                }
            } else if (pressed) {
                fill = Style::withAlpha(colors.pressed, 190);
            } else if (hovered) {
                // Vertical navigation feels better with a subtle hover background.
                fill = vertical ? Style::mix(colors.surface, colors.hover, 0.10) : Qt::transparent;
            }

            if (fill.alpha() > 0) {
                painter.setPen(stroke.alpha() > 0 ? QPen(stroke, 1.0) : Qt::NoPen);
                painter.setBrush(fill);
                painter.drawRoundedRect(bg, vertical ? 9 : 8, vertical ? 9 : 8);
            }

            QFont f = font();
            f.setWeight(selected ? QFont::DemiBold : QFont::Normal);
            painter.setFont(f);
            painter.setPen(selected ? colors.text : colors.subText);

            QRect textR = vertical ? tabR.adjusted(22, 0, -16, 0) : tabR.adjusted(14, 0, -14, 0);
            const QString text = fontMetrics().elidedText(tabText(i), Qt::ElideRight, textR.width());
            painter.drawText(textR, vertical ? (Qt::AlignVCenter | Qt::AlignLeft) : Qt::AlignCenter, text);
        }

        // Indicator
        if (count() > 0 && currentIndex() >= 0 && m_indicatorRect.isValid() && (m_indicatorRect.width() > 0.0 || m_indicatorRect.height() > 0.0)) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(colors.accent);

            if (!vertical) {
                // Fluent underline
                const int indicatorHeight = 3;
                const int y = height() - indicatorHeight - 2;
                const int x = static_cast<int>(m_indicatorRect.x());
                const int w = static_cast<int>(m_indicatorRect.width());
                const int inset = 16;
                painter.drawRoundedRect(QRect(x + inset, y, qMax(0, w - inset * 2), indicatorHeight), 2, 2);
            } else {
                // Animated navigation indicator for vertical tabs.
                const QRect tabR = m_indicatorRect.toRect();
                QRect bg = tabR.adjusted(10, 2, -10, -2);
                const bool isWest = (sh == QTabBar::RoundedWest) || (sh == QTabBar::TriangularWest);
                const int indicatorWidth = 3;
                const int indicatorPad = 8;
                const int indicatorInset = 6;
                const int x = isWest ? (bg.x() + indicatorInset) : (bg.right() - indicatorWidth - indicatorInset + 1);
                const int y = bg.y() + indicatorPad;
                const int h = qMax(0, bg.height() - indicatorPad * 2);
                painter.drawRoundedRect(QRect(x, y, indicatorWidth, h), 2, 2);
            }
        }
    }

    void resizeEvent(QResizeEvent *event) override
    {
        QTabBar::resizeEvent(event);
        updateLayoutPadding();
        updateScrollButtons();
        syncIndicatorToCurrent();
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        const int idx = tabAt(event->pos());
        if (idx != m_hoverIndex) {
            m_hoverIndex = idx;
            update();
        }
        QTabBar::mouseMoveEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        m_hoverIndex = -1;
        update();
        QTabBar::leaveEvent(event);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        m_pressedIndex = tabAt(event->pos());
        if (event->button() == Qt::LeftButton && m_pressedIndex >= 0) {
            if (m_pressedIndex != currentIndex()) {
                setCurrentIndex(m_pressedIndex);
            }
            event->accept();
            update();
            return;
        }
        update();
        QTabBar::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        m_pressedIndex = -1;
        update();
        QTabBar::mouseReleaseEvent(event);
    }



private:
    void updateLayoutPadding()
    {
        const auto sh = shape();
        const bool vertical = (sh == QTabBar::RoundedWest) || (sh == QTabBar::RoundedEast) || (sh == QTabBar::TriangularWest)
            || (sh == QTabBar::TriangularEast);
        if (vertical == m_lastVertical) {
            return;
        }
        m_lastVertical = vertical;

        // Add breathing room so the first tab isn't glued to the top in navigation mode.
        // Using contents margins keeps hit testing/tab rects consistent.
        if (vertical) {
            setContentsMargins(0, 10, 0, 10);

            // QTabBar's internal layout doesn't always respect contentsMargins for the first tab.
            // Use :first/:last margins to force a visible 10px gap.
            const QString padStyle = QStringLiteral(
                "QTabBar::tab:first { margin-top: 10px; }\n"
                "QTabBar::tab:last  { margin-bottom: 10px; }\n");
            if (styleSheet() != padStyle) {
                setStyleSheet(padStyle);
            }
        } else {
            setContentsMargins(0, 0, 0, 0);

            if (!styleSheet().isEmpty()) {
                setStyleSheet(QString());
            }
        }
    }

    static QIcon chevronIcon(Qt::ArrowType dir, const QColor &color)
    {
        QPixmap pm(16, 16);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);

        const QPointF c(8.0, 8.0);
        const QColor iconColor = Style::withAlpha(color, 0.92);
        if (dir == Qt::LeftArrow) {
            Style::drawChevronLeft(p, c, iconColor, 8.0, 1.8);
        } else if (dir == Qt::RightArrow) {
            Style::drawChevronRight(p, c, iconColor, 8.0, 1.8);
        } else if (dir == Qt::UpArrow) {
            Style::drawChevronUp(p, c, iconColor, 8.0, 1.8);
        } else if (dir == Qt::DownArrow) {
            Style::drawChevronDown(p, c, iconColor, 8.0, 1.8);
        }
        return QIcon(pm);
    }

    void updateScrollButtons()
    {
        const auto buttons = findChildren<QToolButton *>();
        if (buttons.isEmpty())
            return;

        const QColor fg = ThemeManager::instance().colors().text;
        for (QToolButton *b : buttons) {
            if (!b)
                continue;

            const Qt::ArrowType a = b->arrowType();
            if (a != Qt::LeftArrow && a != Qt::RightArrow && a != Qt::UpArrow && a != Qt::DownArrow)
                continue;

            b->setArrowType(Qt::NoArrow);
            b->setIcon(chevronIcon(a, fg));
            b->setIconSize(QSize(16, 16));
            b->setAutoRaise(true);
            b->setFocusPolicy(Qt::NoFocus);
            if (b->width() < 20)
                b->setFixedSize(26, 26);
        }
    }

    QRectF m_indicatorRect;
    QRectF m_indicatorStartRect;
    QRectF m_indicatorTargetRect;
    QVariantAnimation *m_indicatorAnim = nullptr;

    int m_hoverIndex = -1;
    int m_pressedIndex = -1;

    bool m_lastVertical = false;
};

} // namespace

FluentTabWidget::FluentTabWidget(QWidget *parent)
    : QTabWidget(parent)
{
    auto *bar = new FluentTabBar(this);
    setTabBar(bar);

    m_frameOverlay = new FluentTabFrameOverlay(this);
    m_frameOverlay->setGeometry(rect());
    m_frameOverlay->raise();

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentTabWidget::applyTheme);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
        if (m_frameOverlay) {
            m_frameOverlay->update();
        }
    });
}

void FluentTabWidget::changeEvent(QEvent *event)
{
    QTabWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
        if (m_frameOverlay) {
            m_frameOverlay->update();
        }
    }
}

void FluentTabWidget::resizeEvent(QResizeEvent *event)
{
    QTabWidget::resizeEvent(event);
    if (m_frameOverlay) {
        m_frameOverlay->setGeometry(rect());
        m_frameOverlay->raise();
    }
}

void FluentTabWidget::applyTheme()
{
    const QString next = Theme::tabWidgetStyle(ThemeManager::instance().colors());
    if (styleSheet() != next) {
        setStyleSheet(next);
    }
}

void FluentTabWidget::paintEvent(QPaintEvent *event)
{
    const auto &colors = ThemeManager::instance().colors();

    QStyleOptionTabWidgetFrame opt;
    initStyleOption(&opt);
    const QRect pane = style()->subElementRect(QStyle::SE_TabWidgetTabPane, &opt, this);
    const QRect header = tabBar() ? tabBar()->geometry() : QRect();
    const QRect container = header.isValid() ? pane.united(header) : pane;

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = QRectF(container).adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal radius = 10.0;
    QColor fill = colors.surface;
    if (!isEnabled()) {
        fill = Style::mix(colors.surface, colors.hover, 0.35);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(fill);
    painter.drawRoundedRect(r, radius, radius);

    QTabWidget::paintEvent(event);
}

} // namespace Fluent
