#include "Fluent/FluentCalendarPicker.h"
#include "Fluent/datePicker/FluentCalendarPopup.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QAbstractSpinBox>
#include <QEasingCurve>
#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QVariantAnimation>

namespace Fluent {


FluentCalendarPicker::FluentCalendarPicker(QWidget *parent)
    : QDateEdit(parent)
{
    setCalendarPopup(false);
    setButtonSymbols(QAbstractSpinBox::NoButtons);
    setDisplayFormat("yyyy-MM-dd");
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

    connect(this, &QDateEdit::dateChanged, this, [this](const QDate &d) {
        if (m_popup) {
            m_popup->setDate(d);
        }
    });

    if (auto *lineEdit = findChild<QLineEdit *>()) {
        lineEdit->setFrame(false);
        lineEdit->installEventFilter(this);
        const auto m = Style::metrics();
        lineEdit->setTextMargins(m.paddingX, 0, m.iconAreaWidth, 0);
    }

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentCalendarPicker::applyTheme);
}

qreal FluentCalendarPicker::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentCalendarPicker::setHoverLevel(qreal value)
{
    m_hoverLevel = qBound(0.0, value, 1.0);
    update();
}

qreal FluentCalendarPicker::focusLevel() const
{
    return m_focusLevel;
}

void FluentCalendarPicker::setFocusLevel(qreal value)
{
    m_focusLevel = qBound(0.0, value, 1.0);
    update();
}

void FluentCalendarPicker::changeEvent(QEvent *event)
{
    QDateEdit::changeEvent(event);

    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentCalendarPicker::applyTheme()
{
    const auto &colors = ThemeManager::instance().colors();
    {
        const QString next = QString("QDateEdit { background: transparent; color: %1; }")
                                 .arg(colors.text.name());
        if (styleSheet() != next) {
            setStyleSheet(next);
        }
    }
    if (auto *lineEdit = findChild<QLineEdit *>()) {
        const auto m = Style::metrics();
        lineEdit->setTextMargins(m.paddingX, 0, m.iconAreaWidth, 0);

        const bool dark = ThemeManager::instance().themeMode() == ThemeManager::ThemeMode::Dark;
        const QColor textColor = isEnabled() ? colors.text : colors.disabledText;

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
        if (lineEdit->styleSheet() != next) {
            lineEdit->setStyleSheet(next);
        }

        // Keep caret accent while selection/text colors come from stylesheet.
        QPalette pal = lineEdit->palette();
        pal.setColor(QPalette::Text, colors.accent);
        pal.setColor(QPalette::Highlight, selectionBg);
        pal.setColor(QPalette::HighlightedText, colors.text);
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
        QColor placeholder = colors.subText;
        placeholder.setAlphaF(dark ? 0.55 : 0.60);
        pal.setColor(QPalette::PlaceholderText, placeholder);
#endif
        lineEdit->setPalette(pal);
    }

    if (m_popup) {
        m_popup->update();
    }
    update();
}

void FluentCalendarPicker::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const auto &colors = ThemeManager::instance().colors();

    const bool enabled = isEnabled();
    const bool expanded = isPopupVisible();

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF rect = QRectF(this->rect());
    Style::paintControlSurface(painter, rect, colors, m_hoverLevel, m_focusLevel, enabled, expanded);

    const auto m = Style::metrics();
    const QRect arrowRect(this->rect().right() - m.iconAreaWidth, this->rect().top(), m.iconAreaWidth, this->rect().height());

    QColor separator = colors.border;
    separator.setAlpha(80);
    painter.setPen(QPen(separator, 1));
    painter.drawLine(QPointF(arrowRect.left() + 0.5, arrowRect.top() + 8), QPointF(arrowRect.left() + 0.5, arrowRect.bottom() - 8));

    const QColor iconColor = enabled ? colors.subText : colors.disabledText;
    Style::drawChevronDown(painter, arrowRect.center(), iconColor, 8.0, 1.7);
}

