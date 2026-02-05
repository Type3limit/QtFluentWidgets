#include "Fluent/FluentTextEdit.h"
#include "Fluent/FluentScrollBar.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QEvent>
#include <QFocusEvent>
#include <QFrame>
#include <QPainter>
#include <QPalette>
#include <QResizeEvent>
#include <QShowEvent>
#include <QVariantAnimation>
#include <QWidget>

namespace Fluent {

namespace {

class BorderOverlay final : public QWidget
{
public:
    explicit BorderOverlay(FluentTextEdit *owner)
        : QWidget(owner)
        , m_owner(owner)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAutoFillBackground(false);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        if (!m_owner) {
            return;
        }

        QPainter p(this);
        if (!p.isActive()) {
            return;
        }

        const auto &colors = ThemeManager::instance().colors();
        const auto m = Style::metrics();

        // Mirror Style::paintControlSurface border/focus ring.
        const QColor stroke = m_owner->isEnabled() ? colors.border : Style::mix(colors.border, colors.disabledText, 0.35);

        p.setRenderHint(QPainter::Antialiasing, true);
        const QRectF r = QRectF(rect());

        // Pixel-align like Style::paintControlSurface.
        qreal dpr = 1.0;
        if (p.device()) {
            dpr = p.device()->devicePixelRatioF();
            if (dpr <= 0.0) {
                dpr = 1.0;
            }
        }
        const qreal px = 0.5 / dpr;
        const QRectF rr = r.adjusted(px, px, -px, -px);

        p.setPen(QPen(stroke, 1.0));
        p.setBrush(Qt::NoBrush);
        p.drawPath(Style::roundedRectPath(rr, m.radius));

        if (m_owner->isEnabled() && m_owner->focusLevel() > 0.0) {
            QColor focus = colors.focus;
            focus.setAlphaF(0.9 * qBound<qreal>(0.0, m_owner->focusLevel(), 1.0));
            p.setPen(QPen(focus, 2.0));
            p.drawPath(Style::roundedRectPath(rr.adjusted(1.0, 1.0, -1.0, -1.0), m.radius - 1));
        }
    }

private:
    FluentTextEdit *m_owner = nullptr;
};

} // namespace

FluentTextEdit::FluentTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
    setFrameStyle(QFrame::NoFrame);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(false);

    const auto metrics = Style::metrics();
    setViewportMargins(metrics.paddingX, metrics.paddingY, metrics.paddingX, metrics.paddingY);
    setMinimumHeight(metrics.height);

    setVerticalScrollBar(new FluentScrollBar(Qt::Vertical, this));
    setHorizontalScrollBar(new FluentScrollBar(Qt::Horizontal, this));

    if (viewport()) {
        viewport()->setMouseTracking(true);
        viewport()->setAutoFillBackground(false);
        viewport()->setAttribute(Qt::WA_StyledBackground, true);
        viewport()->installEventFilter(this);
    }

    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(150);
    m_hoverAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverLevel = qBound<qreal>(0.0, value.toReal(), 1.0);
        update();
        if (m_borderOverlay) {
            m_borderOverlay->update();
        }
    });

    m_focusAnim = new QVariantAnimation(this);
    m_focusAnim->setDuration(200);
    m_focusAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_focusAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_focusLevel = qBound<qreal>(0.0, value.toReal(), 1.0);
        update();
        if (m_borderOverlay) {
            m_borderOverlay->update();
        }
    });

    m_borderOverlay = new BorderOverlay(this);
    updateBorderOverlayGeometry();
    // Defer showing until the widget is shown to avoid early paints.
    m_borderOverlay->hide();

    // Defer theme application until the widget is shown to avoid triggering early viewport paints.
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentTextEdit::applyTheme);
}

QSize FluentTextEdit::sizeHint() const
{
    return QSize(200, 80);
}

qreal FluentTextEdit::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentTextEdit::setHoverLevel(qreal value)
{
    m_hoverLevel = qBound<qreal>(0.0, value, 1.0);
    update();
}

qreal FluentTextEdit::focusLevel() const
{
    return m_focusLevel;
}

void FluentTextEdit::setFocusLevel(qreal value)
{
    m_focusLevel = qBound<qreal>(0.0, value, 1.0);
    update();
}

