#include "Fluent/FluentSpinBox.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QCoreApplication>
#include <QEvent>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QLineEdit>
#include <QPainter>
#include <QMouseEvent>
#include <QEasingCurve>
#include <QResizeEvent>
#include <QStringList>
#include <QTimer>

namespace Fluent {

namespace {
static int textAdvancePx(const QFontMetrics &fm, const QString &s)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    return fm.horizontalAdvance(s);
#else
    return fm.width(s);
#endif
}

static QPainterPath rightCornerPath(const QRectF &r, qreal radius, bool roundTopRight, bool roundBottomRight)
{
    const qreal rr = qMax<qreal>(0.0, qMin(radius, qMin(r.width(), r.height()) / 2.0));

    const qreal l = r.left();
    const qreal t = r.top();
    const qreal rt = r.right();
    const qreal b = r.bottom();

    QPainterPath p;
    p.moveTo(l, t);

    // Top edge -> top-right corner
    if (roundTopRight && rr > 0.0) {
        p.lineTo(rt - rr, t);
        p.quadTo(rt, t, rt, t + rr);
    } else {
        p.lineTo(rt, t);
    }

    // Right edge -> bottom-right corner
    if (roundBottomRight && rr > 0.0) {
        p.lineTo(rt, b - rr);
        p.quadTo(rt, b, rt - rr, b);
    } else {
        p.lineTo(rt, b);
    }

    // Bottom edge -> left edge
    p.lineTo(l, b);
    p.closeSubpath();
    return p;
}
} // namespace

FluentSpinBox::FluentSpinBox(QWidget *parent)
    : QSpinBox(parent)
    , m_hoverLevel(0.0)
    , m_focusLevel(0.0)
{
    setButtonSymbols(QAbstractSpinBox::NoButtons);
    setFrame(false);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setMinimumHeight(Style::metrics().height);

    ensureEditor();
    if (m_editor) {
        const auto m = Style::metrics();
        m_editor->setFrame(false);
        // The editor's geometry already excludes the stepper button area.
        // Keep only a small right padding so text doesn't get clipped.
        m_editor->setTextMargins(m.paddingX, 0, m.paddingX, 0);
    }

    // Make sure the editor never covers the button area (affects cursor hit-testing).
    QTimer::singleShot(0, this, [this]() {
        ensureEditor();
        if (m_editor) {
            const auto m = Style::metrics();
            m_editor->setGeometry(0, 0, qMax(0, width() - m.iconAreaWidth), height());
        }
    });

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

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentSpinBox::applyTheme);
}

QSize FluentSpinBox::sizeHint() const
{
    QSize sz = QSpinBox::sizeHint();
    const auto m = Style::metrics();
    sz.setHeight(qMax(sz.height(), m.height));

    // Compute a content-based minimum width so numbers/prefix/suffix don't get clipped.
    // Users can still override with setMinimumWidth()/setFixedWidth().
    QStringList candidates;
    candidates << (prefix() + textFromValue(minimum()) + suffix());
    candidates << (prefix() + textFromValue(maximum()) + suffix());
    candidates << (prefix() + textFromValue(value()) + suffix());
    if (!specialValueText().isEmpty()) {
        candidates << specialValueText();
    }

    int textW = 0;
    const QFontMetrics fm(font());
    for (const QString &s : candidates) {
        textW = qMax(textW, textAdvancePx(fm, s));
    }

    // Match the editor margins we set in ensureEditor():
    // left = paddingX, right = iconAreaWidth + 4 (button area) plus a little padding.
    const int leftPad = m.paddingX;
    const int rightPad = m.paddingX;
    const int dividerGap = 4;
    const int borderAndSafety = 6;
    const int autoMinW = qMax(0, textW + leftPad + rightPad + m.iconAreaWidth + dividerGap + borderAndSafety);
    sz.setWidth(qMax(sz.width(), autoMinW));
    return sz;
}

QSize FluentSpinBox::minimumSizeHint() const
{
    return sizeHint();
}

void FluentSpinBox::changeEvent(QEvent *event)
{
    QSpinBox::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentSpinBox::applyTheme()
{
    const auto &colors = ThemeManager::instance().colors();
    ensureEditor();
    if (m_editor) {
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

void FluentSpinBox::ensureEditor()
{
    QLineEdit *le = lineEdit();
    if (!le || le == m_editor) {
        return;
    }

    m_editor = le;
    m_editor->setFrame(false);
    m_editor->installEventFilter(this);
    m_editor->setMouseTracking(true);
}

bool FluentSpinBox::eventFilter(QObject *watched, QEvent *event)
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
            return QSpinBox::eventFilter(watched, event);
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
            return QSpinBox::eventFilter(watched, event);
        }

        if (part != ButtonPart::None) {
            startStepperAnimation(1.0);
            QMouseEvent mapped(me->type(), parentPos, me->globalPos(), me->button(), me->buttons(), me->modifiers());
            QCoreApplication::sendEvent(this, &mapped);
            return true;
        }
#endif
    }

    return QSpinBox::eventFilter(watched, event);
}

