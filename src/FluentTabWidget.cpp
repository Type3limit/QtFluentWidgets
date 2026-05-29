#include "Fluent/FluentTabWidget.h"
#include "Fluent/FluentFramePainter.h"
#include "Fluent/FluentMotion.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QEvent>
#include <QIcon>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
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

static QPainterPath topRoundedRectPath(const QRectF &rect, qreal radius)
{
    const qreal r = qMin(radius, qMin(rect.width(), rect.height()) / 2.0);
    QPainterPath path;
    path.moveTo(rect.left(), rect.bottom());
    path.lineTo(rect.left(), rect.top() + r);
    path.quadTo(rect.left(), rect.top(), rect.left() + r, rect.top());
    path.lineTo(rect.right() - r, rect.top());
    path.quadTo(rect.right(), rect.top(), rect.right(), rect.top() + r);
    path.lineTo(rect.right(), rect.bottom());
    path.closeSubpath();
    return path;
}

static QPainterPath topRoundedStrokePath(const QRectF &rect, qreal radius)
{
    const qreal r = qMin(radius, qMin(rect.width(), rect.height()) / 2.0);
    QPainterPath path;
    path.moveTo(rect.left(), rect.bottom());
    path.lineTo(rect.left(), rect.top() + r);
    path.quadTo(rect.left(), rect.top(), rect.left() + r, rect.top());
    path.lineTo(rect.right() - r, rect.top());
    path.quadTo(rect.right(), rect.top(), rect.right(), rect.top() + r);
    path.lineTo(rect.right(), rect.bottom());
    return path;
}

static QPainterPath bottomRoundedRectPath(const QRectF &rect, qreal radius)
{
    const qreal r = qMin(radius, qMin(rect.width(), rect.height()) / 2.0);
    const qreal left = rect.left();
    const qreal top = rect.top();
    const qreal right = rect.right();
    const qreal bottom = rect.bottom();

    QPainterPath path;
    path.moveTo(left, top);
    path.lineTo(right, top);
    path.lineTo(right, bottom - r);
    path.quadTo(right, bottom, right - r, bottom);
    path.lineTo(left + r, bottom);
    path.quadTo(left, bottom, left, bottom - r);
    path.lineTo(left, top);
    path.closeSubpath();
    return path;
}

static QPainterPath bottomRoundedStrokePathWithTopGap(const QRectF &rect,
                                                      qreal radius,
                                                      qreal gapLeft,
                                                      qreal gapRight)
{
    const qreal r = qMin(radius, qMin(rect.width(), rect.height()) / 2.0);
    const qreal left = rect.left();
    const qreal top = rect.top();
    const qreal right = rect.right();
    const qreal bottom = rect.bottom();
    const qreal gapStart = qBound(left, gapLeft, right);
    const qreal gapEnd = qBound(left, gapRight, right);

    QPainterPath path;
    path.moveTo(left, top);
    if (gapStart > left) {
        path.lineTo(gapStart, top);
    }

    path.moveTo(gapEnd, top);
    if (gapEnd < right) {
        path.lineTo(right, top);
    }
    path.lineTo(right, bottom - r);
    path.quadTo(right, bottom, right - r, bottom);
    path.lineTo(left + r, bottom);
    path.quadTo(left, bottom, left, bottom - r);
    path.lineTo(left, top);
    return path;
}

static QPainterPath bottomRoundedStrokePath(const QRectF &rect, qreal radius)
{
    return bottomRoundedStrokePathWithTopGap(rect, radius, rect.left(), rect.left());
}

