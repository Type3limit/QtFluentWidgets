#include "Fluent/FluentMainWindow.h"

#include "Fluent/FluentFramePainter.h"
#include "Fluent/FluentMenuBar.h"
#include "Fluent/FluentResizeHelper.h"
#include "Fluent/FluentStatusBar.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentToolBar.h"
#include "Fluent/FluentToolButton.h"

#include <QAbstractButton>
#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRegion>
#include <QShowEvent>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>
#include <QVariantAnimation>

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

namespace Fluent {

namespace {
enum WindowGlyphType { GlyphMinimize = 0, GlyphMaximize = 1, GlyphRestore = 2, GlyphClose = 3 };

// Window frame radius matching the Windows 11 DWM DWMWCP_ROUND corner clip (8 DIPs).
static constexpr qreal kWindowFrameRadius = 8.0;

static QString rgbaString(const QColor &c)
{
    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(c.red())
        .arg(c.green())
        .arg(c.blue())
        .arg(c.alpha());
}

// ---------------------------------------------------------------------------
// WindowBorderOverlay – a transparent child of the MainWindow that covers the
// ENTIRE content area (title bar + central widget).  It paints the accent
// border stroke + trace animation on top of everything so it is never covered
// by opaque child widgets.
// ---------------------------------------------------------------------------
class WindowBorderOverlay final : public QWidget
{
public:
    explicit WindowBorderOverlay(QWidget *parent)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAutoFillBackground(false);
    }

    void setBorderEffect(const FluentBorderEffect *border) { m_border = border; }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)

        const QWidget *w = window();
        if (w && !w->property("_fluentPaintReady").toBool()) {
            return;
        }
        if (w) {
            if (QWindow *wh = w->windowHandle()) {
                if (!wh->isExposed()) {
                    return;
                }
            }
        }

        QPainter p(this);
        if (!p.isActive()) {
            return;
        }

        const auto &colors = ThemeManager::instance().colors();
        const bool maximized = w && (w->isMaximized() || w->isFullScreen());

        FluentFrameSpec frame;
        frame.radius = kWindowFrameRadius;
        frame.maximized = maximized;
        {
            qreal dpr = devicePixelRatioF();
            if (dpr <= 0.0) {
                dpr = 1.0;
            }
            frame.borderInset = 0.5 / dpr;
        }

        if (m_border) {
            m_border->applyToFrameSpec(frame, colors);
        }

        const qreal radius = maximized ? 0.0 : frame.radius;
        QRectF panelRect(rect());
        panelRect.adjust(frame.borderInset, frame.borderInset, -frame.borderInset, -frame.borderInset);

        const QColor border = frame.borderColorOverride.isValid()
            ? frame.borderColorOverride
            : (frame.accentBorderEnabled ? colors.accent : colors.border);

        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(QPen(border, frame.borderWidth));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(panelRect, radius, radius);

        if (frame.traceEnabled) {
            Style::paintTraceBorder(p, panelRect, radius, frame.traceColor, frame.traceT, frame.borderWidth, 0.0);
        }
    }

private:
    const FluentBorderEffect *m_border = nullptr;
};

// ---------------------------------------------------------------------------
// WindowFrameHost – a simple container that sits inside the MainWindow as the
// central widget.  It only paints the Fluent surface fill (rounded rect).
// The accent border is painted by WindowBorderOverlay (a sibling widget that
// covers the entire window content area, including the title bar).
// ---------------------------------------------------------------------------
class WindowFrameHost final : public QWidget
{
public:
    explicit WindowFrameHost(QWidget *parent)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_StyledBackground, false);
        setAutoFillBackground(false);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)

        const QWidget *w = window();
        if (w && !w->property("_fluentPaintReady").toBool()) {
            return;
        }
        if (w) {
            if (QWindow *wh = w->windowHandle()) {
                if (!wh->isExposed()) {
                    return;
                }
            }
        }

        QPainter p(this);
        if (!p.isActive()) {
            return;
        }

        const auto &colors = ThemeManager::instance().colors();

        // Simple opaque fill — no rounded rect needed here because the border
        // overlay and DWM handle the visual rounding.
        p.fillRect(rect(), colors.surface);
    }
};

} // namespace

