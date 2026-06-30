#include "Fluent/FluentToolButton.h"
#include "Fluent/FluentMenu.h"
#include "Fluent/FluentMotion.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "FluentButtonVisuals_p.h"

#include <QAbstractAnimation>
#include <QEvent>
#include <QFontMetrics>
#include <QIcon>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QVariantAnimation>

namespace Fluent {

namespace {

constexpr int kMenuArrowSlotWidth = 22;

QSize applyToolButtonShapeToSize(QSize size, FluentToolButton::Shape shape)
{
    if (shape != FluentToolButton::Shape::Circular) {
        return size;
    }

    const int extent = qMax(size.width(), size.height());
    return QSize(extent, extent);
}

QSize logicalPixmapSize(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {
        return {};
    }

    const qreal dpr = pixmap.devicePixelRatio();
    return QSize(qRound(pixmap.width() / dpr), qRound(pixmap.height() / dpr));
}

QSize requestedToolIconSize(const QToolButton *button)
{
    const auto metrics = Style::metrics();
    return button->iconSize().isValid() ? button->iconSize() : QSize(metrics.height - 12, metrics.height - 12);
}

Qt::ToolButtonStyle effectiveToolButtonStyle(const QToolButton *button, bool hasIcon, bool hasText)
{
    if (!hasIcon && hasText) {
        return Qt::ToolButtonTextOnly;
    }
    if (hasIcon && !hasText) {
        return Qt::ToolButtonIconOnly;
    }

    const Qt::ToolButtonStyle style = button->toolButtonStyle();
    if (style == Qt::ToolButtonTextOnly
        || style == Qt::ToolButtonTextBesideIcon
        || style == Qt::ToolButtonTextUnderIcon) {
        return style;
    }
    return Qt::ToolButtonTextBesideIcon;
}

void drawToolButtonContent(QPainter &painter,
                           QToolButton *button,
                           const QRectF &contentRect,
                           const QColor &textColor)
{
    const bool hasIcon = !button->icon().isNull();
    const bool hasText = !button->text().isEmpty();
    if (!hasIcon && !hasText) {
        return;
    }

    const QFontMetrics fm(button->font());
    const Qt::ToolButtonStyle style = effectiveToolButtonStyle(button, hasIcon, hasText);
    const int gap = hasIcon && hasText ? 8 : 0;

    QPixmap pixmap;
    QSize drawSize;
    if (hasIcon) {
        const QIcon::Mode mode = button->isEnabled() ? QIcon::Normal : QIcon::Disabled;
        const QIcon::State state = button->isChecked() ? QIcon::On : QIcon::Off;
        const QSize requestedSize = requestedToolIconSize(button);
        const QSize actualSize = button->icon().actualSize(requestedSize, mode, state);
        pixmap = button->icon().pixmap(actualSize, mode, state);
        drawSize = logicalPixmapSize(pixmap);
    }

    painter.setPen(textColor);
    painter.setFont(button->font());

    if (style == Qt::ToolButtonIconOnly || !hasText) {
        if (!pixmap.isNull() && drawSize.isValid()) {
            const QRect iconRect(contentRect.center().x() - drawSize.width() / 2.0,
                                 contentRect.center().y() - drawSize.height() / 2.0,
                                 drawSize.width(),
                                 drawSize.height());
            painter.drawPixmap(iconRect, pixmap);
        }
        return;
    }

    if (style == Qt::ToolButtonTextOnly || pixmap.isNull() || !drawSize.isValid()) {
        const QString label = fm.elidedText(button->text(), Qt::ElideRight, qMax(0, qRound(contentRect.width())));
        painter.drawText(contentRect.toRect(), Qt::AlignCenter | Qt::AlignVCenter | Qt::TextShowMnemonic, label);
        return;
    }

    if (style == Qt::ToolButtonTextUnderIcon) {
        const int textHeight = fm.height();
        const int availableTextWidth = qMax(0, qRound(contentRect.width()));
        const QString label = fm.elidedText(button->text(), Qt::ElideRight, availableTextWidth);
        const int textWidth = fm.horizontalAdvance(label);
        const int totalHeight = drawSize.height() + gap + textHeight;
        const int startY = qRound(contentRect.center().y() - totalHeight / 2.0);
        const QRect iconRect(qRound(contentRect.center().x() - drawSize.width() / 2.0),
                             startY,
                             drawSize.width(),
                             drawSize.height());
        const QRect textRect(qRound(contentRect.center().x() - textWidth / 2.0),
                             startY + drawSize.height() + gap,
                             textWidth,
                             textHeight);
        painter.drawPixmap(iconRect, pixmap);
        painter.drawText(textRect, Qt::AlignCenter | Qt::AlignVCenter | Qt::TextShowMnemonic, label);
        return;
    }

    const int availableTextWidth = qMax(0, qRound(contentRect.width()) - drawSize.width() - gap);
    const QString label = fm.elidedText(button->text(), Qt::ElideRight, availableTextWidth);
    const int textWidth = fm.horizontalAdvance(label);
    const int totalWidth = drawSize.width() + gap + textWidth;
    const int startX = qRound(contentRect.center().x() - totalWidth / 2.0);
    const QRect iconRect(startX,
                         qRound(contentRect.center().y() - drawSize.height() / 2.0),
                         drawSize.width(),
                         drawSize.height());
    const QRect textRect(startX + drawSize.width() + gap,
                         qRound(contentRect.top()),
                         textWidth,
                         qRound(contentRect.height()));
    painter.drawPixmap(iconRect, pixmap);
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic, label);
}

bool shouldHandleMenuPress(const QToolButton *button, const QMouseEvent *event)
{
    if (!event || event->button() != Qt::LeftButton || !button->menu() || !button->rect().contains(event->pos())) {
        return false;
    }

    if (button->popupMode() == QToolButton::InstantPopup) {
        return true;
    }

    if (button->popupMode() != QToolButton::MenuButtonPopup) {
        return false;
    }

    const QRect arrowRect(button->width() - kMenuArrowSlotWidth, 0, kMenuArrowSlotWidth, button->height());
    return arrowRect.contains(event->pos());
}

} // namespace

