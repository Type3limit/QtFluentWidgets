#include "Fluent/FluentTimePicker.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QCoreApplication>
#include <QEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>
#include <QTimer>

namespace Fluent {

namespace {
static QPainterPath rightCornerPath(const QRectF &r, qreal radius, bool roundTopRight, bool roundBottomRight)
{
    const qreal rr = qMax<qreal>(0.0, qMin(radius, qMin(r.width(), r.height()) / 2.0));

    const qreal l = r.left();
    const qreal t = r.top();
    const qreal rt = r.right();
    const qreal b = r.bottom();

    QPainterPath p;
    p.moveTo(l, t);

    if (roundTopRight && rr > 0.0) {
        p.lineTo(rt - rr, t);
        p.quadTo(rt, t, rt, t + rr);
    } else {
        p.lineTo(rt, t);
    }

    if (roundBottomRight && rr > 0.0) {
        p.lineTo(rt, b - rr);
        p.quadTo(rt, b, rt - rr, b);
    } else {
        p.lineTo(rt, b);
    }

    p.lineTo(l, b);
    p.closeSubpath();
    return p;
}
} // namespace

FluentTimePicker::FluentTimePicker(QWidget *parent)
    : QTimeEdit(parent)
{
    setDisplayFormat("HH:mm");
    setButtonSymbols(QAbstractSpinBox::NoButtons);
    setFrame(false);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setMinimumHeight(Style::metrics().height);

    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(150);
    m_hoverAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverLevel = value.toReal();
        update();
    });

    m_focusAnim = new QVariantAnimation(this);
    m_focusAnim->setDuration(200);
    m_focusAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_focusAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_focusLevel = value.toReal();
        update();
    });

    m_stepperAnim = new QVariantAnimation(this);
    m_stepperAnim->setDuration(110);
    m_stepperAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_stepperAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_stepperLevel = value.toReal();
        update();
    });

    ensureEditor();

    // Ensure editor never overlaps our custom button area (cursor / hit-test correctness).
    QTimer::singleShot(0, this, [this]() {
        ensureEditor();
        if (m_editor) {
            const auto m = Style::metrics();
            m_editor->setGeometry(0, 0, qMax(0, width() - m.iconAreaWidth), height());
        }
    });

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentTimePicker::applyTheme);
}

qreal FluentTimePicker::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentTimePicker::setHoverLevel(qreal value)
{
    m_hoverLevel = qBound(0.0, value, 1.0);
    update();
}

qreal FluentTimePicker::focusLevel() const
{
    return m_focusLevel;
}

void FluentTimePicker::setFocusLevel(qreal value)
{
    m_focusLevel = qBound(0.0, value, 1.0);
    update();
}

