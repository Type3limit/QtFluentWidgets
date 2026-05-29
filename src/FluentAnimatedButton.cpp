#include "Fluent/FluentAnimatedButton.h"

#include "Fluent/FluentMotion.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QVariantAnimation>

namespace Fluent {

namespace {

constexpr int kIconTextGap = 8;

struct ButtonColors
{
    QColor base;
    QColor hover;
    QColor pressed;
    QColor border;
    QColor text;
};

QString normalizedState(QString state)
{
    state = state.trimmed();
    return state.isEmpty() ? QStringLiteral("Normal") : state;
}

QSize boundedIconSize(const QSize &size)
{
    if (size.isValid()) {
        return size;
    }
    return QSize(24, 24);
}

ButtonColors resolveButtonColors(const ThemeColors &colors, bool primary, bool checked, bool enabled)
{
    ButtonColors resolved;
    const auto tokens = Theme::tokens(colors);

    if (primary) {
        resolved.base = checked ? tokens.accent.dark1 : tokens.accent.base;
        resolved.hover = tokens.accent.light1;
        resolved.pressed = tokens.accent.dark1;
        resolved.border = Style::mix(tokens.accent.base, tokens.onAccent, 0.18);
        resolved.text = tokens.onAccent;
    } else {
        const QColor accentTint = Style::mix(colors.surface, colors.accent, 0.12);
        resolved.base = checked ? accentTint : colors.surface;
        resolved.hover = checked
                             ? Style::mix(accentTint, colors.accent, 0.10)
                             : Style::mix(colors.surface, colors.hover, 0.88);
        resolved.pressed = checked
                               ? Style::mix(accentTint, colors.accent, 0.18)
                               : Style::mix(colors.surface, colors.pressed, 0.92);
        resolved.border = checked ? Style::mix(colors.border, colors.accent, 0.85) : colors.border;
        resolved.text = checked ? Theme::contrastColor(accentTint) : colors.text;
    }

    if (!enabled) {
        resolved.base = Style::mix(colors.surface, colors.hover, 0.45);
        resolved.hover = resolved.base;
        resolved.pressed = resolved.base;
        resolved.border = Style::mix(colors.border, colors.disabledText, 0.25);
        resolved.text = colors.disabledText;
    }

    return resolved;
}

} // namespace

FluentAnimatedButton::FluentAnimatedButton(QWidget *parent)
    : QPushButton(parent)
{
    initialize();
}

FluentAnimatedButton::FluentAnimatedButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
{
    initialize();
}

bool FluentAnimatedButton::isPrimary() const
{
    return m_primary;
}

void FluentAnimatedButton::setPrimary(bool primary)
{
    if (m_primary == primary) {
        return;
    }

    m_primary = primary;
    applyTheme();
}

qreal FluentAnimatedButton::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentAnimatedButton::setHoverLevel(qreal value)
{
    m_hoverLevel = qBound<qreal>(0.0, value, 1.0);
    update();
}

qreal FluentAnimatedButton::pressLevel() const
{
    return m_pressLevel;
}

void FluentAnimatedButton::setPressLevel(qreal value)
{
    m_pressLevel = qBound<qreal>(0.0, value, 1.0);
    update();
}

QSize FluentAnimatedButton::animatedIconSize() const
{
    return m_animatedIconSize;
}

void FluentAnimatedButton::setAnimatedIconSize(const QSize &size)
{
    const QSize next = boundedIconSize(size);
    if (m_animatedIconSize == next) {
        return;
    }

    m_animatedIconSize = next;
    m_animatedIcon->setFallbackIconSize(next);
    m_animatedIcon->setFixedSize(next);
    updateAnimatedIconGeometry();
    updateGeometry();
    update();
}

FluentAnimatedIcon *FluentAnimatedButton::animatedIcon() const
{
    return m_animatedIcon;
}

bool FluentAnimatedButton::loadAnimation(const QString &path)
{
    const bool ok = m_animatedIcon->load(path);
    syncRestingAnimationState(false);
    updateAnimatedIconGeometry();
    return ok;
}

bool FluentAnimatedButton::loadAnimationData(const QByteArray &json, const QString &cacheKey, const QString &resourcePath)
{
    const bool ok = m_animatedIcon->loadData(json, cacheKey, resourcePath);
    syncRestingAnimationState(false);
    updateAnimatedIconGeometry();
    return ok;
}

QString FluentAnimatedButton::animationState() const
{
    return m_animationState;
}

void FluentAnimatedButton::setAnimationState(const QString &state)
{
    setAnimationState(state, true);
}

void FluentAnimatedButton::setAnimationState(const QString &state, bool animated)
{
    const QString next = normalizedState(state);
    if (m_animationState == next) {
        return;
    }

    m_animationState = next;
    m_animatedIcon->setState(next, animated);
    emit animationStateChanged(next);
}

QSize FluentAnimatedButton::sizeHint() const
{
    const auto m = Style::metrics();
    const int textWidth = text().isEmpty() ? 0 : fontMetrics().horizontalAdvance(text());
    const int iconWidth = m_animatedIconSize.width();
    const int contentWidth = textWidth > 0 ? iconWidth + kIconTextGap + textWidth : iconWidth;
    const int width = contentWidth + m.paddingX * 2;
    const int height = qMax(m.height, m_animatedIconSize.height() + m.paddingY * 2);
    return QSize(width, height);
}

QSize FluentAnimatedButton::minimumSizeHint() const
{
    return QSize(qMax(32, m_animatedIconSize.width() + 12), qMax(32, m_animatedIconSize.height() + 8));
}

void FluentAnimatedButton::changeEvent(QEvent *event)
{
    QPushButton::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
        syncRestingAnimationState(true);
    } else if (event->type() == QEvent::FontChange) {
        updateAnimatedIconGeometry();
        updateGeometry();
    }
}

void FluentAnimatedButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    updateAnimatedIconGeometry();

    const auto &colors = ThemeManager::instance().colors();
    const bool checked = isCheckable() && isChecked();
    const ButtonColors buttonColors = resolveButtonColors(colors, m_primary, checked, isEnabled());

    QColor fill = Style::mix(buttonColors.base, buttonColors.hover, m_hoverLevel);
    fill = Style::mix(fill, buttonColors.pressed, m_pressLevel);

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const auto m = Style::metrics();
    const QRectF rect = QRectF(this->rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal radius = m.radius;

    painter.setPen(QPen(buttonColors.border, 1.0));
    painter.setBrush(fill);
    painter.drawRoundedRect(rect, radius, radius);

    if (checked && m_primary && isEnabled()) {
        QColor inner = buttonColors.text;
        inner.setAlpha(115);
        painter.setPen(QPen(inner, 1.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect.adjusted(1.0, 1.0, -1.0, -1.0), radius - 1, radius - 1);
    }

    if (hasFocus() && isEnabled()) {
        QColor focus = colors.focus;
        focus.setAlpha(230);
        painter.setPen(QPen(focus, 2.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), radius - 1, radius - 1);
    }

    if (text().isEmpty()) {
        return;
    }

    const QRect contentRect = rect.toRect();
    const int textWidth = fontMetrics().horizontalAdvance(text());
    const int totalWidth = m_animatedIconSize.width() + kIconTextGap + textWidth;
    const int startX = contentRect.center().x() - totalWidth / 2;
    const int textLeft = startX + m_animatedIconSize.width() + kIconTextGap;
    const QRect textRect(textLeft,
                         contentRect.top(),
                         contentRect.right() - textLeft + 1,
                         contentRect.height());

    painter.setPen(buttonColors.text);
    painter.setFont(font());
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic, text());
}

void FluentAnimatedButton::resizeEvent(QResizeEvent *event)
{
    QPushButton::resizeEvent(event);
    updateAnimatedIconGeometry();
}

void FluentAnimatedButton::enterEvent(FluentEnterEvent *event)
{
    QPushButton::enterEvent(event);
    startHoverAnimation(1.0);
    if (isEnabled()) {
        setAnimationState(QStringLiteral("PointerOver"), true);
    }
}

void FluentAnimatedButton::leaveEvent(QEvent *event)
{
    QPushButton::leaveEvent(event);
    startHoverAnimation(0.0);
    startPressAnimation(0.0);
    syncRestingAnimationState(true);
}