void FluentSpinBox::resizeEvent(QResizeEvent *event)
{
    QSpinBox::resizeEvent(event);

    // Keep the editor from painting/handling events over our custom button area.
    ensureEditor();
    if (m_editor) {
        const auto m = Style::metrics();
        m_editor->setGeometry(0, 0, qMax(0, width() - m.iconAreaWidth), height());
    }
}

void FluentSpinBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const auto &colors = ThemeManager::instance().colors();
    const bool dark = ThemeManager::instance().themeMode() == ThemeManager::ThemeMode::Dark;

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
    const QRect buttonRect(this->width() - m.iconAreaWidth, 0, m.iconAreaWidth, this->height());

    // Make the stepper column slightly distinguishable in dark mode even when idle.
    // This improves discoverability without adding extra borders.
    if (enabled && dark) {
        painter.save();
        const qreal inset = (hasFocus() && enabled) ? 2.0 : 1.0;
        const QRectF clipRect = QRectF(this->rect()).adjusted(inset, inset, -inset, -inset);
        painter.setClipPath(Style::roundedRectPath(clipRect, qMax<qreal>(0.0, m.radius - inset)));
        painter.setPen(Qt::NoPen);
        const QColor idleFill = Style::mix(colors.surface, colors.hover, 0.85);
        painter.setBrush(idleFill);
        const QRectF rf(buttonRect.adjusted(1, 1, -1, -1));
        const qreal corner = qMax<qreal>(0.0, m.radius - inset);
        painter.drawPath(rightCornerPath(rf, corner, true, true));
        painter.restore();
    }

    // Divider
    QColor divider = colors.border;
    divider.setAlpha(80);
    painter.setPen(QPen(divider, 1));
    painter.drawLine(QPointF(buttonRect.left() + 0.5, buttonRect.top() + 8), QPointF(buttonRect.left() + 0.5, buttonRect.bottom() - 8));

    // Button hover/pressed backgrounds (with a small gap)
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
        // Stepper backgrounds should be more visible than generic hover/pressed,
        // especially on small hit targets. Use an accent-tinted fill.
        const qreal baseK = dark ? 0.20 : 0.12;
        const qreal hoverK = dark ? 0.28 : 0.18;
        const qreal pressedK = dark ? 0.40 : 0.26;
        const QColor base = Style::mix(colors.surface, colors.accent, baseK);
        const QColor hover = Style::mix(colors.surface, colors.accent, hoverK);
        const QColor pressedFill = Style::mix(colors.surface, colors.accent, pressedK);
        const QColor fill = isPressed ? pressedFill : Style::mix(base, hover, t);
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);

        // Never let button background bleed into the control border/focus ring.
        // Focus ring is 2px; keep a conservative inset when focused.
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

void FluentSpinBox::startStepperAnimation(qreal endValue)
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

void FluentSpinBox::enterEvent(QEvent *event)
{
    QSpinBox::enterEvent(event);
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(1.0);
    m_hoverAnim->start();
}

void FluentSpinBox::leaveEvent(QEvent *event)
{
    QSpinBox::leaveEvent(event);
    m_hoverButton = ButtonPart::None;
    startStepperAnimation(0.0);
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(0.0);
    m_hoverAnim->start();
}

void FluentSpinBox::focusInEvent(QFocusEvent *event)
{
    QSpinBox::focusInEvent(event);
    m_focusAnim->stop();
    m_focusAnim->setStartValue(m_focusLevel);
    m_focusAnim->setEndValue(1.0);
    m_focusAnim->start();
}

void FluentSpinBox::focusOutEvent(QFocusEvent *event)
{
    QSpinBox::focusOutEvent(event);
    m_focusAnim->stop();
    m_focusAnim->setStartValue(m_focusLevel);
    m_focusAnim->setEndValue(0.0);
    m_focusAnim->start();
}

void FluentSpinBox::mousePressEvent(QMouseEvent *event)
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

    QSpinBox::mousePressEvent(event);
}

void FluentSpinBox::mouseMoveEvent(QMouseEvent *event)
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

    QSpinBox::mouseMoveEvent(event);
}

void FluentSpinBox::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_pressedButton != ButtonPart::None) {
        m_pressedButton = ButtonPart::None;
        startStepperAnimation(m_hoverButton == ButtonPart::None ? 0.0 : 1.0);
        update();
    }
    QSpinBox::mouseReleaseEvent(event);
}

