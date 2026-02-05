#include "Fluent/FluentDialog.h"
#include "Fluent/FluentFramePainter.h"
#include "Fluent/FluentResizeHelper.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentToolButton.h"

#include <QEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QShowEvent>
#include <QWindow>
#include <QHideEvent>

namespace Fluent {

namespace {
enum WindowGlyphType { GlyphMinimize = 0, GlyphMaximize = 1, GlyphRestore = 2, GlyphClose = 3 };

class ModalMaskOverlay final : public QWidget
{
public:
    explicit ModalMaskOverlay(QWidget *window, qreal opacity)
        : QWidget(window)
        , m_window(window)
        , m_opacity(qBound<qreal>(0.0, opacity, 1.0))
    {
        setObjectName("FluentModalMaskOverlay");
        setAttribute(Qt::WA_NoSystemBackground, false);
        setAttribute(Qt::WA_StyledBackground, false);
        setAttribute(Qt::WA_TransparentForMouseEvents, false);

        if (m_window) {
            m_window->installEventFilter(this);
            setGeometry(m_window->rect());
        }
    }

    void setOpacity(qreal opacity)
    {
        m_opacity = qBound<qreal>(0.0, opacity, 1.0);
        update();
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == m_window) {
            switch (event->type()) {
            case QEvent::Resize:
            case QEvent::Show:
            case QEvent::WindowStateChange:
                setGeometry(m_window->rect());
                raise();
                break;
            default:
                break;
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)
        QPainter p(this);
        if (!p.isActive()) {
            return;
        }

        QColor c(0, 0, 0);
        c.setAlphaF(m_opacity);
        p.fillRect(rect(), c);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event) {
            event->accept();
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event) {
            event->accept();
        }
    }

private:
    QPointer<QWidget> m_window;
    qreal m_opacity = 0.35;
};

static QString rgbaString(const QColor &c)
{
    return QString("rgba(%1,%2,%3,%4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(QString::number(c.alphaF(), 'f', 3));
}
}

FluentDialog::FluentDialog(QWidget *parent)
    : QDialog(parent)
    , m_border(this)
{
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setSizeGripEnabled(false);
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlag(Qt::FramelessWindowHint, true);
    setWindowFlag(Qt::NoDropShadowWindowHint, true);
    ensureTitleBar();
    updateWindowMarginsForState();
    // Islands style - removed QGraphicsDropShadowEffect for better performance
    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentDialog::applyTheme);
    m_border.syncFromTheme();

    // Enable dragging the frameless window from the top area.
    installEventFilter(this);

    // Resize helper is optional and disabled by default.
    updateResizeHelperState();
}

void FluentDialog::setMaskEnabled(bool enabled)
{
    m_maskEnabled = enabled;
    if (isVisible()) {
        if (m_maskEnabled) {
            ensureMaskOverlay();
        } else {
            teardownMaskOverlay();
        }
    }
}

bool FluentDialog::maskEnabled() const
{
    return m_maskEnabled;
}

void FluentDialog::setMaskOpacity(qreal opacity)
{
    m_maskOpacity = qBound<qreal>(0.0, opacity, 1.0);
    if (m_maskOverlay) {
        if (auto *overlay = dynamic_cast<ModalMaskOverlay *>(m_maskOverlay.data())) {
            overlay->setOpacity(m_maskOpacity);
        } else {
            m_maskOverlay->update();
        }
    }
}

qreal FluentDialog::maskOpacity() const
{
    return m_maskOpacity;
}

void FluentDialog::setFluentWindowButtons(WindowButtons buttons)
{
    m_windowButtons = buttons;

    if (m_minBtn) {
        m_minBtn->setVisible(m_windowButtons.testFlag(MinimizeButton));
    }
    if (m_maxBtn) {
        m_maxBtn->setVisible(m_windowButtons.testFlag(MaximizeButton));
    }
    if (m_closeBtn) {
        m_closeBtn->setVisible(m_windowButtons.testFlag(CloseButton));
    }
    if (m_windowButtonsHost) {
        // Don't use isVisible() here: before the dialog is shown, it will be false even if
        // the widget is logically visible, causing the host to stay hidden forever.
        const bool any = m_windowButtons.testFlag(MinimizeButton) || m_windowButtons.testFlag(MaximizeButton) || m_windowButtons.testFlag(CloseButton);
        m_windowButtonsHost->setVisible(any);
    }

    updateWindowMarginsForState();

    updateTitleBarContent();
    updateWindowControlIcons();
}

FluentDialog::WindowButtons FluentDialog::fluentWindowButtons() const
{
    return m_windowButtons;
}

void FluentDialog::setFluentResizeEnabled(bool enabled)
{
    m_resizeEnabled = enabled;
    updateResizeHelperState();
}

bool FluentDialog::fluentResizeEnabled() const
{
    return m_resizeEnabled;
}