FluentToolButton::FluentToolButton(QWidget *parent)
    : QToolButton(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setAutoRaise(true);
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
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentToolButton::applyTheme);
    connect(this, &QAbstractButton::toggled, this, QOverload<>::of(&FluentToolButton::update));
}

FluentToolButton::FluentToolButton(const QString &text, QWidget *parent)
    : QToolButton(parent)
{
    setText(text);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setAutoRaise(true);
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
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentToolButton::applyTheme);
    connect(this, &QAbstractButton::toggled, this, QOverload<>::of(&FluentToolButton::update));
}

bool FluentToolButton::isPrimary() const
{
    return m_primary;
}

void FluentToolButton::setPrimary(bool primary)
{
    if (m_primary == primary) {
        return;
    }

    m_primary = primary;
    applyTheme();
}

FluentToolButton::Shape FluentToolButton::shape() const
{
    return m_shape;
}

void FluentToolButton::setShape(Shape shape)
{
    if (m_shape == shape) {
        return;
    }

    m_shape = shape;
    updateGeometry();
    update();
}

qreal FluentToolButton::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentToolButton::setHoverLevel(qreal value)
{
    m_hoverLevel = qBound(0.0, value, 1.0);
    update();
}

qreal FluentToolButton::pressLevel() const
{
    return m_pressLevel;
}

void FluentToolButton::setPressLevel(qreal value)
{
    m_pressLevel = qBound(0.0, value, 1.0);
    update();
}

QSize FluentToolButton::sizeHint() const
{
    const auto metrics = Style::metrics();
    const bool hasIcon = !icon().isNull();
    const bool hasText = !text().isEmpty();
    if (!hasIcon && !hasText) {
        const QSize base = QToolButton::sizeHint().expandedTo(QSize(metrics.height, metrics.height));
        return applyToolButtonShapeToSize(base, m_shape);
    }

    const Qt::ToolButtonStyle style = effectiveToolButtonStyle(this, hasIcon, hasText);
    const QSize iconExtent = hasIcon ? requestedToolIconSize(this) : QSize();
    const QFontMetrics fm(font());
    const QSize textExtent(hasText ? fm.horizontalAdvance(text()) : 0,
                           hasText ? fm.height() : 0);
    const int gap = hasIcon && hasText ? 8 : 0;
    const int arrowSlotWidth = menu() && popupMode() != QToolButton::DelayedPopup ? kMenuArrowSlotWidth : 0;

    QSize content;
    if (style == Qt::ToolButtonIconOnly) {
        content = iconExtent;
    } else if (style == Qt::ToolButtonTextOnly) {
        content = textExtent;
    } else if (style == Qt::ToolButtonTextUnderIcon) {
        content = QSize(qMax(iconExtent.width(), textExtent.width()),
                        iconExtent.height() + gap + textExtent.height());
    } else {
        content = QSize(iconExtent.width() + gap + textExtent.width(),
                        qMax(iconExtent.height(), textExtent.height()));
    }

    QSize size(content.width() + metrics.paddingX * 2 + arrowSlotWidth,
               qMax(metrics.height, content.height() + metrics.paddingY * 2));
    size = size.expandedTo(QSize(metrics.height, metrics.height));
    return applyToolButtonShapeToSize(size, m_shape);
}

