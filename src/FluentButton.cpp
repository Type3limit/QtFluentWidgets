#include "Fluent/FluentButton.h"
#include "Fluent/FluentMotion.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "FluentButtonVisuals_p.h"

#include <QAbstractAnimation>
#include <QEvent>
#include <QFontMetrics>
#include <QIcon>
#include <QPainter>
#include <QStyleOptionButton>
#include <QVariantAnimation>

namespace Fluent {

namespace {

QSize logicalPixmapSize(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {
        return {};
    }

    const qreal dpr = pixmap.devicePixelRatio();
    return QSize(qRound(pixmap.width() / dpr), qRound(pixmap.height() / dpr));
}

QSize requestedIconSize(const QSize &iconSize, const ControlMetrics &metrics)
{
    return iconSize.isValid() ? iconSize : QSize(metrics.height - 12, metrics.height - 12);
}

bool isVerticalIconPosition(FluentButton::IconPosition position)
{
    return position == FluentButton::IconPosition::Top
        || position == FluentButton::IconPosition::Bottom;
}

int effectiveIconSpacing(FluentButton::IconPosition position, int spacing)
{
    spacing = qMax(0, spacing);
    if (isVerticalIconPosition(position)) {
        return qMin(spacing, 3);
    }
    return spacing;
}

int effectiveVerticalPadding(const ControlMetrics &metrics,
                             FluentButton::IconPosition position,
                             bool hasIcon,
                             bool hasText)
{
    if (hasIcon && hasText && isVerticalIconPosition(position)) {
        return qMin(metrics.paddingY, 2);
    }
    return metrics.paddingY;
}

QSize applyButtonShapeToSize(QSize size, FluentButton::Shape shape)
{
    if (shape != FluentButton::Shape::Circular) {
        return size;
    }

    const int extent = qMax(size.width(), size.height());
    return QSize(extent, extent);
}

} // namespace

FluentButton::FluentButton(QWidget *parent)
    : QPushButton(parent)
{
    initialize();
}

FluentButton::FluentButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
{
    initialize();
}

FluentButton::FluentButton(const QIcon &icon, const QString &text, QWidget *parent)
    : QPushButton(icon, text, parent)
{
    initialize();
}

void FluentButton::initialize()
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setAutoDefault(false);
    setMinimumHeight(Style::metrics().height);

    m_hoverAnim = new QVariantAnimation(this);
    FluentMotion::configure(m_hoverAnim, FluentMotionRole::Hover);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverLevel = value.toReal();
        update();
    });

    m_pressAnim = new QVariantAnimation(this);
    FluentMotion::configure(m_pressAnim, FluentMotionRole::Press);
    connect(m_pressAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_pressLevel = value.toReal();
        update();
    });

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentButton::applyTheme);
    connect(this, &QAbstractButton::toggled, this, QOverload<>::of(&FluentButton::update));
}

bool FluentButton::isPrimary() const
{
    return m_primary;
}

void FluentButton::setPrimary(bool primary)
{
    if (m_primary == primary) {
        return;
    }

    m_primary = primary;
    applyTheme();
}

FluentButton::IconPosition FluentButton::iconPosition() const
{
    return m_iconPosition;
}

void FluentButton::setIconPosition(IconPosition position)
{
    if (m_iconPosition == position) {
        return;
    }

    m_iconPosition = position;
    updateGeometry();
    update();
}

int FluentButton::iconSpacing() const
{
    return m_iconSpacing;
}

void FluentButton::setIconSpacing(int spacing)
{
    spacing = qMax(0, spacing);
    if (m_iconSpacing == spacing) {
        return;
    }

    m_iconSpacing = spacing;
    updateGeometry();
    update();
}

FluentButton::Shape FluentButton::shape() const
{
    return m_shape;
}

void FluentButton::setShape(Shape shape)
{
    if (m_shape == shape) {
        return;
    }

    m_shape = shape;
    updateGeometry();
    update();
}

qreal FluentButton::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentButton::setHoverLevel(qreal value)
{
    m_hoverLevel = qBound(0.0, value, 1.0);
    update();
}

qreal FluentButton::pressLevel() const
{
    return m_pressLevel;
}

void FluentButton::setPressLevel(qreal value)
{
    m_pressLevel = qBound(0.0, value, 1.0);
    update();
}