void FluentDialog::setFluentResizeBorderWidth(int px)
{
    if (!m_resizeHelper) {
        updateResizeHelperState();
    }
    if (m_resizeHelper) {
        m_resizeHelper->setBorderWidth(px);
    }
}

int FluentDialog::fluentResizeBorderWidth() const
{
    return m_resizeHelper ? m_resizeHelper->borderWidth() : Style::windowMetrics().resizeBorder;
}

void FluentDialog::changeEvent(QEvent *event)
{
    QDialog::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    } else if (event->type() == QEvent::WindowTitleChange) {
        updateTitleBarContent();
    } else if (event->type() == QEvent::WindowStateChange) {
        updateWindowMarginsForState();
        updateWindowControlIcons();
    }
}

void FluentDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    // Play a single trace when the dialog first appears (if accent border is enabled).
    m_border.playInitialTraceOnce(0);

    if (m_maskEnabled) {
        ensureMaskOverlay();
    }
}

void FluentDialog::hideEvent(QHideEvent *event)
{
    teardownMaskOverlay();
    QDialog::hideEvent(event);
}

void FluentDialog::ensureMaskOverlay()
{
    QWidget *w = parentWidget() ? parentWidget()->window() : nullptr;
    if (!w) {
        return;
    }

    if (m_maskOverlay && m_maskOverlay->parentWidget() == w) {
        if (auto *overlay = dynamic_cast<ModalMaskOverlay *>(m_maskOverlay.data())) {
            overlay->setOpacity(m_maskOpacity);
        }
        m_maskOverlay->setGeometry(w->rect());
        m_maskOverlay->show();
        m_maskOverlay->raise();
        return;
    }

    teardownMaskOverlay();
    auto *overlay = new ModalMaskOverlay(w, m_maskOpacity);
    overlay->setGeometry(w->rect());
    overlay->show();
    overlay->raise();
    m_maskOverlay = overlay;
}

void FluentDialog::teardownMaskOverlay()
{
    if (!m_maskOverlay) {
        return;
    }

    m_maskOverlay->hide();
    m_maskOverlay->deleteLater();
    m_maskOverlay = nullptr;
}

void FluentDialog::applyTheme()
{
    // Skip animation while hidden to avoid painting on an unready surface.
    if (isVisible()) {
        m_border.onThemeChanged();
    } else {
        m_border.syncFromTheme();
    }

    // Ensure global QWidget background rules don't fill the transparent window.
    const QString next =
        "QDialog { background: transparent; }"
        "QDialog QWidget { background: transparent; }";
    if (styleSheet() != next) {
        setStyleSheet(next);
    }

    updateTitleBarContent();
    updateWindowControlIcons();
    update();
}

void FluentDialog::updateResizeHelperState()
{
    if (m_resizeEnabled) {
        if (!m_resizeHelper) {
            m_resizeHelper = new FluentResizeHelper(this);
            m_resizeHelper->setBorderWidth(Style::windowMetrics().resizeBorder);
        }
        m_resizeHelper->setEnabled(true);
    } else {
        if (m_resizeHelper) {
            m_resizeHelper->setEnabled(false);
        }
    }
}