FluentMainWindow::FluentMainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setWindowFlag(Qt::NoDropShadowWindowHint, true);

    // Guard against early paints before the backing store is ready.
    setProperty("_fluentPaintReady", false);

    // FluentMainWindow intentionally does not use QMainWindow's tool/status bars.
    QMainWindow::setStatusBar(nullptr);

    // Keep overlay/title bar above widgets added later (e.g. setCentralWidget after construction).
    installEventFilter(this);

    // Create the frame host that paints the surface fill for the central widget area.
    m_frameHost = new WindowFrameHost(this);
    m_frameHost->setObjectName("FluentMainWindowFrameHost");

    // Create the border overlay as a direct child of the MainWindow so it
    // covers the ENTIRE content area (title bar + central widget).
    m_borderOverlay = new WindowBorderOverlay(this);
    m_borderOverlay->setObjectName("FluentMainWindowBorderOverlay");
    if (auto *overlay = dynamic_cast<WindowBorderOverlay *>(m_borderOverlay)) {
        overlay->setBorderEffect(&m_border);
    }

    m_border.syncFromTheme();
    m_border.setRequestUpdate([this]() {
        updateFrameHost();
    });

    ensureTitleBar();
    setFluentTitleBarEnabled(true);
    // Frameless windows need manual resize support; keep it on by default.
    setFluentResizeEnabled(true);

    // Keep the global application stylesheet always in sync with the current theme.
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentMainWindow::applyThemeToApplication);
    applyThemeToApplication();

    // Decide initial inset/overlay visibility.
    updateTitleBarContent();

    // Initialize border state.
    syncBorderVisualState();
}

void FluentMainWindow::syncBorderVisualState()
{
    updateFrameHost();
    updateTitleBarContent();
    if (m_titleBarHost) {
        m_titleBarHost->update();
    }
}

void FluentMainWindow::updateFrameHost()
{
    if (m_frameHost) {
        m_frameHost->update();
    }
    if (m_borderOverlay) {
        // Position the overlay to cover the entire content area (inside the 1px margins).
        const int inset = (isMaximized() || isFullScreen()) ? 0 : 1;
        const QRect overlayRect(inset, inset, width() - 2 * inset, height() - 2 * inset);
        if (m_borderOverlay->geometry() != overlayRect) {
            m_borderOverlay->setGeometry(overlayRect);
        }
        m_borderOverlay->raise();
        m_borderOverlay->update();
    }
}

void FluentMainWindow::addToolBar(QToolBar *toolbar)
{
    if (toolbar) {
        toolbar->hide();
        toolbar->deleteLater();
    }
}

void FluentMainWindow::addToolBar(Qt::ToolBarArea area, QToolBar *toolbar)
{
    Q_UNUSED(area);
    addToolBar(toolbar);
}

void FluentMainWindow::insertToolBar(QToolBar *before, QToolBar *toolbar)
{
    Q_UNUSED(before);
    addToolBar(toolbar);
}

void FluentMainWindow::setStatusBar(QStatusBar *statusbar)
{
    if (statusbar) {
        statusbar->hide();
        statusbar->deleteLater();
    }
    QMainWindow::setStatusBar(nullptr);
}

void FluentMainWindow::setCentralWidget(QWidget *widget)
{
    if (widget == m_userCentralWidget) {
        return;
    }

    ensureFrameHostAsCentral();

    if (!m_frameHost) {
        m_userCentralWidget = widget;
        QMainWindow::setCentralWidget(widget);
        return;
    }

    // Remove old
    if (m_userCentralWidget && m_userCentralWidget->parent() == m_frameHost) {
        m_userCentralWidget->setParent(nullptr);
    }
    m_userCentralWidget = widget;

    auto *layout = qobject_cast<QVBoxLayout *>(m_frameHost->layout());
    if (!layout) {
        layout = new QVBoxLayout(m_frameHost);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
    }

    while (layout->count() > 0) {
        QLayoutItem *it = layout->takeAt(0);
        if (it) {
            if (QWidget *w = it->widget()) {
                w->setParent(nullptr);
            }
            delete it;
        }
    }

    if (widget) {
        widget->setParent(m_frameHost);
        layout->addWidget(widget);
    }


    updateFrameHost();
}

void FluentMainWindow::setFluentTitleBarTitle(const QString &title)
{
    m_titleOverride = title;
    m_hasTitleOverride = true;
    updateTitleBarContent();
}

void FluentMainWindow::clearFluentTitleBarTitle()
{
    m_titleOverride.clear();
    m_hasTitleOverride = false;
    updateTitleBarContent();
}

QString FluentMainWindow::fluentTitleBarTitle() const
{
    return m_hasTitleOverride ? m_titleOverride : windowTitle();
}

void FluentMainWindow::setFluentTitleBarIcon(const QIcon &icon)
{
    m_iconOverride = icon;
    m_hasIconOverride = true;
    updateTitleBarContent();
}

void FluentMainWindow::clearFluentTitleBarIcon()
{
    m_iconOverride = QIcon();
    m_hasIconOverride = false;
    updateTitleBarContent();
}

QIcon FluentMainWindow::fluentTitleBarIcon() const
{
    if (m_hasIconOverride) {
        return m_iconOverride;
    }
    return windowIcon();
}

