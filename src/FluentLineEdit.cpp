#include "Fluent/FluentLineEdit.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QEvent>
#include <QPainter>
#include <QVariantAnimation>

namespace Fluent {

FluentLineEdit::FluentLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    setFrame(false);
    const auto m = Style::metrics();
    setTextMargins(m.paddingX, 0, m.paddingX, 0);
    setMinimumHeight(m.height);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);

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

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentLineEdit::applyTheme);
}

FluentLineEdit::FluentLineEdit(const QString &text, QWidget *parent)
    : QLineEdit(text, parent)
{
    setFrame(false);
    const auto m = Style::metrics();
    setTextMargins(m.paddingX, 0, m.paddingX, 0);
    setMinimumHeight(m.height);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);

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

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentLineEdit::applyTheme);
}

qreal FluentLineEdit::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentLineEdit::setHoverLevel(qreal value)
{
    m_hoverLevel = qBound(0.0, value, 1.0);
    update();
}

qreal FluentLineEdit::focusLevel() const
{
    return m_focusLevel;
}

void FluentLineEdit::setFocusLevel(qreal value)
{
    m_focusLevel = qBound(0.0, value, 1.0);
    update();
}

void FluentLineEdit::changeEvent(QEvent *event)
{
    QLineEdit::changeEvent(event);

    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentLineEdit::applyTheme()
{
    const auto &colors = ThemeManager::instance().colors();
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
    if (styleSheet() != next) {
        setStyleSheet(next);
    }

    // Try to make caret follow accent while keeping text color from stylesheet.
    QPalette pal = palette();
    pal.setColor(QPalette::Text, colors.accent);
    pal.setColor(QPalette::Highlight, selectionBg);
    pal.setColor(QPalette::HighlightedText, colors.text);
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    // WaterMark/placeholder should stay readable; do NOT follow accent.
    QColor placeholder = colors.subText;
    placeholder.setAlphaF(dark ? 0.55 : 0.60);
    pal.setColor(QPalette::PlaceholderText, placeholder);
#endif
    setPalette(pal);
    update();
}

void FluentLineEdit::paintEvent(QPaintEvent *event)
{
    const auto &colors = ThemeManager::instance().colors();

    QPainter painter(this);
    if (!painter.isActive()) {
        QLineEdit::paintEvent(event);
        return;
    }
    Style::paintControlSurface(
        painter,
        QRectF(this->rect()),
        colors,
        m_hoverLevel,
        m_focusLevel,
        isEnabled(),
        false);

    QLineEdit::paintEvent(event);
}

void FluentLineEdit::enterEvent(QEvent *event)
{
    QLineEdit::enterEvent(event);
    startHoverAnimation(1.0);
}

void FluentLineEdit::leaveEvent(QEvent *event)
{
    QLineEdit::leaveEvent(event);
    startHoverAnimation(0.0);
}

void FluentLineEdit::focusInEvent(QFocusEvent *event)
{
    QLineEdit::focusInEvent(event);
    startFocusAnimation(1.0);
}

void FluentLineEdit::focusOutEvent(QFocusEvent *event)
{
    QLineEdit::focusOutEvent(event);
    startFocusAnimation(0.0);
}

void FluentLineEdit::startHoverAnimation(qreal endValue)
{
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(endValue);
    m_hoverAnim->start();
}

void FluentLineEdit::startFocusAnimation(qreal endValue)
{
    m_focusAnim->stop();
    m_focusAnim->setStartValue(m_focusLevel);
    m_focusAnim->setEndValue(endValue);
    m_focusAnim->start();
}

} // namespace Fluent