FluentSpinBox::ButtonPart FluentSpinBox::hitTestButton(const QPoint &pos) const
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

FluentDoubleSpinBox::FluentDoubleSpinBox(QWidget *parent)
    : QDoubleSpinBox(parent)
{
    setButtonSymbols(QAbstractSpinBox::NoButtons);
    setFrame(false);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setMinimumHeight(Style::metrics().height);

    ensureEditor();
    if (m_editor) {
        const auto m = Style::metrics();
        m_editor->setFrame(false);
        // The editor's geometry already excludes the stepper button area.
        m_editor->setTextMargins(m.paddingX, 0, m.paddingX, 0);
    }

    QTimer::singleShot(0, this, [this]() {
        ensureEditor();
        if (m_editor) {
            const auto m = Style::metrics();
            m_editor->setGeometry(0, 0, qMax(0, width() - m.iconAreaWidth), height());
        }
    });

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

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentDoubleSpinBox::applyTheme);
}

QSize FluentDoubleSpinBox::sizeHint() const
{
    QSize sz = QDoubleSpinBox::sizeHint();
    const auto m = Style::metrics();
    sz.setHeight(qMax(sz.height(), m.height));

    // Compute a content-based minimum width so the full formatted number is visible.
    QStringList candidates;
    candidates << (prefix() + textFromValue(minimum()) + suffix());
    candidates << (prefix() + textFromValue(maximum()) + suffix());
    candidates << (prefix() + textFromValue(value()) + suffix());
    if (!specialValueText().isEmpty()) {
        candidates << specialValueText();
    }

    int textW = 0;
    const QFontMetrics fm(font());
    for (const QString &s : candidates) {
        textW = qMax(textW, textAdvancePx(fm, s));
    }

    const int leftPad = m.paddingX;
    const int rightPad = m.paddingX;
    const int dividerGap = 4;
    const int borderAndSafety = 6;
    const int autoMinW = qMax(0, textW + leftPad + rightPad + m.iconAreaWidth + dividerGap + borderAndSafety);
    sz.setWidth(qMax(sz.width(), autoMinW));
    return sz;
}

QSize FluentDoubleSpinBox::minimumSizeHint() const
{
    return sizeHint();
}

void FluentDoubleSpinBox::changeEvent(QEvent *event)
{
    QDoubleSpinBox::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentDoubleSpinBox::applyTheme()
{
    const auto &colors = ThemeManager::instance().colors();
    ensureEditor();
    if (m_editor) {
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

void FluentDoubleSpinBox::ensureEditor()
{
    QLineEdit *le = lineEdit();
    if (!le || le == m_editor) {
        return;
    }

    m_editor = le;
    m_editor->setFrame(false);
    m_editor->installEventFilter(this);
    m_editor->setMouseTracking(true);
}

bool FluentDoubleSpinBox::eventFilter(QObject *watched, QEvent *event)
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
            return QDoubleSpinBox::eventFilter(watched, event);
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
            return QDoubleSpinBox::eventFilter(watched, event);
        }

        if (part != ButtonPart::None) {
            startStepperAnimation(1.0);
            QMouseEvent mapped(me->type(), parentPos, me->globalPos(), me->button(), me->buttons(), me->modifiers());
            QCoreApplication::sendEvent(this, &mapped);
            return true;
        }
#endif
    }

    return QDoubleSpinBox::eventFilter(watched, event);
}

void FluentDoubleSpinBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const auto &colors = ThemeManager::instance().colors();

    const bool dark = ThemeManager::instance().themeMode() == ThemeManager::ThemeMode::Dark;

    const bool enabled = isEnabled();
    const bool pressed = (m_pressedButton != ButtonPart::None);

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = QRectF(this->rect());
    Style::paintControlSurface(painter, r, colors, m_hoverLevel, m_focusLevel, enabled, pressed);

    const auto m = Style::metrics();
    const QRect buttonRect(this->width() - m.iconAreaWidth, 0, m.iconAreaWidth, this->height());

    // Match FluentSpinBox: make the stepper column slightly distinguishable in dark mode.
    if (enabled && dark) {
        painter.save();
        const qreal inset = (hasFocus() && enabled) ? 2.0 : 1.0;
        const QRectF clipRect = QRectF(this->rect()).adjusted(inset, inset, -inset, -inset);
        painter.setClipPath(Style::roundedRectPath(clipRect, qMax<qreal>(0.0, m.radius - inset)));
        painter.setPen(Qt::NoPen);
        const QColor idleFill = Style::mix(colors.surface, colors.hover, 0.85);
        painter.setBrush(idleFill);
        const QRectF rf(buttonRect.adjusted(1, 1, -1, -1));
        const qreal corner = qMax<qreal>(0.0, m.radius - inset);
        painter.drawPath(rightCornerPath(rf, corner, true, true));
        painter.restore();
    }

    QColor divider = colors.border;
    divider.setAlpha(80);
    painter.setPen(QPen(divider, 1));
    painter.drawLine(QPointF(buttonRect.left() + 0.5, buttonRect.top() + 8), QPointF(buttonRect.left() + 0.5, buttonRect.bottom() - 8));

    const int gap = 2;
    const int halfH = (buttonRect.height() - gap) / 2;
    const QRect upRect(buttonRect.left() + 1, buttonRect.top() + 1, buttonRect.width() - 2, halfH - 1);
    const QRect downRect(buttonRect.left() + 1, buttonRect.top() + halfH + gap, buttonRect.width() - 2, halfH - 1);

    auto paintButtonBg = [&](const QRect &rr, ButtonPart part) {
        if (!enabled) return;
        const bool isPressed = (m_pressedButton == part);
        const bool isHovered = (m_hoverButton == part);
        if (!isPressed && !isHovered) return;

        const qreal t = isPressed ? 1.0 : (isHovered ? qBound<qreal>(0.0, m_stepperLevel, 1.0) : 0.0);
        // Keep consistent with FluentSpinBox: dark mode needs stronger contrast.
        const qreal baseK = dark ? 0.20 : 0.12;
        const qreal hoverK = dark ? 0.28 : 0.18;
        const qreal pressedK = dark ? 0.40 : 0.26;
        const QColor base = Style::mix(colors.surface, colors.accent, baseK);
        const QColor hover = Style::mix(colors.surface, colors.accent, hoverK);
        const QColor pressedFill = Style::mix(colors.surface, colors.accent, pressedK);
        const QColor fill = isPressed ? pressedFill : Style::mix(base, hover, t);
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);

        const qreal inset = (hasFocus() && enabled) ? 2.0 : 1.0;
        const QRectF clipRect = QRectF(this->rect()).adjusted(inset, inset, -inset, -inset);
        painter.setClipPath(Style::roundedRectPath(clipRect, qMax<qreal>(0.0, m.radius - inset)));

        painter.setPen(Qt::NoPen);
        painter.setBrush(fill);

        const QRectF rf(rr);
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

void FluentDoubleSpinBox::startStepperAnimation(qreal endValue)
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

void FluentDoubleSpinBox::resizeEvent(QResizeEvent *event)
{
    QDoubleSpinBox::resizeEvent(event);
    ensureEditor();
    if (m_editor) {
        const auto m = Style::metrics();
        m_editor->setGeometry(0, 0, qMax(0, width() - m.iconAreaWidth), height());
    }
}

void FluentDoubleSpinBox::enterEvent(QEvent *event)
{
    QDoubleSpinBox::enterEvent(event);
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(1.0);
    m_hoverAnim->start();
}

void FluentDoubleSpinBox::leaveEvent(QEvent *event)
{
    QDoubleSpinBox::leaveEvent(event);
    m_hoverButton = ButtonPart::None;
    startStepperAnimation(0.0);
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(0.0);
    m_hoverAnim->start();
}

void FluentDoubleSpinBox::focusInEvent(QFocusEvent *event)
{
    QDoubleSpinBox::focusInEvent(event);
    m_focusAnim->stop();
    m_focusAnim->setStartValue(m_focusLevel);
    m_focusAnim->setEndValue(1.0);
    m_focusAnim->start();
}

void FluentDoubleSpinBox::focusOutEvent(QFocusEvent *event)
{
    QDoubleSpinBox::focusOutEvent(event);
    m_focusAnim->stop();
    m_focusAnim->setStartValue(m_focusLevel);
    m_focusAnim->setEndValue(0.0);
    m_focusAnim->start();
}

void FluentDoubleSpinBox::mousePressEvent(QMouseEvent *event)
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

    QDoubleSpinBox::mousePressEvent(event);
}

void FluentDoubleSpinBox::mouseMoveEvent(QMouseEvent *event)
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

    QDoubleSpinBox::mouseMoveEvent(event);
}

void FluentDoubleSpinBox::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_pressedButton != ButtonPart::None) {
        m_pressedButton = ButtonPart::None;
        startStepperAnimation(m_hoverButton == ButtonPart::None ? 0.0 : 1.0);
        update();
    }
    QDoubleSpinBox::mouseReleaseEvent(event);
}

FluentDoubleSpinBox::ButtonPart FluentDoubleSpinBox::hitTestButton(const QPoint &pos) const
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