void FluentTimePicker::changeEvent(QEvent *event)
{
    QTimeEdit::changeEvent(event);

    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentTimePicker::applyTheme()
{
    const auto &colors = ThemeManager::instance().colors();
    {
        const QString next = QString("QTimeEdit { background: transparent; color: %1; }")
                                 .arg(colors.text.name());
        if (styleSheet() != next) {
            setStyleSheet(next);
        }
    }

    ensureEditor();
    if (m_editor) {
        const auto m = Style::metrics();
        m_editor->setTextMargins(m.paddingX, 0, m.iconAreaWidth + 4, 0);
        const QColor textColor = isEnabled() ? colors.text : colors.disabledText;

        const bool dark = ThemeManager::instance().themeMode() == ThemeManager::ThemeMode::Dark;
        QColor selectionBg = colors.accent;
        selectionBg.setAlphaF(dark ? 0.35 : 0.22);
        const QString selectionBgStr = QStringLiteral("rgba(%1,%2,%3,%4)")
                                           .arg(selectionBg.red())
                                           .arg(selectionBg.green())
                                           .arg(selectionBg.blue())
                                           .arg(QString::number(selectionBg.alphaF(), 'f', 3));

        const QString next = QString(
            "QLineEdit { background: transparent; color: %1; border: none; selection-background-color: %2; selection-color: %3; }")
                                 .arg(textColor.name())
                                 .arg(selectionBgStr)
                                 .arg(colors.text.name());
        if (m_editor->styleSheet() != next) {
            m_editor->setStyleSheet(next);
        }

        // Try to make caret follow accent while keeping text color from stylesheet.
        QPalette pal = m_editor->palette();
        pal.setColor(QPalette::Text, colors.accent);
        pal.setColor(QPalette::Highlight, selectionBg);
        pal.setColor(QPalette::HighlightedText, colors.text);
    #if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
        QColor placeholder = colors.subText;
        placeholder.setAlphaF(dark ? 0.55 : 0.60);
        pal.setColor(QPalette::PlaceholderText, placeholder);
    #endif
        m_editor->setPalette(pal);
    }
    update();
}

void FluentTimePicker::ensureEditor()
{
    QLineEdit *le = findChild<QLineEdit *>();
    if (!le || le == m_editor) {
        return;
    }

    m_editor = le;
    m_editor->setFrame(false);
    m_editor->installEventFilter(this);
    m_editor->setMouseTracking(true);
}

bool FluentTimePicker::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_editor && (event->type() == QEvent::MouseMove ||
                               event->type() == QEvent::MouseButtonPress ||
                               event->type() == QEvent::MouseButtonRelease ||
                               event->type() == QEvent::MouseButtonDblClick)) {
        auto *me = static_cast<QMouseEvent *>(event);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const QPoint parentPos = m_editor->mapToParent(me->position().toPoint());
        const QPointF parentPosF(parentPos);
        const ButtonPart part = hitTestButton(parentPos);
        if (part == ButtonPart::None) {
            if (m_hoverButton != ButtonPart::None) {
                m_hoverButton = ButtonPart::None;
                setCursor(Qt::IBeamCursor);
                m_editor->setCursor(Qt::IBeamCursor);
                startStepperAnimation(0.0);
                update();
            }
            return QTimeEdit::eventFilter(watched, event);
        }

        if (part != ButtonPart::None) {
            startStepperAnimation(1.0);
            QMouseEvent mapped(me->type(), parentPosF, me->globalPosition(), me->button(), me->buttons(), me->modifiers());
            QCoreApplication::sendEvent(this, &mapped);
            return true;
        }
#else
        const QPoint parentPos = m_editor->mapToParent(me->pos());
        const ButtonPart part = hitTestButton(parentPos);
        if (part == ButtonPart::None) {
            if (m_hoverButton != ButtonPart::None) {
                m_hoverButton = ButtonPart::None;
                setCursor(Qt::IBeamCursor);
                m_editor->setCursor(Qt::IBeamCursor);
                startStepperAnimation(0.0);
                update();
            }
            return QTimeEdit::eventFilter(watched, event);
        }

        if (part != ButtonPart::None) {
            startStepperAnimation(1.0);
            QMouseEvent mapped(me->type(), parentPos, me->globalPos(), me->button(), me->buttons(), me->modifiers());
            QCoreApplication::sendEvent(this, &mapped);
            return true;
        }
#endif
    }

    return QTimeEdit::eventFilter(watched, event);
}

void FluentTimePicker::resizeEvent(QResizeEvent *event)
{
    QTimeEdit::resizeEvent(event);

    // Keep the editor from painting/handling events over our custom button area.
    ensureEditor();
    if (m_editor) {
        const auto m = Style::metrics();
        m_editor->setGeometry(0, 0, qMax(0, width() - m.iconAreaWidth), height());
    }
}

void FluentTimePicker::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const auto &colors = ThemeManager::instance().colors();

    const bool enabled = isEnabled();
    const bool pressed = (m_pressedButton != ButtonPart::None);

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF rect = QRectF(this->rect());
    Style::paintControlSurface(painter, rect, colors, m_hoverLevel, m_focusLevel, enabled, pressed);

    const auto m = Style::metrics();
    const QRect buttonRect(width() - m.iconAreaWidth, 0, m.iconAreaWidth, height());

    QColor divider = colors.border;
    divider.setAlpha(80);
    painter.setPen(QPen(divider, 1));
    painter.drawLine(QPointF(buttonRect.left() + 0.5, buttonRect.top() + 8), QPointF(buttonRect.left() + 0.5, buttonRect.bottom() - 8));

    const int gap = 2;
    const int halfH = (buttonRect.height() - gap) / 2;
    const QRect upRect(buttonRect.left() + 1, buttonRect.top() + 1, buttonRect.width() - 2, halfH - 1);
    const QRect downRect(buttonRect.left() + 1, buttonRect.top() + halfH + gap, buttonRect.width() - 2, halfH - 1);

    auto paintButtonBg = [&](const QRect &r, ButtonPart part) {
        if (!enabled) return;
        const bool isPressed = (m_pressedButton == part);
        const bool isHovered = (m_hoverButton == part);
        if (!isPressed && !isHovered) return;

        const qreal t = isPressed ? 1.0 : (isHovered ? qBound<qreal>(0.0, m_stepperLevel, 1.0) : 0.0);
        // Stepper backgrounds should be more visible than generic hover/pressed.
        const QColor base = Style::mix(colors.surface, colors.accent, 0.12);
        const QColor hover = Style::mix(colors.surface, colors.accent, 0.18);
        const QColor pressedFill = Style::mix(colors.surface, colors.accent, 0.26);
        const QColor fill = isPressed ? pressedFill : Style::mix(base, hover, t);
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);

        // Keep background strictly inside border/focus ring.
        const auto m = Style::metrics();
        const qreal inset = (hasFocus() && enabled) ? 2.0 : 1.0;
        const QRectF clipRect = QRectF(this->rect()).adjusted(inset, inset, -inset, -inset);
        painter.setClipPath(Style::roundedRectPath(clipRect, qMax<qreal>(0.0, m.radius - inset)));

        painter.setPen(Qt::NoPen);
        painter.setBrush(fill);

        // Up: only top-right rounded. Down: only bottom-right rounded.
        const QRectF rf(r);
        const qreal corner = qMax<qreal>(0.0, m.radius - inset);
        const bool isUp = (part == ButtonPart::Up);
        painter.drawPath(rightCornerPath(rf, corner, isUp, !isUp));
        painter.restore();
    };

    paintButtonBg(upRect, ButtonPart::Up);
    paintButtonBg(downRect, ButtonPart::Down);

    const QColor iconColor = enabled ? colors.subText : colors.disabledText;
    Style::drawChevronUp(painter, upRect.center(), iconColor, 7.0, 1.6);
    Style::drawChevronDown(painter, downRect.center(), iconColor, 7.0, 1.6);
}

