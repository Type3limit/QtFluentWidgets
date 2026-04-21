#include "Fluent/FluentKeySequenceEdit.h"

#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QEvent>
#include <QEasingCurve>
#include <QFocusEvent>
#include <QLineEdit>
#include <QPainter>
#include <QPalette>
#include <QVariantAnimation>

namespace Fluent {

namespace {

QString rgbaColorString(const QColor &color, qreal alpha)
{
    QColor rgba(color);
    rgba.setAlphaF(qBound(0.0, alpha, 1.0));

    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(rgba.red())
        .arg(rgba.green())
        .arg(rgba.blue())
        .arg(QString::number(rgba.alphaF(), 'f', 3));
}

} // namespace

FluentKeySequenceEdit::FluentKeySequenceEdit(QWidget *parent)
    : QKeySequenceEdit(parent)
{
    initializeControl();
}

FluentKeySequenceEdit::FluentKeySequenceEdit(const QKeySequence &keySequence, QWidget *parent)
    : QKeySequenceEdit(keySequence, parent)
{
    initializeControl();
}

qreal FluentKeySequenceEdit::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentKeySequenceEdit::setHoverLevel(qreal value)
{
    m_hoverLevel = qBound(0.0, value, 1.0);
    update();
}

qreal FluentKeySequenceEdit::focusLevel() const
{
    return m_focusLevel;
}

void FluentKeySequenceEdit::setFocusLevel(qreal value)
{
    m_focusLevel = qBound(0.0, value, 1.0);
    update();
}

QSize FluentKeySequenceEdit::sizeHint() const
{
    QSize size = QKeySequenceEdit::sizeHint();
    size.setHeight(qMax(size.height(), minimumHeight()));
    return size;
}

QSize FluentKeySequenceEdit::minimumSizeHint() const
{
    QSize size = QKeySequenceEdit::minimumSizeHint();
    size.setHeight(qMax(size.height(), minimumHeight()));
    return size;
}

bool FluentKeySequenceEdit::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_editor && event != nullptr) {
        switch (event->type()) {
        case QEvent::Enter:
            startHoverAnimation(1.0);
            break;
        case QEvent::Leave:
            startHoverAnimation(0.0);
            break;
        case QEvent::FocusIn:
            startFocusAnimation(1.0);
            break;
        case QEvent::FocusOut:
            updateFocusState();
            break;
        case QEvent::EnabledChange:
            applyTheme();
            break;
        default:
            break;
        }
    }

    return QKeySequenceEdit::eventFilter(watched, event);
}

void FluentKeySequenceEdit::changeEvent(QEvent *event)
{
    QKeySequenceEdit::changeEvent(event);

    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentKeySequenceEdit::paintEvent(QPaintEvent *event)
{
    ensureEditor();

    const auto &colors = ThemeManager::instance().colors();

    QPainter painter(this);
    if (!painter.isActive()) {
        QKeySequenceEdit::paintEvent(event);
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

    QKeySequenceEdit::paintEvent(event);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void FluentKeySequenceEdit::enterEvent(QEnterEvent *event)
#else
void FluentKeySequenceEdit::enterEvent(QEvent *event)
#endif
{
    QKeySequenceEdit::enterEvent(event);
    startHoverAnimation(1.0);
}

void FluentKeySequenceEdit::leaveEvent(QEvent *event)
{
    QKeySequenceEdit::leaveEvent(event);
    if (m_editor == nullptr || !m_editor->underMouse()) {
        startHoverAnimation(0.0);
    }
}

void FluentKeySequenceEdit::focusInEvent(QFocusEvent *event)
{
    QKeySequenceEdit::focusInEvent(event);
    startFocusAnimation(1.0);
}

void FluentKeySequenceEdit::focusOutEvent(QFocusEvent *event)
{
    QKeySequenceEdit::focusOutEvent(event);
    updateFocusState();
}

void FluentKeySequenceEdit::initializeControl()
{
    setMinimumHeight(Style::metrics().height);
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

    ensureEditor();
    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentKeySequenceEdit::applyTheme);
}

void FluentKeySequenceEdit::ensureEditor()
{
    auto *editor = findChild<QLineEdit *>();
    if (editor == nullptr || editor == m_editor) {
        return;
    }

    if (m_editor != nullptr) {
        m_editor->removeEventFilter(this);
    }

    m_editor = editor;
    m_editor->installEventFilter(this);
    m_editor->setFrame(false);
    m_editor->setAttribute(Qt::WA_TranslucentBackground, true);
}

void FluentKeySequenceEdit::applyTheme()
{
    ensureEditor();

    const auto &colors = ThemeManager::instance().colors();
    const bool dark = ThemeManager::instance().themeMode() == ThemeManager::ThemeMode::Dark;
    QColor selectionBg = colors.accent;
    selectionBg.setAlphaF(dark ? 0.35 : 0.22);

    const QString hostStyle = QStringLiteral(
        "QKeySequenceEdit { background: transparent; border: none; padding: 0px; }");
    if (styleSheet() != hostStyle) {
        setStyleSheet(hostStyle);
    }

    if (m_editor != nullptr) {
        const QColor textColor = isEnabled() ? colors.text : colors.disabledText;
        const QString editorStyle = QString(
            "QLineEdit {"
            "  background: transparent;"
            "  color: %1;"
            "  border: none;"
            "  padding: 0px %2px;"
            "  selection-background-color: %3;"
            "  selection-color: %4;"
            "}"
            "QLineEdit:disabled {"
            "  color: %5;"
            "}")
            .arg(textColor.name())
            .arg(Style::metrics().paddingX)
            .arg(rgbaColorString(selectionBg, selectionBg.alphaF()))
            .arg(colors.text.name())
            .arg(colors.disabledText.name());
        if (m_editor->styleSheet() != editorStyle) {
            m_editor->setStyleSheet(editorStyle);
        }

        QPalette palette = m_editor->palette();
        palette.setColor(QPalette::Text, textColor);
    palette.setColor(QPalette::Highlight, selectionBg);
        palette.setColor(QPalette::HighlightedText, colors.text);
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
        QColor placeholder = colors.subText;
        placeholder.setAlphaF(dark ? 0.55 : 0.60);
        palette.setColor(QPalette::PlaceholderText, placeholder);
#endif
        m_editor->setPalette(palette);
    }

    updateFocusState();
    update();
}

void FluentKeySequenceEdit::startHoverAnimation(qreal endValue)
{
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(endValue);
    m_hoverAnim->start();
}

void FluentKeySequenceEdit::startFocusAnimation(qreal endValue)
{
    m_focusAnim->stop();
    m_focusAnim->setStartValue(m_focusLevel);
    m_focusAnim->setEndValue(endValue);
    m_focusAnim->start();
}

void FluentKeySequenceEdit::updateFocusState()
{
    const bool focused = hasFocus() || (m_editor != nullptr && m_editor->hasFocus());
    startFocusAnimation(focused ? 1.0 : 0.0);
}

} // namespace Fluent