QSize FluentToolButton::minimumSizeHint() const
{
    return sizeHint();
}

void FluentToolButton::setMenu(QMenu *menu)
{
    if (menu && !menu->parent()) {
        menu->setParent(this);
    }
    QToolButton::setMenu(menu);
    updateGeometry();
    update();
}

void FluentToolButton::changeEvent(QEvent *event)
{
    QToolButton::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentToolButton::applyTheme()
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

void FluentToolButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const auto &colors = ThemeManager::instance().colors();
    const auto &tokens = ThemeManager::instance().tokens();

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);

    QRect rect = this->rect().adjusted(0, 0, -1, -1);

    // Caption / window buttons (used by FluentMainWindow)
    const QVariant glyphVar = property("fluentWindowGlyph");
    if (glyphVar.isValid()) {
        const int glyph = glyphVar.toInt();
        const bool isClose = (glyph == 3);
        const bool neutralClose = property("fluentNeutralCloseGlyph").toBool();

        const qreal hover = isEnabled() ? m_hoverLevel : 0.0;
        const qreal press = isEnabled() ? m_pressLevel : 0.0;

        QColor fill = Qt::transparent;
        if (isClose && !neutralClose) {
            // Win11-like close hover: vivid red hover with a theme-derived pressed shade.
            QColor danger = tokens.semantic.error.isValid() ? tokens.semantic.error : QColor("#E81123");
            const QColor dangerPressedTarget = tokens.dark ? tokens.neutral.background : colors.text;
            QColor dangerPressed = Style::mix(danger, dangerPressedTarget, tokens.dark ? 0.16 : 0.20);
            danger.setAlphaF(0.92);
            dangerPressed.setAlphaF(0.97);
            fill = ButtonVisuals::transparentVersion(danger);
            fill = Style::mix(fill, danger, hover);
            fill = Style::mix(fill, dangerPressed, press);
        } else {
            // Subtle hover/press like caption buttons, derived from neutral tokens.
            QColor hoverFill = tokens.neutral.cardHover;
            hoverFill.setAlphaF(tokens.dark ? 0.62 : 0.56);
            QColor pressedFill = tokens.neutral.fillTertiary;
            pressedFill.setAlphaF(tokens.dark ? 0.78 : 0.72);
            fill = ButtonVisuals::transparentVersion(hoverFill);
            fill = Style::mix(fill, hoverFill, hover);
            fill = Style::mix(fill, pressedFill, press);
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(fill);
        painter.drawRoundedRect(QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5), 4.0, 4.0);

        QColor glyphColor = Style::withAlpha(colors.text, 235);
        if (isClose && !neutralClose && (hover > 0.01 || press > 0.01)) {
            glyphColor = Style::withAlpha(Theme::contrastColor(tokens.semantic.error), 245);
        }

        const qreal strokeW = 1.6;
        painter.setPen(QPen(glyphColor, strokeW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);

        // Use a fixed square glyph box so icons stay 16x16 and never look squashed
        // when the caption button is wide.
        const QPointF center = QRectF(rect).center();
        const QRectF glyphBox(center.x() - 8.0, center.y() - 8.0, 16.0, 16.0);
        const qreal pad = 3.0;
        const QRectF r = glyphBox.adjusted(pad, pad, -pad, -pad);

        switch (glyph) {
        case 0: // minimize
            painter.drawLine(QPointF(r.left(), r.bottom()), QPointF(r.right(), r.bottom()));
            break;
        case 1: // maximize
            painter.drawRoundedRect(r.adjusted(0.7, 0.7, -0.7, -0.7), 1.2, 1.2);
            break;
        case 2: // restore
            {
                // Draw two windows with a visible offset so the glyph doesn't look like
                // perfectly stacked squares.
                const qreal offset = 1.6;
                const qreal shrink = 0.9;
                const QRectF base = r.adjusted(shrink, shrink, -shrink, -shrink);
                const QRectF back = base.translated(offset, -offset);
                const QRectF front = base.translated(-offset, offset);
                painter.drawRoundedRect(back, 1.2, 1.2);
                painter.drawRoundedRect(front, 1.2, 1.2);
            }
            break;
        case 3: // close
            painter.drawLine(QPointF(r.left(), r.top()), QPointF(r.right(), r.bottom()));
            painter.drawLine(QPointF(r.right(), r.top()), QPointF(r.left(), r.bottom()));
            break;
        default:
            break;
        }
        return;
    }

    const bool subtleCommandButton = property("fluentCommandBarButton").toBool();

    const bool checked = isCheckable() && isChecked();
    const ButtonVisuals::StateColors state =
        ButtonVisuals::resolve(colors, tokens, m_primary, checked, isEnabled(), subtleCommandButton, 0.16);
    const QColor fill = ButtonVisuals::fillForState(state, m_hoverLevel, m_pressLevel);
    const qreal radius = controlRadiusForRect(QRectF(rect));

    const QColor bottomBorder = m_shape == Shape::Rounded ? state.bottomBorder : QColor(0, 0, 0, 0);
    const QRectF buttonRect = QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5);
    ButtonVisuals::paintRoundedControl(painter, buttonRect, radius, fill, state.border, bottomBorder);

    if (hasFocus() && isEnabled()) {
        QColor focus = tokens.accent.base;
        focus.setAlpha(230);
        painter.setPen(QPen(focus, 2.0));
        painter.setBrush(Qt::NoBrush);
        const qreal focusRadius = qMax<qreal>(0.0, radius - 1.0);
        painter.drawRoundedRect(QRectF(rect).adjusted(1.0, 1.0, -1.0, -1.0), focusRadius, focusRadius);
    }

    const QColor labelColor = isEnabled()
        ? ButtonVisuals::textForState(state.text, m_pressLevel, true)
        : colors.disabledText;

    const bool hasMenuArrow = menu() && popupMode() != QToolButton::DelayedPopup;
    const qreal arrowSlotWidth = hasMenuArrow ? qreal(kMenuArrowSlotWidth) : 0.0;
    QRectF contentRect = buttonRect.adjusted(Style::metrics().paddingX, Style::metrics().paddingY,
                                             -Style::metrics().paddingX - arrowSlotWidth,
                                             -Style::metrics().paddingY);
    if (contentRect.width() < 0.0) {
        contentRect.setWidth(0.0);
    }
    drawToolButtonContent(painter, this, contentRect, labelColor);

    if (hasMenuArrow) {
        const QPointF center(buttonRect.right() - arrowSlotWidth / 2.0, buttonRect.center().y() + 0.5);
        Style::drawChevronDown(painter, center, labelColor, 6.5, 1.5);
    }
}