bool FluentDialog::eventFilter(QObject *watched, QEvent *event)
{
    // Install this filter on newly added child widgets as well.
    if (event->type() == QEvent::ChildAdded) {
        if (auto *ce = static_cast<QChildEvent *>(event)) {
            if (ce->child() && ce->child()->isWidgetType()) {
                ce->child()->installEventFilter(this);
            }
        }
        // Keep title bar above content widgets added later.
        if (m_titleBarHost) {
            m_titleBarHost->raise();
        }
        return QDialog::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            const QPoint globalPos = me->globalPos();

            // Only start drag if press is in the top region of the dialog.
            QWidget *w = qobject_cast<QWidget *>(watched);
            const QPoint localInDialog = mapFromGlobal(globalPos);

            // Title bar drag region:
            // - Allow dragging when cursor is in the title bar area
            // - BUT do not steal events from interactive widgets (window buttons)
            const auto wm = Style::windowMetrics();
            const int titleH = m_titleBarHost ? m_titleBarHost->height() : wm.titleBarHeight;

            const bool titleBarVisible = m_titleBarHost && m_titleBarHost->isVisible() && titleH > 0;
            const QRect dragRect(0, 0, qMax(0, width()), titleBarVisible ? titleH : qMax(0, height()));
            if (!dragRect.contains(localInDialog)) {
                return QDialog::eventFilter(watched, event);
            }

            // If title bar is visible, keep the original behavior: only the title area starts drag,
            // and never steal from window buttons.
            if (titleBarVisible) {
                const QPoint inTitle = m_titleBarHost->mapFrom(this, localInDialog);
                QWidget *child = m_titleBarHost->childAt(inTitle);
                for (QWidget *p = child; p && p != m_titleBarHost; p = p->parentWidget()) {
                    if (p == m_windowButtonsHost) {
                        return QDialog::eventFilter(watched, event);
                    }
                    if (qobject_cast<QAbstractButton *>(p)) {
                        return QDialog::eventFilter(watched, event);
                    }
                }
            } else {
                // Title bar hidden: allow dragging the dialog background, but don't steal from
                // interactive widgets.
                QWidget *w = qobject_cast<QWidget *>(watched);
                for (QWidget *p = w; p && p != this; p = p->parentWidget()) {
                    if (qobject_cast<QAbstractButton *>(p)) {
                        return QDialog::eventFilter(watched, event);
                    }
                    if (p->focusPolicy() != Qt::NoFocus) {
                        return QDialog::eventFilter(watched, event);
                    }
                    if (auto *lbl = qobject_cast<QLabel *>(p)) {
                        if (lbl->textInteractionFlags().testFlag(Qt::TextSelectableByMouse)
                            || lbl->textInteractionFlags().testFlag(Qt::TextSelectableByKeyboard)) {
                            return QDialog::eventFilter(watched, event);
                        }
                    }
                }
            }

            m_dragging = true;
            m_dragOffset = globalPos - frameGeometry().topLeft();
            return true;
        }
    } else if (event->type() == QEvent::MouseMove) {
        if (m_dragging) {
            auto *me = static_cast<QMouseEvent *>(event);
            const QPoint globalPos = me->globalPos();
            move(globalPos - m_dragOffset);
            return true;
        }
    } else if (event->type() == QEvent::MouseButtonRelease) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton && m_dragging) {
            m_dragging = false;
            return true;
        }
    }

    return QDialog::eventFilter(watched, event);
}

void FluentDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    if (!m_titleBarHost) {
        return;
    }

    const auto wm = Style::windowMetrics();
    const int h = m_titleBarHost->height();
    m_titleBarHost->setGeometry(0, 0, qMax(0, width()), h);
    m_titleBarHost->raise();
    updateTitleBarContent();
}

void FluentDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    // Skip the very first paint attempt; on some platforms the backing store may not be ready
    // yet and creating QPainter(this) would trigger engine==0 warnings.
    if (!m_paintReady) {
        m_paintReady = true;
        QTimer::singleShot(0, this, [this]() {
            if (isVisible()) {
                update();
            }
        });
        return;
    }
    if (QWindow *wh = windowHandle()) {
        if (!wh->isExposed()) {
            return;
        }
    }
    QPainter p(this);
    if (!p.isActive()) {
        return;
    }

    const auto &colors = ThemeManager::instance().colors();

    const bool maximized = isMaximized() || isFullScreen();

    FluentFrameSpec frame;
    frame.radius = 10.0;
    frame.maximized = maximized;
    // Align strokes to device pixels to reduce jaggies and avoid edge clipping on HiDPI.
    {
        qreal dpr = devicePixelRatioF();
        if (dpr <= 0.0) {
            dpr = 1.0;
        }
        frame.borderInset = 0.5 / dpr;
    }
    m_border.applyToFrameSpec(frame, colors);
    paintFluentFrame(p, rect(), colors, frame);
}

void FluentDialog::ensureTitleBar()
{
    if (m_titleBarHost) {
        return;
    }

    m_titleBarHost = new QWidget(this);
    m_titleBarHost->setObjectName("FluentDialogTitleBarHost");
    m_titleBarHost->setAttribute(Qt::WA_StyledBackground, true);

    const auto wm = Style::windowMetrics();
    m_titleBarHost->setFixedHeight(wm.titleBarHeight);

    auto *layout = new QHBoxLayout(m_titleBarHost);
    layout->setContentsMargins(wm.titleBarPaddingX, wm.titleBarPaddingY, wm.titleBarPaddingX, wm.titleBarPaddingY);
    layout->setSpacing(8);

    layout->addStretch(1);

    m_titleLabel = new QLabel(m_titleBarHost);
    m_titleLabel->setObjectName("FluentDialogTitle");
    m_titleLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_titleLabel->setMinimumWidth(0);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_titleLabel, 0, Qt::AlignCenter);

    layout->addStretch(1);

    m_windowButtonsHost = new QWidget(m_titleBarHost);
    auto *btnLayout = new QHBoxLayout(m_windowButtonsHost);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(wm.windowButtonSpacing);

    m_minBtn = new FluentToolButton(m_windowButtonsHost);
    m_maxBtn = new FluentToolButton(m_windowButtonsHost);
    m_closeBtn = new FluentToolButton(m_windowButtonsHost);

    for (auto *b : {m_minBtn, m_maxBtn, m_closeBtn}) {
        b->setFixedSize(wm.windowButtonWidth, wm.windowButtonHeight);
        b->setAutoRaise(true);
        b->setFocusPolicy(Qt::NoFocus);
        b->setToolButtonStyle(Qt::ToolButtonIconOnly);
        b->setProperty("fluentWindowGlyph", -1);
    }

    m_minBtn->setProperty("fluentWindowGlyph", GlyphMinimize);
    m_maxBtn->setProperty("fluentWindowGlyph", GlyphMaximize);
    m_closeBtn->setProperty("fluentWindowGlyph", GlyphClose);

    m_minBtn->setToolTip(tr("Minimize"));
    m_maxBtn->setToolTip(tr("Maximize"));
    m_closeBtn->setToolTip(tr("Close"));

    connect(m_minBtn, &QToolButton::clicked, this, &QWidget::showMinimized);
    connect(m_maxBtn, &QToolButton::clicked, this, [this]() {
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
        updateWindowMarginsForState();
        updateWindowControlIcons();
    });
    connect(m_closeBtn, &QToolButton::clicked, this, &QWidget::close);

    btnLayout->addWidget(m_minBtn);
    btnLayout->addWidget(m_maxBtn);
    btnLayout->addWidget(m_closeBtn);

    layout->addWidget(m_windowButtonsHost, 0, Qt::AlignRight | Qt::AlignVCenter);

    // Apply initial visibility flags.
    setFluentWindowButtons(m_windowButtons);

    // Ensure title bar stays on top.
    m_titleBarHost->raise();
}