void FluentCalendarPicker::mousePressEvent(QMouseEvent *event)
{
    if (!event || event->button() != Qt::LeftButton) {
        QDateEdit::mousePressEvent(event);
        return;
    }

    const auto m = Style::metrics();
    const QRect arrowRect(this->rect().right() - m.iconAreaWidth, this->rect().top(), m.iconAreaWidth, this->rect().height());
    if (arrowRect.contains(event->pos())) {
        QTimer::singleShot(0, this, [this]() {
            if (isPopupVisible()) {
                hidePopup();
            } else {
                showPopup();
            }
        });
        event->accept();
        return;
    }

    QDateEdit::mousePressEvent(event);
}

bool FluentCalendarPicker::eventFilter(QObject *watched, QEvent *event)
{
    if (!event) {
        return QDateEdit::eventFilter(watched, event);
    }

    auto *lineEdit = findChild<QLineEdit *>();
    if (watched == lineEdit) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me && me->button() == Qt::LeftButton) {
                // QDateEdit is a composite control; clicks land on the embedded QLineEdit.
                // Defer popup toggling to avoid Qt::Popup being immediately dismissed.
                QTimer::singleShot(0, this, [this]() {
                    if (isPopupVisible()) {
                        hidePopup();
                    } else {
                        showPopup();
                    }
                });
                me->accept();
                return true;
            }
        }

        if (event->type() == QEvent::KeyPress) {
            auto *ke = static_cast<QKeyEvent *>(event);
            if (!ke) {
                return QDateEdit::eventFilter(watched, event);
            }

            const bool altDown = (ke->key() == Qt::Key_Down && (ke->modifiers() & Qt::AltModifier));
            if (ke->key() == Qt::Key_F4 || altDown) {
                togglePopup();
                ke->accept();
                return true;
            }

            if (isPopupVisible() && ke->key() == Qt::Key_Escape) {
                hidePopup();
                ke->accept();
                return true;
            }
        }
    }

    return QDateEdit::eventFilter(watched, event);
}

void FluentCalendarPicker::keyPressEvent(QKeyEvent *event)
{
    if (!event) {
        QDateEdit::keyPressEvent(event);
        return;
    }

    const bool altDown = (event->key() == Qt::Key_Down && (event->modifiers() & Qt::AltModifier));
    if (event->key() == Qt::Key_F4 || altDown) {
        togglePopup();
        event->accept();
        return;
    }

    if (isPopupVisible() && event->key() == Qt::Key_Escape) {
        hidePopup();
        event->accept();
        return;
    }

    QDateEdit::keyPressEvent(event);
}

void FluentCalendarPicker::enterEvent(QEvent *event)
{
    QDateEdit::enterEvent(event);
    startHoverAnimation(1.0);
}

void FluentCalendarPicker::leaveEvent(QEvent *event)
{
    QDateEdit::leaveEvent(event);
    startHoverAnimation(0.0);
}

void FluentCalendarPicker::focusInEvent(QFocusEvent *event)
{
    QDateEdit::focusInEvent(event);
    startFocusAnimation(1.0);
}

void FluentCalendarPicker::focusOutEvent(QFocusEvent *event)
{
    QDateEdit::focusOutEvent(event);
    startFocusAnimation(0.0);
}

bool FluentCalendarPicker::isPopupVisible() const
{
    return m_popup && m_popup->isVisible();
}

void FluentCalendarPicker::togglePopup()
{
    if (isPopupVisible()) {
        hidePopup();
    } else {
        showPopup();
    }
    update();
}

void FluentCalendarPicker::showPopup()
{
    if (!m_popup) {
        m_popup = new FluentCalendarPopup(this);
        m_popup->setDate(date());
        connect(m_popup, &FluentCalendarPopup::datePicked, this, [this](const QDate &d) {
            setDate(d);
            setFocus();
        });
        connect(m_popup, &FluentCalendarPopup::dismissed, this, [this]() {
            update();
        });
    }

    m_popup->setAnchor(this);
    m_popup->setDate(date());
    m_popup->popup();
}

void FluentCalendarPicker::hidePopup()
{
    if (m_popup) {
        m_popup->dismiss();
    }
}

void FluentCalendarPicker::startHoverAnimation(qreal endValue)
{
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(endValue);
    m_hoverAnim->start();
}

void FluentCalendarPicker::startFocusAnimation(qreal endValue)
{
    m_focusAnim->stop();
    m_focusAnim->setStartValue(m_focusLevel);
    m_focusAnim->setEndValue(endValue);
    m_focusAnim->start();
}

} // namespace Fluent