void FluentAnimatedButton::mousePressEvent(QMouseEvent *event)
{
    startPressAnimation(1.0);
    if (isEnabled() && event->button() == Qt::LeftButton) {
        setAnimationState(QStringLiteral("Pressed"), true);
    }
    QPushButton::mousePressEvent(event);
}

void FluentAnimatedButton::mouseReleaseEvent(QMouseEvent *event)
{
    startPressAnimation(0.0);
    QPushButton::mouseReleaseEvent(event);
    if (isEnabled() && event->button() == Qt::LeftButton) {
        if (rect().contains(mousePositionF(event).toPoint())) {
            setAnimationState(QStringLiteral("PointerOver"), true);
        } else {
            syncRestingAnimationState(true);
        }
    }
}

void FluentAnimatedButton::initialize()
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setAutoDefault(false);
    setMinimumHeight(Style::metrics().height);

    m_animatedIcon = new FluentAnimatedIcon(this);
    m_animatedIcon->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_animatedIcon->setInteractive(false);
    m_animatedIcon->setFallbackIconSize(m_animatedIconSize);
    m_animatedIcon->setFixedSize(m_animatedIconSize);

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
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentAnimatedButton::applyTheme);
    connect(this, &QAbstractButton::toggled, this, [this](bool) {
        updateAnimatedIconTint();
        syncRestingAnimationState(true);
        update();
    });
}

void FluentAnimatedButton::applyTheme()
{
    FluentMotion::configure(m_hoverAnim, FluentMotionRole::Hover);
    FluentMotion::configure(m_pressAnim, FluentMotionRole::Press);
    updateAnimatedIconTint();
    update();
}

void FluentAnimatedButton::startHoverAnimation(qreal endValue)
{
    m_hoverAnim->stop();
    if (m_hoverAnim->duration() <= 0) {
        setHoverLevel(endValue);
        return;
    }
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(endValue);
    m_hoverAnim->start();
}

void FluentAnimatedButton::startPressAnimation(qreal endValue)
{
    m_pressAnim->stop();
    if (m_pressAnim->duration() <= 0) {
        setPressLevel(endValue);
        return;
    }
    m_pressAnim->setStartValue(m_pressLevel);
    m_pressAnim->setEndValue(endValue);
    m_pressAnim->start();
}

void FluentAnimatedButton::updateAnimatedIconGeometry()
{
    if (!m_animatedIcon) {
        return;
    }

    const QRect contentRect = rect();
    int x = contentRect.center().x() - m_animatedIconSize.width() / 2;

    if (!text().isEmpty()) {
        const int textWidth = fontMetrics().horizontalAdvance(text());
        const int totalWidth = m_animatedIconSize.width() + kIconTextGap + textWidth;
        x = contentRect.center().x() - totalWidth / 2;
    }

    const int y = contentRect.center().y() - m_animatedIconSize.height() / 2;
    m_animatedIcon->setGeometry(QRect(QPoint(x, y), m_animatedIconSize));
}

void FluentAnimatedButton::updateAnimatedIconTint()
{
    if (!m_animatedIcon) {
        return;
    }

    m_animatedIcon->setTintColor(currentForegroundColor());
}

QColor FluentAnimatedButton::currentForegroundColor() const
{
    const bool checked = isCheckable() && isChecked();
    return resolveButtonColors(ThemeManager::instance().colors(), m_primary, checked, isEnabled()).text;
}

void FluentAnimatedButton::syncRestingAnimationState(bool animated)
{
    if (!isEnabled() && hasAnimationState(QStringLiteral("Disabled"))) {
        setAnimationState(QStringLiteral("Disabled"), animated);
        return;
    }

    if (isCheckable() && isChecked() && hasAnimationState(QStringLiteral("Selected"))) {
        setAnimationState(QStringLiteral("Selected"), animated);
        return;
    }

    setAnimationState(QStringLiteral("Normal"), animated);
}

bool FluentAnimatedButton::hasAnimationState(const QString &state) const
{
    if (m_animatedIcon->hasMarker(state)) {
        return true;
    }

    const QString suffix = QStringLiteral("To") + state + QStringLiteral("_End");
    const QStringList names = m_animatedIcon->markerNames();
    for (const QString &name : names) {
        if (name.endsWith(suffix, Qt::CaseInsensitive)) {
            return true;
        }
    }

    return false;
}

} // namespace Fluent