void FluentTextEdit::changeEvent(QEvent *event)
{
    QTextEdit::changeEvent(event);
    if (event && event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentTextEdit::applyTheme()
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
        "QTextEdit { background: transparent; color: %1; border: none; selection-background-color: %2; selection-color: %3; }"
        "QTextEdit QAbstractScrollArea::viewport { background: transparent; }"
    ).arg(textColor.name()).arg(selectionBgStr).arg(colors.text.name());

    if (styleSheet() != next) {
        setStyleSheet(next);
    }

    QPalette pal = palette();
    // Keep caret accent like FluentLineEdit; text color is controlled via stylesheet.
    pal.setColor(QPalette::Text, colors.accent);
    pal.setColor(QPalette::Highlight, selectionBg);
    pal.setColor(QPalette::HighlightedText, colors.text);
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    QColor placeholder = colors.subText;
    placeholder.setAlphaF(dark ? 0.55 : 0.60);
    pal.setColor(QPalette::PlaceholderText, placeholder);
#endif
    setPalette(pal);

    if (isVisible()) {
        update();
        if (m_borderOverlay) {
            m_borderOverlay->update();
        }
    }
}

void FluentTextEdit::paintEvent(QPaintEvent *event)
{
    const auto &colors = ThemeManager::instance().colors();

    QPainter painter(this);
    if (!painter.isActive()) {
        QTextEdit::paintEvent(event);
        return;
    }

    // Ensure our surface is drawn behind QTextEdit's internals.
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    Style::paintControlSurface(
        painter,
        QRectF(rect()),
        colors,
        m_hoverLevel,
        m_focusLevel,
        isEnabled(),
        false);

    // Workaround: keep viewport marked as "in paint" while QTextEdit paints it.
    struct ScopedViewportPaintFlag {
        QWidget *vp = nullptr;
        bool prev = false;
        explicit ScopedViewportPaintFlag(QWidget *v)
            : vp(v)
        {
            if (!vp) {
                return;
            }
            prev = vp->testAttribute(Qt::WA_WState_InPaintEvent);
            if (!prev) {
                vp->setAttribute(Qt::WA_WState_InPaintEvent, true);
            }
        }
        ~ScopedViewportPaintFlag()
        {
            if (vp && !prev) {
                vp->setAttribute(Qt::WA_WState_InPaintEvent, false);
            }
        }
    } scopedViewportPaint(viewport());

    QTextEdit::paintEvent(event);
}

bool FluentTextEdit::viewportEvent(QEvent *event)
{
    return QTextEdit::viewportEvent(event);
}

void FluentTextEdit::resizeEvent(QResizeEvent *event)
{
    QTextEdit::resizeEvent(event);
    updateBorderOverlayGeometry();
}

void FluentTextEdit::showEvent(QShowEvent *event)
{
    QTextEdit::showEvent(event);

    if (!m_themeAppliedOnce) {
        m_themeAppliedOnce = true;
        applyTheme();
    }

    if (m_borderOverlay) {
        updateBorderOverlayGeometry();
        m_borderOverlay->raise();
        m_borderOverlay->show();
        m_borderOverlay->update();
    }
    update();
}

void FluentTextEdit::updateBorderOverlayGeometry()
{
    if (!m_borderOverlay) {
        return;
    }
    m_borderOverlay->setGeometry(rect());
}

bool FluentTextEdit::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == viewport() && event) {
        if (event->type() == QEvent::Enter || event->type() == QEvent::HoverEnter) {
            startHoverAnimation(1.0);
        } else if (event->type() == QEvent::Leave || event->type() == QEvent::HoverLeave) {
            startHoverAnimation(0.0);
        }
    }

    return QTextEdit::eventFilter(watched, event);
}

void FluentTextEdit::enterEvent(QEvent *event)
{
    QTextEdit::enterEvent(event);
    startHoverAnimation(1.0);
}

void FluentTextEdit::leaveEvent(QEvent *event)
{
    QTextEdit::leaveEvent(event);
    startHoverAnimation(0.0);
}

void FluentTextEdit::focusInEvent(QFocusEvent *event)
{
    QTextEdit::focusInEvent(event);
    startFocusAnimation(1.0);
}

void FluentTextEdit::focusOutEvent(QFocusEvent *event)
{
    QTextEdit::focusOutEvent(event);
    startFocusAnimation(0.0);
}

void FluentTextEdit::startHoverAnimation(qreal endValue)
{
    if (!m_hoverAnim) {
        return;
    }
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(endValue);
    m_hoverAnim->start();
}

void FluentTextEdit::startFocusAnimation(qreal endValue)
{
    if (!m_focusAnim) {
        return;
    }
    m_focusAnim->stop();
    m_focusAnim->setStartValue(m_focusLevel);
    m_focusAnim->setEndValue(endValue);
    m_focusAnim->start();
}

} // namespace Fluent