static void paintWindowCloseGlyph(QPainter &painter,
                                  const QRectF &buttonRect,
                                  const ThemeColors &colors,
                                  qreal hoverLevel,
                                  qreal pressLevel,
                                  bool neutralClose = false)
{
    QColor fill = Qt::transparent;
    if (!neutralClose) {
        QColor danger = colors.error.isValid() ? colors.error : QColor("#E81123");
        QColor dangerPressed = danger.darker(125);
        danger.setAlphaF(0.92);
        dangerPressed.setAlphaF(0.97);
        fill = Style::mix(fill, danger, hoverLevel);
        fill = Style::mix(fill, dangerPressed, pressLevel);
    } else {
        QColor h = colors.hover;
        h.setAlphaF(0.58);
        QColor pr = colors.pressed;
        pr.setAlphaF(0.78);
        fill = Style::mix(fill, h, hoverLevel);
        fill = Style::mix(fill, pr, pressLevel);
    }

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(fill);
    painter.drawRoundedRect(buttonRect.adjusted(0.5, 0.5, -0.5, -0.5), 4.0, 4.0);

    QColor glyphColor = Style::withAlpha(colors.text, 235);
    if (!neutralClose && (hoverLevel > 0.01 || pressLevel > 0.01)) {
        glyphColor = QColor(255, 255, 255, 245);
    }

    painter.setPen(QPen(glyphColor, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    const QPointF center = buttonRect.center();
    const QRectF glyphBox(center.x() - 8.0, center.y() - 8.0, 16.0, 16.0);
    const QRectF r = glyphBox.adjusted(3.0, 3.0, -3.0, -3.0);
    painter.drawLine(QPointF(r.left(), r.top()), QPointF(r.right(), r.bottom()));
    painter.drawLine(QPointF(r.right(), r.top()), QPointF(r.left(), r.bottom()));
    painter.restore();
}

static void paintDocumentPaneSurface(QPainter &painter,
                                     const QRectF &rect,
                                     const QRectF &selectedTabRect,
                                     const ThemeColors &colors,
                                     qreal radius,
                                     bool enabled)
{
    FluentSurfaceSpec spec;
    spec.level = FluentSurfaceLevel::Card;
    spec.radius = radius;
    spec.drawBorder = true;
    spec.enabled = enabled;

    const QRectF panelRect = rect.adjusted(spec.borderInset, spec.borderInset, -spec.borderInset, -spec.borderInset);
    const QColor fill = fluentSurfaceFill(colors, spec);
    const QColor border = fluentSurfaceBorder(colors, spec);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(fill);
    painter.drawPath(bottomRoundedRectPath(panelRect, spec.radius));

    bool drewBorder = false;
    if (selectedTabRect.isValid() && selectedTabRect.width() > 4.0) {
        const qreal y = panelRect.top();
        const qreal gapLeft = qMax(panelRect.left() + 1.0, selectedTabRect.left() + 1.0);
        const qreal gapRight = qMin(panelRect.right() - 1.0, selectedTabRect.right() - 1.0);
        if (gapRight > gapLeft) {
            // Let the selected tab merge into the pane: the pane top border is
            // deliberately absent under the active tab, like Explorer tabs.
            painter.setPen(Qt::NoPen);
            painter.setBrush(fill);
            painter.drawRect(QRectF(QPointF(gapLeft, y - 1.0), QPointF(gapRight, y + 2.0)));

            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(border, spec.borderWidth));
            painter.drawPath(bottomRoundedStrokePathWithTopGap(panelRect, spec.radius, gapLeft, gapRight));
            drewBorder = true;

            const qreal stitchTop = qMin(selectedTabRect.bottom(), y - 1.0);
            painter.drawLine(QPointF(gapLeft, stitchTop), QPointF(gapLeft, y + 0.5));
            painter.drawLine(QPointF(gapRight, stitchTop), QPointF(gapRight, y + 0.5));
        }
    }

    if (!drewBorder) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(border, spec.borderWidth));
        painter.drawPath(bottomRoundedStrokePath(panelRect, spec.radius));
    }

    painter.restore();
}

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
        if (const auto *tabs = qobject_cast<const FluentTabWidget *>(m_host.data())) {
            if (tabs->tabDisplayMode() == FluentTabWidget::TabDisplayMode::Document) {
                return;
            }
        }

        const auto &colors = ThemeManager::instance().colors();
        const auto tokens = Theme::tokens(colors);

        QPainter painter(this);
        if (!painter.isActive()) {
            return;
        }
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QRectF outer = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        const qreal radius = tokens.radius.card;

        QColor stroke = tokens.neutral.strokeSubtle;
        if (!m_host->isEnabled()) {
            stroke = Style::mix(stroke, colors.disabledText, 0.35);
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
        FluentMotion::configure(m_indicatorAnim, FluentMotionRole::Selection);
        m_indicatorAnim->setStartValue(0.0);
        m_indicatorAnim->setEndValue(1.0);
        connect(m_indicatorAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            const qreal t = value.toReal();
            m_indicatorRect = lerpRect(m_indicatorStartRect, m_indicatorTargetRect, t);
            update();
        });

        connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
            FluentMotion::configure(m_indicatorAnim, FluentMotionRole::Selection);
            FluentMotion::configure(m_hoverAnim, FluentMotionRole::Hover);
        });

        connect(this, &QTabBar::currentChanged, this, [this](int index) {
            animateTo(index);
        });

        m_hoverAnim = new QVariantAnimation(this);
        FluentMotion::configure(m_hoverAnim, FluentMotionRole::Hover);
        connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            m_hoverLevel = value.toReal();
            update();
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
        if (m_indicatorAnim->duration() <= 0) {
            m_indicatorRect = m_indicatorTargetRect;
            update();
            return;
        }
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

    FluentTabWidget::TabDisplayMode displayMode() const
    {
        return m_displayMode;
    }

    void setDisplayMode(FluentTabWidget::TabDisplayMode mode)
    {
        if (m_displayMode == mode) {
            return;
        }
        m_displayMode = mode;
        if (m_displayMode == FluentTabWidget::TabDisplayMode::Document) {
            syncNativeCloseButtons();
        } else if (tabsClosable()) {
            // Switching away from Document gives native QTabBar its close buttons back.
            QTabBar::setTabsClosable(false);
            QTabBar::setTabsClosable(true);
        }
        updateGeometry();
        update();
    }