void FluentMainWindow::setFluentTitleBarCenterWidget(QWidget *widget)
{
    ensureTitleBar();

    if (widget == m_centerCustomWidget) {
        return;
    }

    if (m_centerCustomWidget && m_centerCustomWidget->parentWidget() == m_centerHost) {
        m_centerCustomWidget->setParent(nullptr);
    }
    m_centerCustomWidget = widget;

    if (!m_centerHost) {
        return;
    }

    auto *layout = qobject_cast<QHBoxLayout *>(m_centerHost->layout());
    if (!layout) {
        layout = new QHBoxLayout(m_centerHost);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
    }

    while (layout->count() > 0) {
        QLayoutItem *it = layout->takeAt(0);
        if (it) {
            if (QWidget *w = it->widget()) {
                w->setParent(nullptr);
            }
            delete it;
        }
    }

    if (m_centerCustomWidget) {
        m_titleLabel->setVisible(false);
        m_centerCustomWidget->setParent(m_centerHost);
        layout->addWidget(m_centerCustomWidget);
    } else {
        m_titleLabel->setVisible(true);
        m_titleLabel->setParent(m_centerHost);
        layout->addWidget(m_titleLabel);
    }

    updateTitleBarContent();
}

QWidget *FluentMainWindow::fluentTitleBarCenterWidget() const
{
    return m_centerCustomWidget;
}

void FluentMainWindow::setFluentTitleBarLeftWidget(QWidget *widget)
{
    ensureTitleBar();

    if (widget == m_leftCustomWidget) {
        return;
    }

    if (m_leftCustomWidget && m_leftSlotHost && m_leftCustomWidget->parentWidget() == m_leftSlotHost) {
        m_leftCustomWidget->setParent(nullptr);
    }
    m_leftCustomWidget = widget;

    if (!m_leftSlotHost) {
        return;
    }

    auto *layout = qobject_cast<QHBoxLayout *>(m_leftSlotHost->layout());
    if (!layout) {
        layout = new QHBoxLayout(m_leftSlotHost);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(8);
    }

    while (layout->count() > 0) {
        QLayoutItem *it = layout->takeAt(0);
        if (it) {
            if (QWidget *w = it->widget()) {
                w->setParent(nullptr);
            }
            delete it;
        }
    }

    if (m_leftCustomWidget) {
        m_leftCustomWidget->setParent(m_leftSlotHost);
        layout->addWidget(m_leftCustomWidget);
        m_leftSlotHost->show();
    } else {
        m_leftSlotHost->hide();
    }

    if (m_leftHost && m_leftHost->layout()) {
        m_leftHost->layout()->activate();
        m_leftHost->setMinimumWidth(m_leftHost->sizeHint().width());
    }

    updateTitleBarContent();
}

QWidget *FluentMainWindow::fluentTitleBarLeftWidget() const
{
    return m_leftCustomWidget;
}

void FluentMainWindow::setFluentTitleBarRightWidget(QWidget *widget)
{
    ensureTitleBar();

    if (widget == m_rightCustomWidget) {
        return;
    }

    if (m_rightCustomWidget && m_rightSlotHost && m_rightCustomWidget->parentWidget() == m_rightSlotHost) {
        m_rightCustomWidget->setParent(nullptr);
    }
    m_rightCustomWidget = widget;

    if (!m_rightSlotHost) {
        return;
    }

    auto *layout = qobject_cast<QHBoxLayout *>(m_rightSlotHost->layout());
    if (!layout) {
        layout = new QHBoxLayout(m_rightSlotHost);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(8);
    }

    while (layout->count() > 0) {
        QLayoutItem *it = layout->takeAt(0);
        if (it) {
            if (QWidget *w = it->widget()) {
                w->setParent(nullptr);
            }
            delete it;
        }
    }

    if (m_rightCustomWidget) {
        m_rightCustomWidget->setParent(m_rightSlotHost);
        layout->addWidget(m_rightCustomWidget);
        m_rightSlotHost->show();
    } else {
        m_rightSlotHost->hide();
    }

    if (m_rightHost) {
        const bool anyButtons = m_windowButtons.testFlag(MinimizeButton) || m_windowButtons.testFlag(MaximizeButton) || m_windowButtons.testFlag(CloseButton);
        m_rightHost->setVisible(anyButtons || (m_rightCustomWidget != nullptr));
    }

    if (m_rightHost && m_rightHost->layout()) {
        m_rightHost->layout()->activate();
        m_rightHost->setMinimumWidth(m_rightHost->sizeHint().width());
    }

    updateTitleBarContent();
}

QWidget *FluentMainWindow::fluentTitleBarRightWidget() const
{
    return m_rightCustomWidget;
}