void FluentDialog::updateTitleBarContent()
{
    if (!m_titleBarHost || !m_titleLabel) {
        return;
    }

    const auto &colors = ThemeManager::instance().colors();

    const bool anyButtonsVisible = (m_windowButtons.testFlag(MinimizeButton)
        || m_windowButtons.testFlag(MaximizeButton)
        || m_windowButtons.testFlag(CloseButton));

    // Elide title to avoid pushing buttons.
    int rightW = 0;
    // NOTE: do not use isVisible() here; before the dialog is shown it will be false even if
    // the widgets are logically visible.
    if (m_windowButtonsHost && anyButtonsVisible) {
        rightW = qMax(m_windowButtonsHost->minimumWidth(), m_windowButtonsHost->sizeHint().width());
    }
    const int margins = Style::windowMetrics().titleBarPaddingX * 2 + 24;
    const int maxTitleWidth = qMax(0, m_titleBarHost->width() - rightW - margins);
    const QString titleText = fontMetrics().elidedText(windowTitle(), Qt::ElideRight, maxTitleWidth);

    m_titleLabel->setMaximumWidth(maxTitleWidth);
    m_titleLabel->setText(titleText);

    QColor divider = colors.border;
    divider.setAlpha(140);

    QColor topHighlight = QColor(255, 255, 255, (colors.background.lightnessF() < 0.5) ? 26 : 0);
    if (!anyButtonsVisible) {
        divider.setAlpha(0);
        topHighlight.setAlpha(0);
    }

    // Keep title bar subtle; dialog panel already has its own border.
    m_titleBarHost->setStyleSheet(QString(
        "#FluentDialogTitleBarHost { background: transparent; border-bottom: 1px solid %1; border-top: 1px solid %2; }"
        "#FluentDialogTitle { color: %3; font-weight: 600; }"
    ).arg(rgbaString(divider), rgbaString(topHighlight), colors.text.name()));
}

void FluentDialog::updateWindowControlIcons()
{
    if (m_maxBtn) {
        m_maxBtn->setProperty("fluentWindowGlyph", isMaximized() ? GlyphRestore : GlyphMaximize);
    }
    if (m_minBtn) {
        m_minBtn->update();
    }
    if (m_maxBtn) {
        m_maxBtn->update();
    }
    if (m_closeBtn) {
        m_closeBtn->update();
    }
}

void FluentDialog::updateWindowMarginsForState()
{
    const auto wm = Style::windowMetrics();
    const bool maximized = isMaximized() || isFullScreen();

    const bool anyButtonsVisible = (m_windowButtons.testFlag(MinimizeButton)
        || m_windowButtons.testFlag(MaximizeButton)
        || m_windowButtons.testFlag(CloseButton));

    const int titleH = anyButtonsVisible ? wm.titleBarHeight : 0;

    if (maximized) {
        // When maximized, remove transparent margins so the window fills the screen.
        setContentsMargins(0, titleH, 0, 0);
    } else {
        // No shadow: keep content flush, only reserve space for the custom title bar.
        setContentsMargins(0, titleH, 0, 0);
    }

    if (m_titleBarHost) {
        m_titleBarHost->setVisible(titleH > 0);
        m_titleBarHost->setFixedHeight(titleH);
        m_titleBarHost->setGeometry(0, 0, qMax(0, width()), titleH);
        m_titleBarHost->raise();
    }
}

} // namespace Fluent