QSize FluentButton::sizeHint() const
{
    const auto metrics = Style::metrics();
    const bool hasButtonIcon = !icon().isNull();
    const bool hasText = !text().isEmpty();

    if (!hasButtonIcon && !hasText) {
        QSize size = QPushButton::sizeHint();
        size.rheight() = qMax(size.height(), metrics.height);
        return applyButtonShapeToSize(size, m_shape);
    }

    const QFontMetrics fm(font());
    const QSize iconExtent = hasButtonIcon ? requestedIconSize(iconSize(), metrics) : QSize();
    const QSize textExtent(hasText ? fm.horizontalAdvance(text()) : 0,
                           hasText ? fm.height() : 0);
    const int gap = hasButtonIcon && hasText ? effectiveIconSpacing(m_iconPosition, m_iconSpacing) : 0;
    const int paddingY = effectiveVerticalPadding(metrics, m_iconPosition, hasButtonIcon, hasText);

    QSize content;
    if (hasButtonIcon && hasText && isVerticalIconPosition(m_iconPosition)) {
        content = QSize(qMax(iconExtent.width(), textExtent.width()),
                        iconExtent.height() + gap + textExtent.height());
    } else {
        content = QSize(iconExtent.width() + gap + textExtent.width(),
                        qMax(iconExtent.height(), textExtent.height()));
    }

    QSize size(content.width() + metrics.paddingX * 2,
               qMax(metrics.height, content.height() + paddingY * 2));
    size = size.expandedTo(QSize(metrics.height, metrics.height));
    return applyButtonShapeToSize(size.expandedTo(QPushButton::sizeHint()), m_shape);
}

QSize FluentButton::minimumSizeHint() const
{
    return sizeHint();
}

void FluentButton::changeEvent(QEvent *event)
{
    QPushButton::changeEvent(event);

    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentButton::applyTheme()
{
    const bool hoverRunning = m_hoverAnim && m_hoverAnim->state() == QAbstractAnimation::Running;
    const bool pressRunning = m_pressAnim && m_pressAnim->state() == QAbstractAnimation::Running;
    const QVariant hoverEnd = m_hoverAnim ? m_hoverAnim->endValue() : QVariant();
    const QVariant pressEnd = m_pressAnim ? m_pressAnim->endValue() : QVariant();

    FluentMotion::configure(m_hoverAnim, FluentMotionRole::Hover);
    FluentMotion::configure(m_pressAnim, FluentMotionRole::Press);
    if (hoverRunning && m_hoverAnim->duration() <= 0) {
        m_hoverAnim->stop();
        m_hoverLevel = qBound<qreal>(0.0, hoverEnd.toReal(), 1.0);
    }
    if (pressRunning && m_pressAnim->duration() <= 0) {
        m_pressAnim->stop();
        m_pressLevel = qBound<qreal>(0.0, pressEnd.toReal(), 1.0);
    }
    update();
}

void FluentButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const auto &colors = ThemeManager::instance().colors();
    const auto &tokens = ThemeManager::instance().tokens();

    const bool checked = isCheckable() && isChecked();
    const ButtonVisuals::StateColors state =
        ButtonVisuals::resolve(colors, tokens, m_primary, checked, isEnabled());
    const QColor fill = ButtonVisuals::fillForState(state, m_hoverLevel, m_pressLevel);
    const QColor textColor = ButtonVisuals::textForState(state.text, m_pressLevel, isEnabled());

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    
    const auto m = Style::metrics();
    QRectF rect = QRectF(this->rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal radius = controlRadiusForRect(rect);

    const QColor bottomBorder = m_shape == Shape::Rounded ? state.bottomBorder : QColor(0, 0, 0, 0);
    ButtonVisuals::paintRoundedControl(painter, rect, radius, fill, state.border, bottomBorder);

    // Fluent-like checked detail (without indicator bar): add a subtle inner highlight so
    // the selected state is still obvious on accent-filled primary buttons.
    if (checked && m_primary && isEnabled()) {
        QColor inner = tokens.onAccent;
        inner.setAlpha(115);
        painter.setPen(QPen(inner, 1.0));
        painter.setBrush(Qt::NoBrush);
        const qreal innerRadius = qMax<qreal>(0.0, radius - 1.0);
        painter.drawRoundedRect(rect.adjusted(1.0, 1.0, -1.0, -1.0), innerRadius, innerRadius);
    }

    if (hasFocus() && isEnabled()) {
        QColor focus = tokens.accent.base;
        focus.setAlpha(230);
        painter.setPen(QPen(focus, 2.0));
        painter.setBrush(Qt::NoBrush);
        const qreal focusRadius = qMax<qreal>(0.0, radius - 1.0);
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), focusRadius, focusRadius);
    }

    painter.setPen(textColor);
    painter.setFont(this->font());

    const bool hasButtonIcon = !icon().isNull();
    const bool hasText = !text().isEmpty();
    const int paddingY = effectiveVerticalPadding(m, m_iconPosition, hasButtonIcon, hasText);
    QRect contentRect = rect.toRect().adjusted(m.paddingX, paddingY, -m.paddingX, -paddingY);

    if (!icon().isNull()) {
        const QIcon::Mode mode = isEnabled() ? QIcon::Normal : QIcon::Disabled;
        const QIcon::State state = (isCheckable() && isChecked()) ? QIcon::On : QIcon::Off;
        const QSize requestedSize = requestedIconSize(iconSize(), m);
        const QSize actualSize = icon().actualSize(requestedSize, mode, state);
        const QPixmap pixmap = icon().pixmap(actualSize, mode, state);
        const QSize drawSize = logicalPixmapSize(pixmap);

        if (pixmap.isNull() || !drawSize.isValid()) {
            painter.drawText(contentRect, Qt::AlignCenter | Qt::AlignVCenter | Qt::TextShowMnemonic, text());
            return;
        }

        if (text().isEmpty()) {
            const QRect iconRect(contentRect.center().x() - drawSize.width() / 2,
                                 contentRect.center().y() - drawSize.height() / 2,
                                 drawSize.width(),
                                 drawSize.height());
            painter.drawPixmap(iconRect, pixmap);
            return;
        }

        const int textWidth = fontMetrics().horizontalAdvance(text());
        const int textHeight = fontMetrics().height();

        if (isVerticalIconPosition(m_iconPosition)) {
            const int availableGap = contentRect.height() - drawSize.height() - textHeight;
            const int gap = qBound(0, effectiveIconSpacing(m_iconPosition, m_iconSpacing), availableGap);
            const int totalHeight = drawSize.height() + gap + textHeight;
            const int startY = contentRect.center().y() - totalHeight / 2;
            const bool iconFirst = m_iconPosition == IconPosition::Top;
            const int iconY = iconFirst ? startY : startY + textHeight + gap;
            const int textY = iconFirst ? startY + drawSize.height() + gap : startY;
            const QRect iconRect(contentRect.center().x() - drawSize.width() / 2,
                                 iconY,
                                 drawSize.width(),
                                 drawSize.height());
            const QRect textRect(contentRect.left(), textY, contentRect.width(), textHeight);
            painter.drawPixmap(iconRect, pixmap);
            painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextShowMnemonic, text());
            return;
        }

        const int gap = effectiveIconSpacing(m_iconPosition, m_iconSpacing);
        const int totalWidth = drawSize.width() + gap + textWidth;
        const int startX = contentRect.center().x() - totalWidth / 2;
        const bool iconFirst = m_iconPosition == IconPosition::Left;
        const int iconX = iconFirst ? startX : startX + textWidth + gap;
        const int textX = iconFirst ? startX + drawSize.width() + gap : startX;

        const QRect iconRect(iconX,
                             contentRect.center().y() - drawSize.height() / 2,
                             drawSize.width(),
                             drawSize.height());
        const QRect textRect(textX,
                             contentRect.top(),
                             textWidth,
                             contentRect.height());
        painter.drawPixmap(iconRect, pixmap);
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic, text());
    } else {
        painter.drawText(contentRect, Qt::AlignCenter | Qt::AlignVCenter | Qt::TextShowMnemonic, text());
    }
}