protected:
    bool event(QEvent *event) override
    {
        // QTabBar updates tab rects lazily; keep underline synced with layout changes.
        if (event->type() == QEvent::LayoutRequest || event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange
            || event->type() == QEvent::ChildAdded) {
            updateLayoutPadding();
            syncNativeCloseButtons();
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
        syncNativeCloseButtons();
        updateScrollButtons();
        syncIndicatorToCurrent();
        update();
    }

    void tabLayoutChange() override
    {
        QTabBar::tabLayoutChange();
        updateLayoutPadding();
        syncNativeCloseButtons();
        syncIndicatorToCurrent();
        update();
    }

    void tabInserted(int index) override
    {
        QTabBar::tabInserted(index);
        syncNativeCloseButtons();
        updateGeometry();
    }

    void tabRemoved(int index) override
    {
        QTabBar::tabRemoved(index);
        m_hoverIndex = -1;
        m_closeHoverIndex = -1;
        m_closePressedIndex = -1;
        syncIndicatorToCurrent();
        updateGeometry();
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
            const bool document = (m_displayMode == FluentTabWidget::TabDisplayMode::Document);
            if (document) {
                const QFontMetrics fm(font());
                int w = 30 + fm.horizontalAdvance(tabText(index));
                if (!tabIcon(index).isNull()) {
                    w += 24;
                }
                if (tabsClosable()) {
                    w += 30;
                }
                sz.setHeight(qMax(sz.height(), 46));
                int maxWidth = 260;
                if (count() > 0 && width() > 0) {
                    int reserved = 0;
                    if (const auto *tabs = qobject_cast<const FluentTabWidget *>(parentWidget())) {
                        if (const QWidget *corner = tabs->cornerWidget(Qt::TopRightCorner)) {
                            const QSize cornerSize = corner->sizeHint().isValid() ? corner->sizeHint() : corner->size();
                            reserved = qMax(cornerSize.width(), corner->width()) + 14;
                        }
                    }
                    const int available = qMax(0, width() - reserved - 6);
                    maxWidth = qBound(112, available / qMax(1, count()), 260);
                }
                sz.setWidth(qBound(112, w, maxWidth));
            } else {
                sz.setHeight(qMax(sz.height(), 36));
                sz.setWidth(qMax(sz.width() + 12, 72));
            }
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
        const auto tokens = Theme::tokens(colors);

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
        const bool horizontalDocument = !vertical && (m_displayMode == FluentTabWidget::TabDisplayMode::Document);
        QColor bgFill = horizontalDocument ? tokens.neutral.background : tokens.neutral.card;
        if (!isEnabled()) {
            bgFill = Style::mix(bgFill, colors.hover, 0.35);
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(bgFill);
        if (horizontalDocument) {
            qreal stripLeft = 0.0;
            if (count() > 0) {
                const QRect firstTab = tabRect(0);
                if (firstTab.isValid()) {
                    stripLeft = firstTab.adjusted(4, 4, -4, 0).left();
                }
            }
            if (stripLeft > 0.0) {
                painter.fillRect(QRectF(0.0, 0.0, stripLeft, height()), tokens.neutral.card);
            }
            const QRectF stripRect(QPointF(stripLeft, 0.0), QPointF(width(), height() + 1.0));
            painter.drawPath(topRoundedRectPath(stripRect.adjusted(0.5, 0.5, -0.5, 0.0), tokens.radius.card));
        } else {
            painter.drawRect(rect());
        }

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
                QColor fill = Style::mix(tokens.neutral.card, colors.accent, tokens.dark ? 0.18 : 0.09);
                if (!isEnabled()) {
                    fill = Style::mix(tokens.neutral.card, colors.hover, 0.35);
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
            const bool hovered = (i == m_hoverIndex && m_hoverLevel > 0.001);
            const bool pressed = (i == m_pressedIndex);
            const bool document = horizontalDocument;

            // For vertical navigation, keep the background nearly full-height to avoid large gaps.
            QRect bg = vertical ? tabR.adjusted(10, 2, -10, -2) : tabR.adjusted(document ? 4 : 4, document ? 4 : 8, document ? -4 : -4, document ? 0 : -8);
            QColor fill = Qt::transparent;
            QColor stroke = Qt::transparent;

            if (selected) {
                if (document) {
                    FluentSurfaceSpec spec;
                    spec.level = FluentSurfaceLevel::Card;
                    spec.drawBorder = true;
                    spec.enabled = isEnabled();
                    fill = fluentSurfaceFill(colors, spec);
                    stroke = fluentSurfaceBorder(colors, spec);
                } else if (!vertical) {
                    fill = Qt::transparent;
                    stroke = Qt::transparent;
                }
            } else if (pressed) {
                fill = document ? Style::mix(tokens.neutral.card, colors.pressed, tokens.dark ? 0.54 : 0.46)
                                : Qt::transparent;
            } else if (hovered) {
                if (document) {
                    // Subtle animated hover: keep RGB stable and fade alpha to avoid dark flashes.
                    fill = Style::mix(tokens.neutral.card, colors.hover, tokens.dark ? 0.46 : 0.36);
                    fill.setAlphaF(qBound<qreal>(0.0, (tokens.dark ? 0.78 : 0.64) * m_hoverLevel, 1.0));
                }
            }

            if (fill.alpha() > 0) {
                painter.setBrush(fill);
                if (document) {
                    QRectF tabFill = QRectF(bg).adjusted(0.5, 0.5, -0.5, selected ? 2.0 : -0.5);
                    painter.setPen(Qt::NoPen);
                    painter.drawPath(topRoundedRectPath(tabFill, 10.0));
                    if (selected && stroke.alpha() > 0) {
                        painter.setPen(QPen(stroke, 1.0));
                        painter.setBrush(Qt::NoBrush);
                        painter.drawPath(topRoundedStrokePath(QRectF(bg).adjusted(0.5, 0.5, -0.5, 1.0), 10.0));
                    }
                } else {
                    painter.setPen(stroke.alpha() > 0 ? QPen(stroke, 1.0) : Qt::NoPen);
                    painter.drawRoundedRect(bg, vertical ? 9 : 6, vertical ? 9 : 6);
                }
            }

            if (document && !selected && !hovered && i < count() - 1) {
                const int next = i + 1;
                const bool nextSelected = next == currentIndex();
                const bool nextHovered = next == m_hoverIndex && m_hoverLevel > 0.001;
                if (!nextSelected && !nextHovered) {
                    const int x = bg.right() + 4;
                    const int y = bg.center().y() - 8;
                    QColor separator = tokens.neutral.strokeSubtle;
                    separator.setAlpha(tokens.dark ? 110 : 95);
                    painter.setPen(QPen(separator, 1.0));
                    painter.drawLine(QPointF(x, y), QPointF(x, y + 16));
                }
            }

            QFont f = font();
            f.setWeight(selected ? QFont::DemiBold : QFont::Normal);
            painter.setFont(f);
            const QColor textColor = isEnabled() ? (selected ? colors.text : colors.subText) : colors.disabledText;
            painter.setPen(textColor);

            QRect textR = vertical ? tabR.adjusted(22, 0, -16, 0) : tabR.adjusted(document ? 18 : 14, 0, document ? (tabsClosable() ? -40 : -18) : -14, 0);
            const QIcon icon = tabIcon(i);
            if (document && !icon.isNull()) {
                const QSize wantedIconSize = iconSize().isValid() ? iconSize() : QSize(16, 16);
                const QSize boundedIconSize(qMin(18, wantedIconSize.width()), qMin(18, wantedIconSize.height()));
                QRect iconRect(QPoint(textR.left(), textR.center().y() - boundedIconSize.height() / 2), boundedIconSize);
                icon.paint(&painter,
                           iconRect,
                           Qt::AlignCenter,
                           isEnabled() ? QIcon::Normal : QIcon::Disabled,
                           selected ? QIcon::On : QIcon::Off);
                textR.setLeft(iconRect.right() + 8);
            }
            const QString text = fontMetrics().elidedText(tabText(i), Qt::ElideRight, textR.width());
            const Qt::Alignment align = vertical || document ? (Qt::AlignVCenter | Qt::AlignLeft) : Qt::AlignCenter;
            painter.drawText(textR, align, text);

            if (document && tabsClosable()) {
                const QRect closeR = closeButtonRect(i);
                const bool closeHovered = (i == m_closeHoverIndex);
                const bool closePressed = (i == m_closePressedIndex);
                paintWindowCloseGlyph(painter,
                                      QRectF(closeR),
                                      colors,
                                      closeHovered ? 1.0 : 0.0,
                                      closePressed ? 1.0 : 0.0,
                                      true);
            }
        }

        // Indicator
        if (count() > 0 && currentIndex() >= 0 && m_indicatorRect.isValid() && (m_indicatorRect.width() > 0.0 || m_indicatorRect.height() > 0.0)) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(colors.accent);

            if (!vertical && m_displayMode == FluentTabWidget::TabDisplayMode::Underline) {
                // Fluent underline
                const int indicatorHeight = 3;
                const int y = height() - indicatorHeight - 2;
                const int x = static_cast<int>(m_indicatorRect.x());
                const int w = static_cast<int>(m_indicatorRect.width());
                const int inset = 16;
                painter.drawRoundedRect(QRect(x + inset, y, qMax(0, w - inset * 2), indicatorHeight), 2, 2);
            } else if (vertical) {
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
        syncNativeCloseButtons();
        updateScrollButtons();
        syncIndicatorToCurrent();
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        const int idx = tabAt(event->pos());
        const int closeIdx = documentCloseIndexAt(event->pos());
        if (closeIdx != m_closeHoverIndex) {
            m_closeHoverIndex = closeIdx;
            update();
        }
        if (idx >= 0 && (idx != m_hoverIndex || m_hoverLevel < 0.01)) {
            m_hoverIndex = idx;
            startHoverAnimation(1.0);
        } else if (idx < 0 && m_hoverLevel > 0.0) {
            startHoverAnimation(0.0);
        }
        QTabBar::mouseMoveEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        m_closeHoverIndex = -1;
        m_closePressedIndex = -1;
        startHoverAnimation(0.0);
        update();
        QTabBar::leaveEvent(event);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            const int closeIdx = documentCloseIndexAt(event->pos());
            if (closeIdx >= 0) {
                m_closePressedIndex = closeIdx;
                m_pressedIndex = -1;
                event->accept();
                update();
                return;
            }
        }

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
        if (event->button() == Qt::LeftButton && m_closePressedIndex >= 0) {
            const int pressed = m_closePressedIndex;
            const int releaseIdx = documentCloseIndexAt(event->pos());
            m_closePressedIndex = -1;
            m_pressedIndex = -1;
            if (pressed == releaseIdx) {
                event->accept();
                update();
                emit tabCloseRequested(pressed);
                return;
            }
        }

        m_pressedIndex = -1;
        update();
        QTabBar::mouseReleaseEvent(event);
    }



private:
    bool isHorizontalDocumentMode() const
    {
        const auto sh = shape();
        const bool vertical = (sh == QTabBar::RoundedWest) || (sh == QTabBar::RoundedEast) || (sh == QTabBar::TriangularWest)
            || (sh == QTabBar::TriangularEast);
        return !vertical && m_displayMode == FluentTabWidget::TabDisplayMode::Document;
    }

    QRect closeButtonRect(int index) const
    {
        if (!isHorizontalDocumentMode() || index < 0 || index >= count() || !tabsClosable()) {
            return QRect();
        }

        const QRect tabR = tabRect(index);
        if (!tabR.isValid()) {
            return QRect();
        }

        const QRect bg = tabR.adjusted(4, 4, -4, 0);
        constexpr int closeSize = 24;
        return QRect(bg.right() - closeSize - 6,
                     bg.center().y() - closeSize / 2,
                     closeSize,
                     closeSize);
    }

    int documentCloseIndexAt(const QPoint &pos) const
    {
        if (!isHorizontalDocumentMode() || !tabsClosable()) {
            return -1;
        }

        const int idx = tabAt(pos);
        if (idx < 0) {
            return -1;
        }
        return closeButtonRect(idx).contains(pos) ? idx : -1;
    }

    void syncNativeCloseButtons()
    {
        if (m_syncingNativeCloseButtons || !isHorizontalDocumentMode() || !tabsClosable()) {
            return;
        }

        m_syncingNativeCloseButtons = true;
        for (int i = 0; i < count(); ++i) {
            if (tabButton(i, QTabBar::LeftSide)) {
                setTabButton(i, QTabBar::LeftSide, nullptr);
            }
            if (tabButton(i, QTabBar::RightSide)) {
                setTabButton(i, QTabBar::RightSide, nullptr);
            }
        }
        m_syncingNativeCloseButtons = false;
    }

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

    void startHoverAnimation(qreal endValue)
    {
        if (!m_hoverAnim) {
            return;
        }

        m_hoverAnim->stop();
        if (m_hoverAnim->duration() <= 0) {
            m_hoverLevel = qBound<qreal>(0.0, endValue, 1.0);
            update();
            return;
        }
        m_hoverAnim->setStartValue(m_hoverLevel);
        m_hoverAnim->setEndValue(qBound<qreal>(0.0, endValue, 1.0));
        m_hoverAnim->start();
    }

    QRectF m_indicatorRect;
    QRectF m_indicatorStartRect;
    QRectF m_indicatorTargetRect;
    QVariantAnimation *m_indicatorAnim = nullptr;
    QVariantAnimation *m_hoverAnim = nullptr;

    int m_hoverIndex = -1;
    int m_pressedIndex = -1;
    int m_closeHoverIndex = -1;
    int m_closePressedIndex = -1;
    qreal m_hoverLevel = 0.0;

    bool m_lastVertical = false;
    bool m_syncingNativeCloseButtons = false;
    FluentTabWidget::TabDisplayMode m_displayMode = FluentTabWidget::TabDisplayMode::Underline;
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

bool FluentTabWidget::event(QEvent *event)
{
    const bool handled = QTabWidget::event(event);
    if (event->type() == QEvent::LayoutRequest || event->type() == QEvent::Show || event->type() == QEvent::Polish
        || event->type() == QEvent::ChildAdded) {
        scheduleDocumentCornerWidgetPosition();
    }
    return handled;
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
    scheduleDocumentCornerWidgetPosition();
}

void FluentTabWidget::resizeEvent(QResizeEvent *event)
{
    QTabWidget::resizeEvent(event);
    if (m_frameOverlay) {
        m_frameOverlay->setGeometry(rect());
        m_frameOverlay->raise();
    }
    scheduleDocumentCornerWidgetPosition();
}

void FluentTabWidget::applyTheme()
{
    const QString next = Theme::tabWidgetStyle(ThemeManager::instance().colors());
    if (styleSheet() != next) {
        setStyleSheet(next);
    }
}

void FluentTabWidget::applyContentMargins()
{
    for (int i = 0; i < count(); ++i) {
        applyContentMarginToPage(i);
    }
}

void FluentTabWidget::applyContentMarginToPage(int index)
{
    QWidget *page = widget(index);
    if (!page) {
        return;
    }

    const QMargins margins(m_contentMargin, m_contentMargin, m_contentMargin, m_contentMargin);
    if (page->contentsMargins() != margins) {
        page->setContentsMargins(margins);
    }
}

void FluentTabWidget::tabInserted(int index)
{
    QTabWidget::tabInserted(index);
    applyContentMarginToPage(index);
    scheduleDocumentCornerWidgetPosition();
}

void FluentTabWidget::tabRemoved(int index)
{
    QTabWidget::tabRemoved(index);
    scheduleDocumentCornerWidgetPosition();
}

void FluentTabWidget::scheduleDocumentCornerWidgetPosition()
{
    if (m_documentCornerPositionPending) {
        return;
    }

    m_documentCornerPositionPending = true;
    QTimer::singleShot(0, this, [this]() {
        m_documentCornerPositionPending = false;
        positionDocumentCornerWidget();
    });
}

void FluentTabWidget::positionDocumentCornerWidget()
{
    QWidget *corner = cornerWidget(Qt::TopRightCorner);
    if (!corner || !tabBar()) {
        return;
    }

    const bool documentMode = tabDisplayMode() == TabDisplayMode::Document && tabPosition() == QTabWidget::North;
    if (!documentMode) {
        corner->show();
        return;
    }

    const QRect bar = tabBar()->geometry();
    if (!bar.isValid()) {
        return;
    }

    const QSize s = corner->sizeHint().isValid() ? corner->sizeHint() : corner->size();
    if (corner->size().isEmpty()) {
        corner->resize(s);
    }

    const int gap = 8;
    int x = bar.left() + gap;
    if (count() > 0) {
        const QRect last = tabBar()->tabRect(count() - 1);
        if (last.isValid()) {
            x = bar.left() + last.right() + 1 + gap;
        }
    }

    const int maxX = bar.right() - corner->width() - 4;
    x = qBound(bar.left() + gap, x, qMax(bar.left() + gap, maxX));
    const int y = bar.top() + qMax(0, (bar.height() - corner->height()) / 2);

    corner->setGeometry(x, y, corner->width(), corner->height());
    corner->raise();
    corner->show();
    if (m_frameOverlay) {
        m_frameOverlay->raise();
    }
}

void FluentTabWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    const auto &colors = ThemeManager::instance().colors();
    const auto tokens = Theme::tokens(colors);

    QStyleOptionTabWidgetFrame opt;
    initStyleOption(&opt);
    const QRect pane = style()->subElementRect(QStyle::SE_TabWidgetTabPane, &opt, this);
    const QRect header = tabBar() ? tabBar()->geometry() : QRect();
    const QRect container = header.isValid() ? pane.united(header) : pane;
    const bool documentMode = tabDisplayMode() == TabDisplayMode::Document && tabPosition() == QTabWidget::North;

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (documentMode && header.isValid()) {
        qreal documentLeft = rect().left();
        if (tabBar() && tabBar()->count() > 0) {
            const QRect firstTab = tabBar()->tabRect(0);
            if (firstTab.isValid()) {
                documentLeft = firstTab.translated(header.topLeft()).adjusted(4, 4, -4, 0).left();
            }
        }

        const QColor stripFill = tokens.neutral.background;
        const QRectF stripRect(QPointF(documentLeft, header.top()),
                               QPointF(rect().right(), header.bottom() + 1));
        painter.setPen(Qt::NoPen);
        painter.setBrush(isEnabled() ? stripFill : Style::mix(stripFill, colors.hover, 0.35));
        painter.drawPath(topRoundedRectPath(stripRect.adjusted(0.5, 0.5, -0.5, 0.0), tokens.radius.card));

        QRect documentPane = pane;
        documentPane.setTop(header.bottom() - 1);
        documentPane.setLeft(qRound(documentLeft));
        documentPane.setRight(rect().right());
        documentPane = documentPane.intersected(rect());

        QRectF selectedTabRect;
        if (tabBar() && tabBar()->currentIndex() >= 0) {
            const QRect selectedTab = tabBar()->tabRect(tabBar()->currentIndex());
            if (selectedTab.isValid()) {
                selectedTabRect = QRectF(selectedTab.translated(header.topLeft()).adjusted(4, 4, -4, 0));
            }
        }

        paintDocumentPaneSurface(painter,
                                 QRectF(documentPane),
                                 selectedTabRect,
                                 colors,
                                 tokens.radius.card,
                                 isEnabled());
        return;
    }

    const QRectF r = QRectF(container).adjusted(0.5, 0.5, -0.5, -0.5);
    FluentSurfaceSpec surface;
    surface.level = FluentSurfaceLevel::Card;
    surface.radius = tokens.radius.card;
    surface.drawBorder = documentMode;
    surface.enabled = isEnabled();
    paintFluentSurface(painter, r, colors, surface);
}

FluentTabWidget::TabDisplayMode FluentTabWidget::tabDisplayMode() const
{
    if (auto *bar = dynamic_cast<FluentTabBar *>(tabBar())) {
        return bar->displayMode();
    }
    return TabDisplayMode::Underline;
}

void FluentTabWidget::setTabDisplayMode(TabDisplayMode mode)
{
    if (auto *bar = dynamic_cast<FluentTabBar *>(tabBar())) {
        bar->setDisplayMode(mode);
    }
    if (m_frameOverlay) {
        m_frameOverlay->setVisible(mode != TabDisplayMode::Document);
        m_frameOverlay->update();
    }
    scheduleDocumentCornerWidgetPosition();
}

int FluentTabWidget::contentMargin() const
{
    return m_contentMargin;
}

void FluentTabWidget::setContentMargin(int margin)
{
    const int next = qMax(0, margin);
    if (m_contentMargin == next) {
        return;
    }

    m_contentMargin = next;
    applyContentMargins();
    updateGeometry();
    update();
}

} // namespace Fluent