void FluentMainWindow::ensureFrameHostAsCentral()
{
    if (m_frameHost && m_frameHost->parent() == this) {
        // Already installed as central widget? Check.
        if (QMainWindow::centralWidget() == m_frameHost) {
            return;
        }
    }

    if (!m_frameHost) {
        m_frameHost = new WindowFrameHost(this);
        m_frameHost->setObjectName("FluentMainWindowFrameHost");
    }

    auto *layout = new QVBoxLayout(m_frameHost);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QMainWindow::setCentralWidget(m_frameHost);
}

void FluentMainWindow::setFluentWindowButtons(WindowButtons buttons)
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
        const bool any = m_windowButtons.testFlag(MinimizeButton) || m_windowButtons.testFlag(MaximizeButton) || m_windowButtons.testFlag(CloseButton);
        m_windowButtonsHost->setVisible(any);
    }

    if (m_rightHost) {
        const bool anyButtons = m_windowButtons.testFlag(MinimizeButton) || m_windowButtons.testFlag(MaximizeButton) || m_windowButtons.testFlag(CloseButton);
        const bool anyRight = (m_rightCustomWidget != nullptr);
        m_rightHost->setVisible(anyButtons || anyRight);
    }

    updateTitleBarContent();
    updateWindowControlIcons();
}

FluentMainWindow::WindowButtons FluentMainWindow::fluentWindowButtons() const
{
    return m_windowButtons;
}

void FluentMainWindow::setFluentResizeEnabled(bool enabled)
{
    m_resizeEnabled = enabled;
    updateResizeHelperState();
}

bool FluentMainWindow::fluentResizeEnabled() const
{
    return m_resizeEnabled;
}

void FluentMainWindow::setFluentResizeBorderWidth(int px)
{
    if (!m_resizeHelper) {
        updateResizeHelperState();
    }
    if (m_resizeHelper) {
        m_resizeHelper->setBorderWidth(px);
    }
}

int FluentMainWindow::fluentResizeBorderWidth() const
{
    return m_resizeHelper ? m_resizeHelper->borderWidth() : Style::windowMetrics().resizeBorder;
}

QMenuBar *FluentMainWindow::menuBar() const
{
    // Note: QMainWindow::menuBar() is not virtual; this is a convenience shim.
    auto *self = const_cast<FluentMainWindow *>(this);
    self->ensureTitleBar();

    if (!self->m_menuBar) {
        self->setFluentMenuBar(new FluentMenuBar(self->m_leftHost ? self->m_leftHost : self->m_titleBarHost));
    }

    return self->m_menuBar;
}

void FluentMainWindow::setMenuBar(QMenuBar *menuBar)
{
    ensureTitleBar();

    if (!menuBar) {
        setFluentMenuBar(nullptr);
        return;
    }

    if (auto *fluent = qobject_cast<FluentMenuBar *>(menuBar)) {
        setFluentMenuBar(fluent);
        return;
    }

    // Best-effort migration from a native QMenuBar to FluentMenuBar.
    auto *fluent = new FluentMenuBar(m_leftHost ? m_leftHost : m_titleBarHost);

    const QList<QAction *> actions = menuBar->actions();
    for (QAction *a : actions) {
        if (!a) {
            continue;
        }

        if (a->isSeparator()) {
            fluent->addSeparator();
            continue;
        }

        if (QMenu *m = a->menu()) {
            // Re-parent menu by adding it again.
            fluent->addMenu(m);
            continue;
        }

        fluent->addAction(a);
    }

    // If the passed menu bar is owned by this window, it's safe to discard.
    if (menuBar->parent() == this || menuBar->parent() == m_titleBarHost) {
        menuBar->deleteLater();
    }

    setFluentMenuBar(fluent);
}

FluentMenuBar *FluentMainWindow::fluentMenuBar() const
{
    return qobject_cast<FluentMenuBar *>(menuBar());
}

void FluentMainWindow::setFluentTitleBarEnabled(bool enabled)
{
    m_titleBarEnabled = enabled;

    if (m_titleBarEnabled) {
        setWindowFlag(Qt::FramelessWindowHint, true);
        if (m_titleBarHost) {
            m_titleBarHost->show();
        }
    } else {
        setWindowFlag(Qt::FramelessWindowHint, false);
        if (m_titleBarHost) {
            m_titleBarHost->hide();
        }
    }

    // Applying window flags may require a show() to take effect.
    if (isVisible()) {
        show();
    }

#ifdef Q_OS_WIN
    applyWindowsDwmAttributes();
#endif

    updateResizeHelperState();
}

bool FluentMainWindow::fluentTitleBarEnabled() const
{
    return m_titleBarEnabled;
}

