#include "Fluent/FluentMainWindow.h"

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

static int effectiveWindowRadiusPx(const QWidget *w)
{
    if (!w) {
        return 0;
    }

    if (w->isMaximized() || w->isFullScreen()) {
        return 0;
    }

    const int base = Style::metrics().radius;
    const int inset = w->contentsMargins().left();
    return qMax(0, base - inset);
}

static QString rgbaString(const QColor &c)
{
    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(c.red())
        .arg(c.green())
        .arg(c.blue())
        .arg(c.alpha());
}

class WindowAccentBorderOverlay final : public QWidget
{
public:
    explicit WindowAccentBorderOverlay(QWidget *parent)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_StyledBackground, false);
        setAutoFillBackground(false);
        setFocusPolicy(Qt::NoFocus);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)
        if (const QWidget *w = window()) {
            if (!w->property("_fluentPaintReady").toBool()) {
                return;
            }
            if (QWindow *wh = w->windowHandle()) {
                if (!wh->isExposed()) {
                    return;
                }
            }
        }
        if (const QWidget *w = window()) {
            if (QWindow *wh = w->windowHandle()) {
                if (!wh->isExposed()) {
                    return;
                }
            }
        }
        const auto &colors = ThemeManager::instance().colors();

        QPainter p(this);
        if (!p.isActive()) {
            return;
        }
        p.setRenderHint(QPainter::Antialiasing, true);

        // Align strokes to device pixels to reduce jaggies/blur on rounded corners (notably on HiDPI).
        qreal dpr = devicePixelRatioF();
        if (dpr <= 0.0) {
            dpr = 1.0;
        }
        const qreal px = 0.5 / dpr;

        QPen pen(colors.accent, 1.0);
        if (!ThemeManager::instance().accentBorderEnabled()) {
            pen.setColor(colors.border);
        }
        pen.setJoinStyle(Qt::MiterJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        QRectF r = QRectF(rect()).adjusted(px, px, -px, -px);
        const QWidget *w = window();
        const qreal radius = effectiveWindowRadiusPx(w);

        if (radius <= 0.0) {
            p.drawRect(r);
        } else {
            p.drawRoundedRect(r, radius, radius);
        }
    }
};

class WindowAccentBorderMarqueeOverlay final : public QWidget
{
public:
    explicit WindowAccentBorderMarqueeOverlay(QWidget *parent)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_StyledBackground, false);
        setAutoFillBackground(false);
        setFocusPolicy(Qt::NoFocus);
        hide();
    }

    void setToAccent(bool toAccent)
    {
        m_toAccent = toAccent;
        update();
    }

    void setProgress(qreal t)
    {
        m_t = t;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)
        if (m_t < 0.0) {
            return;
        }

        if (const QWidget *w = window()) {
            if (!w->property("_fluentPaintReady").toBool()) {
                return;
            }
            if (QWindow *wh = w->windowHandle()) {
                if (!wh->isExposed()) {
                    return;
                }
            }
        }

        if (const QWidget *w = window()) {
            if (QWindow *wh = w->windowHandle()) {
                if (!wh->isExposed()) {
                    return;
                }
            }
        }

        const auto &colors = ThemeManager::instance().colors();

        QPainter p(this);
        if (!p.isActive()) {
            return;
        }
        p.setRenderHint(QPainter::Antialiasing, true);

        qreal dpr = devicePixelRatioF();
        if (dpr <= 0.0) {
            dpr = 1.0;
        }
        const qreal px = 0.5 / dpr;

        QRectF r = QRectF(rect()).adjusted(px, px, -px, -px);
        const QWidget *w = window();
        const qreal radius = effectiveWindowRadiusPx(w);

        const QColor trace = m_toAccent ? colors.accent : colors.border;
        Style::paintTraceBorder(p, r, radius, trace, m_t, 1.0, 0.0);
    }

private:
    qreal m_t = -1.0;
    bool m_toAccent = true;
};