void FluentToolButton::enterEvent(FluentEnterEvent *event)
{
    QToolButton::enterEvent(event);
    startHoverAnimation(1.0);
}

void FluentToolButton::leaveEvent(QEvent *event)
{
    QToolButton::leaveEvent(event);
    startHoverAnimation(0.0);
}

void FluentToolButton::mousePressEvent(QMouseEvent *event)
{
    startPressAnimation(1.0);
    if (shouldHandleMenuPress(this, event)) {
        m_menuPressArmed = true;
        setDown(true);
        event->accept();
        return;
    }

    m_menuPressArmed = false;
    QToolButton::mousePressEvent(event);
}

void FluentToolButton::mouseReleaseEvent(QMouseEvent *event)
{
    startPressAnimation(0.0);
    if (m_menuPressArmed) {
        const bool trigger = event
            && event->button() == Qt::LeftButton
            && rect().contains(event->pos())
            && isEnabled();
        m_menuPressArmed = false;
        setDown(false);
        event->accept();
        if (trigger) {
            showFluentMenu();
        }
        return;
    }

    QToolButton::mouseReleaseEvent(event);
}

void FluentToolButton::showFluentMenu()
{
    QMenu *buttonMenu = menu();
    if (!buttonMenu) {
        return;
    }

    const QPoint pos = mapToGlobal(QPoint(0, height() + 4));
    if (auto *fluentMenu = qobject_cast<FluentMenu *>(buttonMenu)) {
        fluentMenu->popup(pos);
    } else {
        buttonMenu->popup(pos);
    }
}

qreal FluentToolButton::controlRadiusForRect(const QRectF &rect) const
{
    if (m_shape == Shape::Pill || m_shape == Shape::Circular) {
        return qMin(rect.width(), rect.height()) / 2.0;
    }

    return Style::metrics().radius;
}

void FluentToolButton::startHoverAnimation(qreal endValue)
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

void FluentToolButton::startPressAnimation(qreal endValue)
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