void FluentMainWindow::setFluentMenuBar(FluentMenuBar *menuBar)
{
    ensureTitleBar();
    if (m_menuBar == menuBar) {
        return;
    }

    if (m_menuBar && m_menuBar->parent() == m_titleBarHost) {
        m_menuBar->deleteLater();
    }

    m_menuBar = menuBar;
    if (!m_menuBar) {
        return;
    }

    m_menuBar->setParent(m_leftHost ? m_leftHost : m_titleBarHost);
    m_menuBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    m_menuBar->setMinimumWidth(m_menuBar->minimumSizeHint().width());

    if (m_leftHost) {
        auto *layout = qobject_cast<QHBoxLayout *>(m_leftHost->layout());
        if (layout) {
            layout->addWidget(m_menuBar, 1);
            layout->activate();
        }
    }

    if (m_leftHost) {
        if (m_leftHost->layout()) {
            m_leftHost->layout()->activate();
        }
        m_leftHost->setMinimumWidth(m_leftHost->sizeHint().width());
    }
    updateTitleBarContent();
}

void FluentMainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowTitleChange || event->type() == QEvent::WindowIconChange) {
        updateTitleBarContent();
    } else if (event->type() == QEvent::WindowStateChange) {
        updateWindowControlIcons();
#ifdef Q_OS_WIN
        applyWindowsDwmAttributes();
#endif

        // Maximized/restored affects the outer frame border/inset.
        updateTitleBarContent();
        updateFrameHost();
    } else if (event->type() == QEvent::ActivationChange) {
        updateFrameHost();
    }
}

void FluentMainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateTitleBarContent();

    updateFrameHost();
}

void FluentMainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    // Allow painting after the first event loop cycle.
    if (!property("_fluentPaintReady").toBool()) {
        QTimer::singleShot(0, this, [this]() {
            setProperty("_fluentPaintReady", true);
            if (m_titleBarHost) {
                m_titleBarHost->update();
            }
            updateFrameHost();
        });
    }

    if (!m_initialTracePlayed) {
        // Trigger the initial trace only after the first paint.
        m_initialTracePending = true;
    }

    updateFrameHost();
}