class WindowCentralClipHost final : public QWidget
{
public:
    explicit WindowCentralClipHost(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_StyledBackground, false);
        setAutoFillBackground(false);
    }

    void refreshMask()
    {
        updateMask();
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        updateMask();
    }

private:
    void updateMask()
    {
        const QWidget *w = window();
        const qreal radius = effectiveWindowRadiusPx(w);

        if (radius <= 0.0) {
            clearMask();
            return;
        }

        bool titleBarVisible = false;
        if (w) {
            if (QWidget *title = w->findChild<QWidget *>(QStringLiteral("FluentTitleBarHost"))) {
                titleBarVisible = title->isVisible() && title->height() > 0;
            }
        }

        const QRectF r = QRectF(rect());
        if (r.isEmpty()) {
            clearMask();
            return;
        }

        QPainterPath path;
        if (titleBarVisible) {
            // Title bar owns the top corners. Only clip bottom corners here so we don't
            // create gaps under the title bar.
            path.moveTo(r.left(), r.top());
            path.lineTo(r.right(), r.top());
            path.lineTo(r.right(), r.bottom() - radius);
            path.quadTo(r.right(), r.bottom(), r.right() - radius, r.bottom());
            path.lineTo(r.left() + radius, r.bottom());
            path.quadTo(r.left(), r.bottom(), r.left(), r.bottom() - radius);
            path.closeSubpath();
        } else {
            // When the title bar is collapsed/hidden, central content reaches the top,
            // so clip all four corners.
            path = Style::roundedRectPath(r, radius);
        }

        setMask(QRegion(path.toFillPolygon().toPolygon()));
    }
};