void FluentTimePicker::enterEvent(QEvent *event)
{
    QTimeEdit::enterEvent(event);
    startHoverAnimation(1.0);
}

void FluentTimePicker::leaveEvent(QEvent *event)
{
    QTimeEdit::leaveEvent(event);
    m_hoverButton = ButtonPart::None;
    startStepperAnimation(0.0);
    startHoverAnimation(0.0);
}

void FluentTimePicker::focusInEvent(QFocusEvent *event)
{
    QTimeEdit::focusInEvent(event);
    startFocusAnimation(1.0);
}

void FluentTimePicker::focusOutEvent(QFocusEvent *event)
{
    QTimeEdit::focusOutEvent(event);
    startFocusAnimation(0.0);
}

void FluentTimePicker::startHoverAnimation(qreal endValue)
{
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(endValue);
    m_hoverAnim->start();
}

void FluentTimePicker::startFocusAnimation(qreal endValue)
{
    m_focusAnim->stop();
    m_focusAnim->setStartValue(m_focusLevel);
    m_focusAnim->setEndValue(endValue);
    m_focusAnim->start();
}

void FluentTimePicker::startStepperAnimation(qreal endValue)
{
    if (!m_stepperAnim) {
        m_stepperLevel = endValue;
        update();
        return;
    }

    m_stepperAnim->stop();
    m_stepperAnim->setStartValue(m_stepperLevel);
    m_stepperAnim->setEndValue(qBound<qreal>(0.0, endValue, 1.0));
    m_stepperAnim->start();
}

void FluentTimePicker::mousePressEvent(QMouseEvent *event)
{
    const ButtonPart part = hitTestButton(event->pos());
    if (event->button() == Qt::LeftButton && part != ButtonPart::None && isEnabled()) {
        m_pressedButton = part;
        startStepperAnimation(1.0);
        update();
        if (part == ButtonPart::Up) {
            stepUp();
        } else {
            stepDown();
        }
        event->accept();
        return;
    }

    QTimeEdit::mousePressEvent(event);
}

void FluentTimePicker::mouseMoveEvent(QMouseEvent *event)
{
    const ButtonPart part = hitTestButton(event->pos());
    if (part != m_hoverButton) {
        m_hoverButton = part;
        const Qt::CursorShape shape = (part == ButtonPart::None) ? Qt::IBeamCursor : Qt::PointingHandCursor;
        setCursor(shape);
        if (m_editor) {
            m_editor->setCursor(shape);
        }
        startStepperAnimation(part == ButtonPart::None ? 0.0 : 1.0);
        update();
    }
    if (part != ButtonPart::None) {
        event->accept();
        return;
    }

    QTimeEdit::mouseMoveEvent(event);
}

void FluentTimePicker::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_pressedButton != ButtonPart::None) {
        m_pressedButton = ButtonPart::None;
        startStepperAnimation(m_hoverButton == ButtonPart::None ? 0.0 : 1.0);
        update();
    }
    QTimeEdit::mouseReleaseEvent(event);
}

FluentTimePicker::ButtonPart FluentTimePicker::hitTestButton(const QPoint &pos) const
{
    const auto m = Style::metrics();
    const QRect buttonRect(width() - m.iconAreaWidth, 0, m.iconAreaWidth, height());
    if (!buttonRect.contains(pos)) {
        return ButtonPart::None;
    }
    const int gap = 2;
    const int halfH = (buttonRect.height() - gap) / 2;
    const int splitY = buttonRect.top() + halfH + gap / 2;
    return (pos.y() < splitY) ? ButtonPart::Up : ButtonPart::Down;
}

} // namespace Fluent