bool FluentMainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == this && m_initialTracePending && event && event->type() == QEvent::Paint) {
        m_initialTracePending = false;
        m_initialTracePlayed = true;
        QTimer::singleShot(0, this, [this]() {
            if (!isVisible()) {
                return;
            }
            m_border.playInitialTraceOnce(0);
            syncBorderVisualState();
        });
    }

    if (watched == this && event->type() == QEvent::ChildAdded) {
        // Install this filter on newly added child widgets as well, so we can enable
        // background dragging even when the title bar is collapsed.
        if (auto *ce = static_cast<QChildEvent *>(event)) {
            if (ce->child() && ce->child()->isWidgetType()) {
                ce->child()->installEventFilter(this);
            }
        }

        // Keep title bar and border overlay above widgets added later.
        if (m_titleBarHost) {
            m_titleBarHost->raise();
        }
        if (m_borderOverlay) {
            m_borderOverlay->raise();
        }

        return QMainWindow::eventFilter(watched, event);
    }

    const bool titleBarActuallyVisible =
        m_titleBarEnabled && m_titleBarHost && m_titleBarHost->isVisible() && m_titleBarHost->height() > 0;

    // If the title bar is collapsed/hidden, allow dragging the window from non-interactive areas.
    if (!titleBarActuallyVisible && event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton && windowHandle()) {
            QWidget *w = qobject_cast<QWidget *>(watched);
            // Do not steal from interactive widgets.
            for (QWidget *p = w; p && p != this; p = p->parentWidget()) {
                if (qobject_cast<QAbstractButton *>(p)) {
                    return QMainWindow::eventFilter(watched, event);
                }
                if (p->focusPolicy() != Qt::NoFocus) {
                    return QMainWindow::eventFilter(watched, event);
                }
                if (auto *lbl = qobject_cast<QLabel *>(p)) {
                    if (lbl->textInteractionFlags().testFlag(Qt::TextSelectableByMouse)
                        || lbl->textInteractionFlags().testFlag(Qt::TextSelectableByKeyboard)) {
                        return QMainWindow::eventFilter(watched, event);
                    }
                }
            }

            windowHandle()->startSystemMove();
            return true;
        }
    }

    if (watched == m_titleBarHost) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                const QPoint inTitle = me->pos();
                QWidget *child = m_titleBarHost->childAt(inTitle);

                // Interactive widgets should remain clickable.
                for (QWidget *p = child; p && p != m_titleBarHost; p = p->parentWidget()) {
                    if (qobject_cast<QAbstractButton *>(p)) {
                        return QMainWindow::eventFilter(watched, event);
                    }

                    if (p->focusPolicy() != Qt::NoFocus) {
                        return QMainWindow::eventFilter(watched, event);
                    }

                    if (auto *mb = qobject_cast<QMenuBar *>(p)) {
                        const QPoint inMenu = mb->mapFrom(m_titleBarHost, inTitle);
                        if (mb->rect().contains(inMenu)) {
                            return QMainWindow::eventFilter(watched, event);
                        }
                        break;
                    }
                }

                if (windowHandle()) {
                    windowHandle()->startSystemMove();
                    return true;
                }
            }
        }
        if (event->type() == QEvent::MouseButtonDblClick) {
            if (isMaximized()) {
                showNormal();
            } else {
                showMaximized();
            }
            updateWindowControlIcons();
#ifdef Q_OS_WIN
            applyWindowsDwmAttributes();
#endif
            return true;
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

#ifdef Q_OS_WIN
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool FluentMainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
#else
bool FluentMainWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
#endif
{
    Q_UNUSED(eventType)

    MSG *msg = static_cast<MSG *>(message);
    if (!msg) {
        return QMainWindow::nativeEvent(eventType, message, result);
    }

    if (msg->message == WM_NCHITTEST && m_titleBarEnabled) {
        if (isMaximized()) {
            return QMainWindow::nativeEvent(eventType, message, result);
        }

        const QPoint globalPos(GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam));
        const QPoint localPos = mapFromGlobal(globalPos);

        const int x = localPos.x();
        const int y = localPos.y();
        const int w = width();
        const int h = height();
        const int b = Style::windowMetrics().resizeBorder;

        const bool left = x >= 0 && x < b;
        const bool right = x <= w && x > w - b;
        const bool top = y >= 0 && y < b;
        const bool bottom = y <= h && y > h - b;

        if (top && left) {
            *result = HTTOPLEFT;
            return true;
        }
        if (top && right) {
            *result = HTTOPRIGHT;
            return true;
        }
        if (bottom && left) {
            *result = HTBOTTOMLEFT;
            return true;
        }
        if (bottom && right) {
            *result = HTBOTTOMRIGHT;
            return true;
        }
        if (left) {
            *result = HTLEFT;
            return true;
        }
        if (right) {
            *result = HTRIGHT;
            return true;
        }
        if (top) {
            *result = HTTOP;
            return true;
        }
        if (bottom) {
            *result = HTBOTTOM;
            return true;
        }

        if (m_titleBarHost && m_titleBarHost->geometry().contains(localPos)) {
            const QPoint inTitle = m_titleBarHost->mapFrom(this, localPos);
            QWidget *child = m_titleBarHost->childAt(inTitle);

            // If cursor is over interactive widgets (menu bar / buttons), don't treat as caption.
            for (QWidget *p = child; p && p != m_titleBarHost; p = p->parentWidget()) {
                if (qobject_cast<QAbstractButton *>(p)) {
                    *result = HTCLIENT;
                    return true;
                }

                if (p->focusPolicy() != Qt::NoFocus) {
                    *result = HTCLIENT;
                    return true;
                }

                if (auto *mb = qobject_cast<QMenuBar *>(p)) {
                    const QPoint inMenu = mb->mapFrom(this, localPos);
                    if (mb->rect().contains(inMenu)) {
                        *result = HTCLIENT;
                        return true;
                    }

                    *result = HTCAPTION;
                    return true;
                }
            }

            *result = HTCAPTION;
            return true;
        }
    }

    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif

void FluentMainWindow::applyThemeToApplication()
{
    // Keep border + trace animation in sync with theme.
    if (isVisible()) {
        m_border.onThemeChanged();
    } else {
        m_border.syncFromTheme();
    }
    syncBorderVisualState();

    if (auto *app = qobject_cast<QApplication *>(QCoreApplication::instance())) {
        const auto &colors = ThemeManager::instance().colors();
        const QString next = Theme::baseStyleSheet(colors);
        if (app->styleSheet() != next) {
            app->setStyleSheet(next);
        }
    }

    updateTitleBarContent();
    updateWindowControlIcons();

    updateFrameHost();

#ifdef Q_OS_WIN
    applyWindowsDwmAttributes();
#endif
}

void FluentMainWindow::ensureTitleBar()
{
    if (m_titleBarHost) {
        return;
    }

    m_titleBarHost = new QWidget(this);
    m_titleBarHost->setObjectName("FluentTitleBarHost");
    m_titleBarHost->installEventFilter(this);

    const auto wm = Style::windowMetrics();
    m_titleBarHost->setMinimumHeight(0);

    auto *layout = new QHBoxLayout(m_titleBarHost);
    layout->setContentsMargins(wm.titleBarPaddingX, wm.titleBarPaddingY, wm.titleBarPaddingX, wm.titleBarPaddingY);
    layout->setSpacing(8);

    // Left zone: icon + optional menu bar
    m_leftHost = new QWidget(m_titleBarHost);
    auto *leftLayout = new QHBoxLayout(m_leftHost);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    m_iconLabel = new QLabel(m_leftHost);
    m_iconLabel->setObjectName("FluentWindowIcon");
    m_iconLabel->setFixedSize(16, 16);
    leftLayout->addWidget(m_iconLabel);

    // Left slot: between icon and menu bar.
    m_leftSlotHost = new QWidget(m_leftHost);
    m_leftSlotHost->setObjectName("FluentTitleBarLeftSlotHost");
    m_leftSlotHost->hide();
    leftLayout->addWidget(m_leftSlotHost);

    layout->addWidget(m_leftHost, 0, Qt::AlignLeft | Qt::AlignVCenter);

    // Center zone: title (kept visually centered via symmetric stretches)
    layout->addStretch(1);

    m_centerHost = new QWidget(m_titleBarHost);
    auto *centerLayout = new QHBoxLayout(m_centerHost);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);

    m_titleLabel = new QLabel(m_centerHost);
    m_titleLabel->setObjectName("FluentWindowTitle");
    m_titleLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_titleLabel->setMinimumWidth(0);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    centerLayout->addWidget(m_titleLabel);

    layout->addWidget(m_centerHost, 0, Qt::AlignCenter);

    layout->addStretch(1);

    // Right zone: optional slot + window control buttons.
    m_rightHost = new QWidget(m_titleBarHost);
    m_rightHost->setObjectName("FluentTitleBarRightHost");
    auto *rightLayout = new QHBoxLayout(m_rightHost);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    m_rightSlotHost = new QWidget(m_rightHost);
    m_rightSlotHost->setObjectName("FluentTitleBarRightSlotHost");
    m_rightSlotHost->hide();
    rightLayout->addWidget(m_rightSlotHost, 0, Qt::AlignVCenter);

    m_windowButtonsHost = new QWidget(m_rightHost);
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
        updateWindowControlIcons();
    });
    connect(m_closeBtn, &QToolButton::clicked, this, &QWidget::close);

    btnLayout->addWidget(m_minBtn);
    btnLayout->addWidget(m_maxBtn);
    btnLayout->addWidget(m_closeBtn);

    rightLayout->addWidget(m_windowButtonsHost, 0, Qt::AlignVCenter);

    layout->addWidget(m_rightHost, 0, Qt::AlignRight | Qt::AlignVCenter);

    setMenuWidget(m_titleBarHost);

    // Apply initial visibility flags.
    setFluentWindowButtons(m_windowButtons);

    updateTitleBarContent();
    updateWindowControlIcons();
}