class WindowAccentCentralBorderHost final : public QWidget
{
public:
    explicit WindowAccentCentralBorderHost(QWidget *parent)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_StyledBackground, false);
        setAutoFillBackground(false);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)
        if (const QWidget *w = window()) {
            if (!w->property("_fluentPaintReady").toBool()) {
                return;
            }
            if (QWindow *wh = w->windowHandle()) {
                if (!wh->isExposed()) {
                    return;
                }
            }
        }
        if (const QWidget *w = window()) {
            if (QWindow *wh = w->windowHandle()) {
                if (!wh->isExposed()) {
                    return;
                }
            }
        }
        const auto &colors = ThemeManager::instance().colors();

        QPainter p(this);
        if (!p.isActive()) {
            return;
        }
        p.setRenderHint(QPainter::Antialiasing, true);

        qreal dpr = devicePixelRatioF();
        if (dpr <= 0.0) {
            dpr = 1.0;
        }
        const qreal px = 0.5 / dpr;

        bool effectiveAccent = ThemeManager::instance().accentBorderEnabled();
        if (const QWidget *w = window()) {
            if (w->property("_fluentBorderMarqueeActive").toBool()) {
                effectiveAccent = w->property("_fluentBorderMarqueeFromAccent").toBool();
            }
        }

        QPen pen(effectiveAccent ? colors.accent : colors.border, 1.0);
        pen.setJoinStyle(Qt::MiterJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        const QWidget *widget = window();
        const bool widgetMaximizedOrFullscreen = widget? (widget->isMaximized() || widget->isFullScreen()) : false;
        const qreal radius = widgetMaximizedOrFullscreen ? 0.0 : effectiveWindowRadiusPx(widget);

        // Draw only left/right/bottom so it connects with the title bar's top border.
        const QRectF r = QRectF(rect()).adjusted(px, 0.0, -px, -px);

        // Left / Right
        p.drawLine(QPointF(r.left(), r.top()), QPointF(r.left(), r.bottom() - radius));
        p.drawLine(QPointF(r.right(), r.top()), QPointF(r.right(), r.bottom() - radius));

        // Bottom
        p.drawLine(QPointF(r.left() + radius, r.bottom()), QPointF(r.right() - radius, r.bottom()));

        if (radius > 0.0) {
            const QRectF bl(r.left(), r.bottom() - 2 * radius, 2 * radius, 2 * radius);
            const QRectF br(r.right() - 2 * radius, r.bottom() - 2 * radius, 2 * radius, 2 * radius);
            p.drawArc(bl, 180 * 16, 90 * 16);
            p.drawArc(br, 270 * 16, 90 * 16);
        }
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

    // Always-on accent border overlay (topmost). Transparent for mouse.
    m_borderOverlay = new WindowAccentBorderOverlay(this);
    m_borderOverlay->setObjectName("FluentMainWindowAccentBorderOverlay");
    m_borderOverlay->setGeometry(rect());
    m_borderOverlay->hide();

    m_borderMarqueeOverlay = new WindowAccentBorderMarqueeOverlay(this);
    m_borderMarqueeOverlay->setObjectName("FluentMainWindowAccentBorderMarqueeOverlay");
    m_borderMarqueeOverlay->setGeometry(rect());

    m_border.syncFromTheme();
    m_border.setRequestUpdate([this]() {
        syncBorderVisualState();
        if (m_borderMarqueeOverlay && m_borderMarqueeOverlay->isVisible()) {
            m_borderMarqueeOverlay->raise();
        }
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

    // Initialize marquee properties/overlay visibility.
    syncBorderVisualState();
}

void FluentMainWindow::syncBorderVisualState()
{
    const bool animating = m_border.isAnimating();
    setProperty("_fluentBorderMarqueeActive", animating);
    setProperty("_fluentBorderMarqueeFromAccent", m_border.fromEnabled());
    setProperty("_fluentBorderMarqueeToAccent", m_border.toEnabled());

    if (auto *overlay = dynamic_cast<WindowAccentBorderMarqueeOverlay *>(m_borderMarqueeOverlay)) {
        if (animating) {
            overlay->setToAccent(m_border.toEnabled());
            overlay->setProgress(m_border.t());
        } else {
            overlay->setProgress(-1.0);
        }
    }

    if (m_borderMarqueeOverlay) {
        m_borderMarqueeOverlay->setGeometry(rect());
        if (animating) {
            m_borderMarqueeOverlay->raise();
            m_borderMarqueeOverlay->show();
        } else {
            m_borderMarqueeOverlay->hide();
        }
        m_borderMarqueeOverlay->update();
    }

    // Keep base border (title bar + central border host) in the "from" color while tracing.
    updateTitleBarContent();
    if (m_titleBarHost) {
        m_titleBarHost->update();
    }
    if (m_centralBorderHost) {
        m_centralBorderHost->update();
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

    ensureCentralBorderHost();

    if (!m_centralBorderHost) {
        m_userCentralWidget = widget;
        QMainWindow::setCentralWidget(widget);
        return;
    }

    // Remove old
    if (m_userCentralWidget && m_centralClipHost && m_userCentralWidget->parent() == m_centralClipHost) {
        m_userCentralWidget->setParent(nullptr);
    }
    m_userCentralWidget = widget;

    QWidget *host = m_centralClipHost ? m_centralClipHost : m_centralBorderHost;
    auto *layout = qobject_cast<QVBoxLayout *>(host->layout());
    if (!layout) {
        layout = new QVBoxLayout(host);
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
        widget->setParent(host);
        layout->addWidget(widget);
    }

    m_centralBorderHost->update();

    if (m_borderOverlay) {
        m_borderOverlay->raise();
        m_borderOverlay->update();
    }
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

void FluentMainWindow::ensureCentralBorderHost()
{
    if (m_centralBorderHost) {
        return;
    }

    m_centralBorderHost = new WindowAccentCentralBorderHost(this);
    m_centralBorderHost->setObjectName(QStringLiteral("FluentMainWindowCentralBorderHost"));

    auto *layout = new QVBoxLayout(m_centralBorderHost);
    // Reserve 1px for the border; rounded clipping is handled by m_centralClipHost.
    layout->setContentsMargins(1, 0, 1, 1);
    layout->setSpacing(0);

    m_centralClipHost = new WindowCentralClipHost(m_centralBorderHost);
    m_centralClipHost->setObjectName(QStringLiteral("FluentMainWindowCentralClipHost"));
    layout->addWidget(m_centralClipHost);

    QMainWindow::setCentralWidget(m_centralBorderHost);
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
    // Keep menu bar visible and clickable in the custom title bar.
    // Using Expanding here can make it compete with the title label and trigger
    // QMenuBar's internal overflow (qt_menubar_ext_button).
    m_menuBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    // Prevent QMenuBar from being compressed into the built-in overflow button ("▼")
    // which looks native and also spawns a native-looking overflow menu.
    // In a Fluent title bar we prefer showing top-level menus directly.
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
        // Reserve enough space for icon + menu bar so clicks on actions won't be eaten by
        // the title bar drag logic and so Qt won't collapse items into the overflow button.
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

        if (m_centralBorderHost) {
            m_centralBorderHost->update();
        }
    } else if (event->type() == QEvent::ActivationChange) {
        if (m_centralBorderHost) {
            m_centralBorderHost->update();
        }
    }
}

void FluentMainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateTitleBarContent();

    if (m_borderOverlay) {
        m_borderOverlay->setGeometry(rect());
        if (m_borderOverlay->isVisible()) {
            m_borderOverlay->raise();
            m_borderOverlay->update();
        }
    }

    if (m_borderMarqueeOverlay) {
        m_borderMarqueeOverlay->setGeometry(rect());
        if (m_borderMarqueeOverlay->isVisible()) {
            m_borderMarqueeOverlay->raise();
            m_borderMarqueeOverlay->update();
        }
    }

    if (m_centralBorderHost) {
        m_centralBorderHost->update();
    }
}

void FluentMainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    // Allow overlay painting after the first event loop cycle.
    if (!property("_fluentPaintReady").toBool()) {
        QTimer::singleShot(0, this, [this]() {
            setProperty("_fluentPaintReady", true);
            if (m_titleBarHost) {
                m_titleBarHost->update();
            }
            if (m_centralBorderHost) {
                m_centralBorderHost->update();
            }
            if (m_borderOverlay) {
                m_borderOverlay->update();
            }
            if (m_borderMarqueeOverlay) {
                m_borderMarqueeOverlay->update();
            }
        });
    }

    if (!m_initialTracePlayed) {
        // Trigger the initial trace only after the first paint.
        // Some platforms can still report QPainter::begin engine==0 if we start too early.
        m_initialTracePending = true;
    }

    if (m_borderOverlay) {
        m_borderOverlay->setGeometry(rect());
        if (m_borderOverlay->isVisible()) {
            m_borderOverlay->raise();
            m_borderOverlay->update();
        }
    }

    if (m_borderMarqueeOverlay) {
        m_borderMarqueeOverlay->setGeometry(rect());
        if (m_borderMarqueeOverlay->isVisible()) {
            m_borderMarqueeOverlay->raise();
            m_borderMarqueeOverlay->update();
        }
    }

    if (m_centralBorderHost) {
        m_centralBorderHost->update();
    }
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

        // Keep title bar above widgets added later (e.g. replacing central widget).
        if (m_titleBarHost) {
            m_titleBarHost->raise();
        }

        if (m_borderOverlay) {
            m_borderOverlay->raise();
        }

        if (m_borderMarqueeOverlay) {
            m_borderMarqueeOverlay->raise();
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

                    // Heuristic: most interactive widgets opt into focus.
                    if (p->focusPolicy() != Qt::NoFocus) {
                        return QMainWindow::eventFilter(watched, event);
                    }

                    if (auto *mb = qobject_cast<QMenuBar *>(p)) {
                        const QPoint inMenu = mb->mapFrom(m_titleBarHost, inTitle);
                        // If cursor is anywhere within the menu bar rect, treat it as interactive.
                        // Using actionAt() here can be unreliable with custom painting/padding and
                        // would incorrectly start a system move instead of opening the menu.
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

                // Special-case menu bar: allow dragging on empty spaces but keep menu actions clickable.
                if (auto *mb = qobject_cast<QMenuBar *>(p)) {
                    const QPoint inMenu = mb->mapFrom(this, localPos);
                    // Treat the whole menubar area as client so clicks always reach QMenuBar.
                    // This avoids "click doesn't open menu" when the title bar hit-test steals it.
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
        // Setting the same application stylesheet repeatedly is expensive:
        // it triggers a full style repolish across the widget tree.
        if (app->styleSheet() != next) {
            app->setStyleSheet(next);
        }
    }

    updateTitleBarContent();
    updateWindowControlIcons();

    if (m_centralBorderHost) {
        m_centralBorderHost->update();
    }

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
    // Height is finalized in updateTitleBarContent() based on current contents.
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

    // Center zone: user content or default title label.
    m_centerHost = new QWidget(m_titleBarHost);
    auto *centerLayout = new QHBoxLayout(m_centerHost);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);

    m_titleLabel = new QLabel(m_centerHost);
    m_titleLabel->setObjectName("FluentWindowTitle");
    // Allow shrinking aggressively so the menu bar doesn't get pushed into overflow.
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

    // Enable glyph-painting mode in FluentToolButton.
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
    // Use the reserved/minimum widths (not transient current widths) to prevent the title label
    // from temporarily taking too much space and forcing QMenuBar into overflow.
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
    const bool animatingBorder = m_border.isAnimating();
    const bool effectiveAccentForFrame = animatingBorder ? m_border.fromEnabled() : m_border.enabled();
    const bool outerFrameBorder = effectiveAccentForFrame && !maximizedOrFullscreen;

    // Outer frame border mode: inset the entire QMainWindow layout by 1px so the overlay
    // border is never occluded by child widgets. When disabled, cancel the inset.
    const int inset = outerFrameBorder ? 1 : 0;
    if (contentsMargins().left() != inset || contentsMargins().top() != inset
        || contentsMargins().right() != inset || contentsMargins().bottom() != inset) {
        setContentsMargins(inset, inset, inset, inset);
    }


    // Do not rely on an overlay for the border; some widget stacks can occlude it.
    // The 1px inset is kept (when enabled) to ensure the widget-drawn borders remain visible.
    if (m_borderOverlay) {
        m_borderOverlay->hide();
    }

    // NOTE: do not use isVisible() here; before the window is shown it will be false even if
    // the widgets are logically visible. Use our flags instead.
    const bool anyButtonsVisible = (m_windowButtons.testFlag(MinimizeButton)
        || m_windowButtons.testFlag(MaximizeButton)
        || m_windowButtons.testFlag(CloseButton));

    const bool anyCustomContent = (m_leftCustomWidget != nullptr)
        || (m_centerCustomWidget != nullptr)
        || (m_rightCustomWidget != nullptr);
    const bool anyMenu = (m_menuBar != nullptr);

    // If the window buttons are all hidden and there's no meaningful title bar content,
    // collapse the title bar so the central widget can occupy the full height.
    const bool titleBarNeeded = anyButtonsVisible || anyCustomContent || anyMenu || m_hasTitleOverride || m_hasIconOverride;
    const bool titleBarVisible = m_titleBarEnabled && titleBarNeeded;
    m_titleBarHost->setVisible(titleBarVisible);

    if (!titleBarVisible) {
        if (m_titleBarHost->height() != 0) {
            m_titleBarHost->setFixedHeight(0);
        }

        if (m_centralClipHost) {
            if (auto *clip = dynamic_cast<WindowCentralClipHost *>(m_centralClipHost)) {
                clip->refreshMask();
            }
        }
        return;
    }
    const QString bottomRule = anyButtonsVisible
        ? QStringLiteral("border-bottom: 1px solid %1;").arg(rgbaString(divider))
        : QStringLiteral("border-bottom: none;");

    const int frameRadius = effectiveWindowRadiusPx(this);
    const QString radiusRule = QStringLiteral(
        "border-top-left-radius: %1px; border-top-right-radius: %1px; "
        "border-bottom-left-radius: 0px; border-bottom-right-radius: 0px;")
        .arg(frameRadius);

    const QColor titleBorder = effectiveAccentForFrame ? colors.accent : colors.border;
    const QString windowBorderRule = QStringLiteral(
        "border-left: 1px solid %1; border-right: 1px solid %1; border-top: 1px solid %1;")
        .arg(titleBorder.name());

    // Background for title bar area (slightly separated from window background)
    m_titleBarHost->setStyleSheet(QString(
        "#FluentTitleBarHost { background: %1; %2 %3 %4 }"
        "#FluentWindowTitle { color: %5; font-weight: 600; }"
    ).arg(colors.surface.name(), windowBorderRule, bottomRule, radiusRule, colors.text.name()));

    // Dynamically adjust title bar height to fit its contents.
    // This prevents custom widgets (e.g. a FluentLineEdit search box) from being clipped.
    if (auto *l = m_titleBarHost->layout()) {
        l->activate();
        const int contentHintH = l->sizeHint().height();
        // When window buttons exist, keep the standard height; otherwise allow shrinking.
        const int baseH = anyButtonsVisible ? Style::windowMetrics().titleBarHeight : 0;
        const int desiredH = qMax(baseH, contentHintH);
        if (m_titleBarHost->height() != desiredH) {
            m_titleBarHost->setFixedHeight(desiredH);
        }
    }

    // IMPORTANT: Qt style-sheet border-radius does NOT clip child widgets. Use a mask so
    // title/content cannot paint outside the rounded outer frame.
    if (frameRadius <= 0) {
        m_titleBarHost->clearMask();
    } else {
        const QRectF r = QRectF(m_titleBarHost->rect());
        QPainterPath path;
        path.moveTo(r.left(), r.bottom());
        path.lineTo(r.left(), r.top() + frameRadius);
        path.quadTo(r.left(), r.top(), r.left() + frameRadius, r.top());
        path.lineTo(r.right() - frameRadius, r.top());
        path.quadTo(r.right(), r.top(), r.right(), r.top() + frameRadius);
        path.lineTo(r.right(), r.bottom());
        path.closeSubpath();
        m_titleBarHost->setMask(QRegion(path.toFillPolygon().toPolygon()));
    }

    if (m_centralClipHost) {
        if (auto *clip = dynamic_cast<WindowCentralClipHost *>(m_centralClipHost)) {
            clip->refreshMask();
        }
    }
}

void FluentMainWindow::updateWindowControlIcons()
{
    if (!m_minBtn || !m_maxBtn || !m_closeBtn) {
        return;
    }

    // Glyph painting mode lives in FluentToolButton; we only switch maximize vs restore here.
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

    // Use dynamic lookup to avoid link-time dependencies.
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

    // Rounded corners (Windows 11). When maximized, Windows uses square corners.
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

    // Immersive dark mode (best-effort; different builds use 19/20)
    const bool isDark = ThemeManager::instance().colors().background.lightnessF() < 0.5;
    const BOOL dark = isDark ? TRUE : FALSE;
    const DWORD attr19 = 19;
    const DWORD attr20 = 20;
    fn(hwnd, attr20, &dark, sizeof(dark));
    fn(hwnd, attr19, &dark, sizeof(dark));

    FreeLibrary(dwm);
}
#endif

void FluentMainWindow::updateResizeHelperState()
{
    // Only needed in frameless title-bar mode; if system frame is enabled, let OS handle resizing.
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
