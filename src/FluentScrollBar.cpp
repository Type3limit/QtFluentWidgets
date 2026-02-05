#include "Fluent/FluentScrollBar.h"

#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QAbstractScrollArea>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QTimer>
#include <QVariantAnimation>

namespace Fluent {

static QColor handleBaseColor(const ThemeColors &colors)
{
    const bool dark = colors.background.lightnessF() < 0.5;
    return dark ? QColor(255, 255, 255, 70) : QColor(0, 0, 0, 70);
}

static QColor handleHoverColor(const ThemeColors &colors)
{
    const bool dark = colors.background.lightnessF() < 0.5;
    return dark ? QColor(255, 255, 255, 110) : QColor(0, 0, 0, 110);
}

static QColor handlePressedColor(const ThemeColors &colors)
{
    const bool dark = colors.background.lightnessF() < 0.5;
    return dark ? QColor(255, 255, 255, 150) : QColor(0, 0, 0, 150);
}

FluentScrollBar::FluentScrollBar(Qt::Orientation orientation, QWidget *parent)
    : QScrollBar(orientation, parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);

    // We paint the thumb ourselves. For child widgets, per-widget translucency is unreliable on
    // some platforms/backing stores (can appear as a black strip), so we avoid it.
    setAutoFillBackground(false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, false);

    // Keep the bar thin; it still reserves layout space.
    const int thickness = 10;
    if (orientation == Qt::Vertical) {
        setFixedWidth(thickness);
    } else {
        setFixedHeight(thickness);
    }

    m_revealAnim = new QVariantAnimation(this);
    m_revealAnim->setDuration(160);
    m_revealAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_revealAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_revealLevel = v.toReal();
        update();
    });

    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(120);
    m_hoverAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_hoverLevel = v.toReal();
        update();
    });

    updateVisibility(false);

    attachToViewportIfPossible();
}

void FluentScrollBar::setForceVisible(bool visible)
{
    if (m_forceVisible == visible) {
        return;
    }
    m_forceVisible = visible;
    updateVisibility(true);
}

bool FluentScrollBar::forceVisible() const
{
    return m_forceVisible;
}

qreal FluentScrollBar::revealLevel() const
{
    return m_revealLevel;
}

void FluentScrollBar::setRevealLevel(qreal v)
{
    m_revealLevel = qBound<qreal>(0.0, v, 1.0);
    update();
}

qreal FluentScrollBar::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentScrollBar::setHoverLevel(qreal v)
{
    m_hoverLevel = qBound<qreal>(0.0, v, 1.0);
    update();
}

void FluentScrollBar::setOverlayMode(bool overlay)
{
    m_overlayMode = overlay;
    updateVisibility(false);
}

bool FluentScrollBar::overlayMode() const
{
    return m_overlayMode;
}

void FluentScrollBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter p(this);
    if (!p.isActive()) {
        return;
    }
    p.setRenderHint(QPainter::Antialiasing, true);

    // If this scrollbar is installed as the real scroll bar of a QAbstractScrollArea, it reserves
    // layout space. Paint that reserved area to match the viewport background.
    if (auto *area = qobject_cast<QAbstractScrollArea *>(parentWidget())) {
        QColor bg;
        if (area->viewport()) {
            bg = area->viewport()->palette().color(area->viewport()->backgroundRole());
        }
        if (!bg.isValid()) {
            bg = ThemeManager::instance().colors().background;
        }
        p.fillRect(rect(), bg);
    }

    // Hide when there's nothing to scroll.
    if (minimum() >= maximum()) {
        return;
    }

    // In overlay mode we fade the handle in/out.
    if (m_overlayMode && m_revealLevel <= 0.01) {
        return;
    }

    QStyleOptionSlider opt;
    initStyleOption(&opt);

    const QRect handle = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, this);
    if (!handle.isValid() || handle.width() <= 0 || handle.height() <= 0) {
        return;
    }

    const auto &colors = ThemeManager::instance().colors();

    QColor base = handleBaseColor(colors);
    QColor hover = handleHoverColor(colors);
    QColor pressed = handlePressedColor(colors);

    QColor c = Style::mix(base, hover, qBound<qreal>(0.0, m_hoverLevel, 1.0));
    if (m_pressed) {
        c = pressed;
    }

    // Apply overlay reveal alpha.
    if (m_overlayMode) {
        c.setAlphaF(c.alphaF() * qBound<qreal>(0.0, m_revealLevel, 1.0));
    }

    // Inset so the pill has breathing room.
    const int inset = 2;
    QRectF r = QRectF(handle).adjusted(inset, inset, -inset, -inset);
    if (r.width() < 2.0 || r.height() < 2.0) {
        r = QRectF(handle);
    }

    const qreal radius = qMin(r.width(), r.height()) / 2.0;

    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawRoundedRect(r, radius, radius);
}