void FluentMainWindow::updateTitleBarContent()
{
    if (!m_titleBarHost) {
        return;
    }

    const auto &colors = ThemeManager::instance().colors();
    const bool dark = colors.background.lightnessF() < 0.5;

    // Elide title to avoid pushing menu/buttons.
    const int leftW = m_leftHost ? qMax(m_leftHost->minimumWidth(), m_leftHost->sizeHint().width()) : 0;
    const int rightW = m_rightHost ? qMax(m_rightHost->minimumWidth(), m_rightHost->sizeHint().width()) : 0;
    const int margins = Style::windowMetrics().titleBarPaddingX * 2 + 24;
    const int maxTitleWidth = qMax(0, width() - leftW - rightW - margins);
    const QString effectiveTitle = m_hasTitleOverride ? m_titleOverride : windowTitle();
    const QString titleText = fontMetrics().elidedText(effectiveTitle, Qt::ElideRight, maxTitleWidth);

    m_titleLabel->setMaximumWidth(maxTitleWidth);
    m_titleLabel->setText(titleText);
    m_titleLabel->setStyleSheet(QString("color: %1;").arg(colors.text.name()));

    QIcon icon;
    if (m_hasIconOverride) {
        icon = m_iconOverride;
    } else {
        icon = windowIcon().isNull() ? qApp->windowIcon() : windowIcon();
    }
    m_iconLabel->setPixmap(icon.pixmap(16, 16));

    QColor divider = colors.border;
    divider.setAlpha(dark ? 180 : 130);

    const bool maximizedOrFullscreen = isMaximized() || isFullScreen();

    // Keep 1px frame space when restored so the accent border drawn by m_frameHost
    // is visible on all edges. When maximized, use 0 inset.
    const int inset = maximizedOrFullscreen ? 0 : 1;
    if (contentsMargins().left() != inset || contentsMargins().top() != inset
        || contentsMargins().right() != inset || contentsMargins().bottom() != inset) {
        setContentsMargins(inset, inset, inset, inset);
    }

    // Update the border overlay to match current content area.
    updateFrameHost();

    // NOTE: do not use isVisible() here; before the window is shown it will be false even if
    // the widgets are logically visible. Use our flags instead.
    const bool anyButtonsVisible = (m_windowButtons.testFlag(MinimizeButton)
        || m_windowButtons.testFlag(MaximizeButton)
        || m_windowButtons.testFlag(CloseButton));

    const bool anyCustomContent = (m_leftCustomWidget != nullptr)
        || (m_centerCustomWidget != nullptr)
        || (m_rightCustomWidget != nullptr);
    const bool anyMenu = (m_menuBar != nullptr);

    const bool titleBarNeeded = anyButtonsVisible || anyCustomContent || anyMenu || m_hasTitleOverride || m_hasIconOverride;
    const bool titleBarVisible = m_titleBarEnabled && titleBarNeeded;
    m_titleBarHost->setVisible(titleBarVisible);

    if (!titleBarVisible) {
        if (m_titleBarHost->height() != 0) {
            m_titleBarHost->setFixedHeight(0);
        }
        return;
    }

    const QString bottomRule = anyButtonsVisible
        ? QStringLiteral("border-bottom: 1px solid %1;").arg(rgbaString(divider))
        : QStringLiteral("border-bottom: none;");

    // The frame host draws the outer border with full kWindowFrameRadius.
    // The title bar is inside the 1px inset, so its effective inner radius is slightly smaller.
    const qreal innerRadius = maximizedOrFullscreen ? 0.0 : qMax(0.0, kWindowFrameRadius - 1.0);
    const int frameRadius = static_cast<int>(innerRadius);
    const QString radiusRule = QStringLiteral(
        "border-top-left-radius: %1px; border-top-right-radius: %1px; "
        "border-bottom-left-radius: 0px; border-bottom-right-radius: 0px;")
        .arg(frameRadius);

    // Background for title bar area (slightly separated from window background)
    m_titleBarHost->setStyleSheet(QString(
        "#FluentTitleBarHost { background: %1; %2 %3 }"
        "#FluentWindowTitle { color: %4; font-weight: 600; }"
    ).arg(colors.surface.name(), bottomRule, radiusRule, colors.text.name()));

    // Dynamically adjust title bar height to fit its contents.
    if (auto *l = m_titleBarHost->layout()) {
        l->activate();
        const int contentHintH = l->sizeHint().height();
        const int baseH = anyButtonsVisible ? Style::windowMetrics().titleBarHeight : 0;
        const int desiredH = qMax(baseH, contentHintH);
        if (m_titleBarHost->height() != desiredH) {
            m_titleBarHost->setFixedHeight(desiredH);
        }
    }

    // NOTE: We intentionally do NOT set a QRegion mask on the title bar.
    // QRegion masks are pixel-based and produce visible jaggies on rounded corners.
    // On Windows 11, DWM DWMWA_WINDOW_CORNER_PREFERENCE = DWMWCP_ROUND provides
    // smooth system-level corner clipping.  The CSS border-radius on the title bar
    // provides the visual rounded background.
}