void FluentButton::enterEvent(FluentEnterEvent *event)
{
    QPushButton::enterEvent(event);
    startHoverAnimation(1.0);
}

void FluentButton::leaveEvent(QEvent *event)
{
    QPushButton::leaveEvent(event);
    startHoverAnimation(0.0);
}

void FluentButton::mousePressEvent(QMouseEvent *event)
{
    startPressAnimation(1.0);
    QPushButton::mousePressEvent(event);
}

void FluentButton::mouseReleaseEvent(QMouseEvent *event)
{
    startPressAnimation(0.0);
    QPushButton::mouseReleaseEvent(event);
}

qreal FluentButton::controlRadiusForRect(const QRectF &rect) const
{
    if (m_shape == Shape::Pill || m_shape == Shape::Circular) {
        return qMin(rect.width(), rect.height()) / 2.0;
    }

    return Style::metrics().radius;
}

void FluentButton::startHoverAnimation(qreal endValue)
{
    FluentMotion::configure(m_hoverAnim, FluentMotionRole::Hover);
    m_hoverAnim->stop();
    if (m_hoverAnim->duration() <= 0) {
        setHoverLevel(endValue);
        return;
    }
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(endValue);
    m_hoverAnim->start();
}

void FluentButton::startPressAnimation(qreal endValue)
{
    FluentMotion::configure(m_pressAnim, FluentMotionRole::Press);
    m_pressAnim->stop();
    if (m_pressAnim->duration() <= 0) {
        setPressLevel(endValue);
        return;
    }
    m_pressAnim->setStartValue(m_pressLevel);
    m_pressAnim->setEndValue(endValue);
    m_pressAnim->start();
}

} // namespace Fluent