void FluentScrollBar::enterEvent(QEvent *event)
{
    QScrollBar::enterEvent(event);
    startHoverAnimation(1.0);
    if (m_overlayMode) {
        startRevealAnimation(1.0);
    }
}

void FluentScrollBar::leaveEvent(QEvent *event)
{
    QScrollBar::leaveEvent(event);
    startHoverAnimation(0.0);
    updateVisibility(true);
}

void FluentScrollBar::mousePressEvent(QMouseEvent *event)
{
    if (event && event->button() == Qt::LeftButton) {
        m_pressed = true;
        if (m_overlayMode) {
            startRevealAnimation(1.0);
        }
        update();
    }
    QScrollBar::mousePressEvent(event);
}

void FluentScrollBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event && event->button() == Qt::LeftButton) {
        m_pressed = false;
        update();
        updateVisibility(true);
    }
    QScrollBar::mouseReleaseEvent(event);
}

bool FluentScrollBar::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_viewport && m_overlayMode) {
        switch (event->type()) {
        case QEvent::Enter:
        case QEvent::HoverEnter:
        case QEvent::MouseMove:
        case QEvent::Wheel:
            revealForInteraction();
            break;
        case QEvent::Leave:
        case QEvent::HoverLeave:
            if (m_hideTimer) {
                m_hideTimer->start();
            } else {
                updateVisibility(true);
            }
            break;
        default:
            break;
        }
    }

    return QScrollBar::eventFilter(watched, event);
}

void FluentScrollBar::attachToViewportIfPossible()
{
    if (m_viewport) {
        return;
    }

    auto *area = qobject_cast<QAbstractScrollArea *>(parentWidget());
    if (!area || !area->viewport()) {
        return;
    }

    m_viewport = area->viewport();
    m_viewport->installEventFilter(this);

    if (auto *vpWidget = qobject_cast<QWidget *>(m_viewport)) {
        vpWidget->setMouseTracking(true);
    }

    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(700);
    connect(m_hideTimer, &QTimer::timeout, this, [this]() {
        updateVisibility(true);
    });
}

void FluentScrollBar::revealForInteraction()
{
    startRevealAnimation(1.0);
    if (m_hideTimer) {
        m_hideTimer->start();
    }
}

void FluentScrollBar::updateVisibility(bool animated)
{
    if (!m_overlayMode) {
        // Non-overlay: always visible.
        if (animated) {
            startRevealAnimation(1.0);
        } else {
            m_revealLevel = 1.0;
            update();
        }
        return;
    }

    const bool shouldReveal = m_forceVisible || m_pressed || underMouse();
    const qreal target = shouldReveal ? 1.0 : 0.0;
    if (animated) {
        startRevealAnimation(target);
    } else {
        m_revealLevel = target;
        update();
    }
}

void FluentScrollBar::startRevealAnimation(qreal to)
{
    if (!m_revealAnim) {
        m_revealLevel = to;
        update();
        return;
    }
    m_revealAnim->stop();
    m_revealAnim->setStartValue(m_revealLevel);
    m_revealAnim->setEndValue(to);
    m_revealAnim->start();
}

void FluentScrollBar::startHoverAnimation(qreal to)
{
    if (!m_hoverAnim) {
        m_hoverLevel = to;
        update();
        return;
    }
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(to);
    m_hoverAnim->start();
}

} // namespace Fluent