void FluentMainWindow::updateWindowControlIcons()
{
    if (!m_minBtn || !m_maxBtn || !m_closeBtn) {
        return;
    }

    m_maxBtn->setProperty("fluentWindowGlyph", isMaximized() ? GlyphRestore : GlyphMaximize);
    m_minBtn->update();
    m_maxBtn->update();
    m_closeBtn->update();
}

#ifdef Q_OS_WIN
void FluentMainWindow::applyWindowsDwmAttributes()
{
    if (!m_titleBarEnabled) {
        return;
    }

    HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
    if (!dwm) {
        return;
    }

    using DwmSetWindowAttributeFn = HRESULT(WINAPI *)(HWND, DWORD, LPCVOID, DWORD);
    auto fn = reinterpret_cast<DwmSetWindowAttributeFn>(GetProcAddress(dwm, "DwmSetWindowAttribute"));
    if (!fn) {
        FreeLibrary(dwm);
        return;
    }

    HWND hwnd = reinterpret_cast<HWND>(winId());

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif
#ifndef DWMWCP_DONOTROUND
#define DWMWCP_DONOTROUND 1
#endif
    const int cornerPref = isMaximized() ? DWMWCP_DONOTROUND : DWMWCP_ROUND;
    fn(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPref, sizeof(cornerPref));

    const bool isDark = ThemeManager::instance().colors().background.lightnessF() < 0.5;
    const BOOL darkVal = isDark ? TRUE : FALSE;
    const DWORD attr19 = 19;
    const DWORD attr20 = 20;
    fn(hwnd, attr20, &darkVal, sizeof(darkVal));
    fn(hwnd, attr19, &darkVal, sizeof(darkVal));

    FreeLibrary(dwm);
}
#endif

void FluentMainWindow::updateResizeHelperState()
{
    const bool shouldEnable = m_resizeEnabled && m_titleBarEnabled;

    if (shouldEnable) {
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

} // namespace Fluent
