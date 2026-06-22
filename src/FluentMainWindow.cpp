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
#include <QActionEvent>
#include <QApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QRegion>
#include <QShowEvent>
#include <QStyle>
#include <QStatusBar>
#include <QStringList>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

namespace Fluent {

namespace {
enum WindowGlyphType { GlyphMinimize = 0, GlyphMaximize = 1, GlyphRestore = 2, GlyphClose = 3 };

// Window frame radius matching the Windows 11 DWM DWMWCP_ROUND corner clip (8 DIPs).
static constexpr qreal kWindowFrameRadius = 8.0;

#ifdef Q_OS_WIN
// DWMWA_WINDOW_CORNER_PREFERENCE rounds a top-level window's corners only on
// Windows 11 (build >= 22000). On Windows 10 DwmSetWindowAttribute accepts the
// call but does nothing, so the frameless window keeps square corners. Detected
// once via RtlGetVersion -- the documented GetVersionEx/VersionHelpers are
// manifest-gated and under-report the build on Win10/11. See GitHub issue #17.
static bool isWindows11OrGreater()
{
    using RtlGetVersionFn = LONG(WINAPI *)(OSVERSIONINFOEXW *);
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) {
        return false;
    }
    auto rtlGetVersion = reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion"));
    if (!rtlGetVersion) {
        return false;
    }
    OSVERSIONINFOEXW info{};
    info.dwOSVersionInfoSize = sizeof(info);
    if (rtlGetVersion(&info) != 0) { // != STATUS_SUCCESS
        return false;
    }
    return info.dwMajorVersion > 10
        || (info.dwMajorVersion == 10 && info.dwBuildNumber >= 22000);
}
#endif

// Effective outer window-frame radius. The frameless window only gets real
// rounded corners where the platform can clip them: Windows 11 via DWM, and
// other platforms via the cross-platform overlay fallback. On Windows 10 there
// is no corner clipping, so painting a rounded border over the opaque card fill
// leaves the corners poking through as white squares (issue #17) -- draw square
// corners there instead, matching the native Windows 10 window shape.
static qreal effectiveWindowFrameRadius()
{
#ifdef Q_OS_WIN
    static const bool rounded = isWindows11OrGreater();
    if (!rounded) {
        return 0.0;
    }
#endif
    return kWindowFrameRadius;
}

static QString applicationStyleSignature(const ThemeColors &colors)
{
    // QApplication::setStyleSheet() repolishes the whole widget tree. Keep this
    // signature to the colors that Theme::baseStyleSheet actually emits so
    // border-only token changes refresh native/fallback chrome without making
    // unrelated themeChanged signals expensive.
    const auto tokens = Theme::tokens(colors);
    const QColor tooltipFill = Style::mix(tokens.neutral.layer,
                                          tokens.accent.base,
                                          tokens.dark ? 0.16 : 0.07);
    const QColor tooltipBorder = Style::mix(tokens.neutral.strokeSubtle,
                                            tokens.accent.base,
                                            tokens.dark ? 0.20 : 0.12);
    const QColor scrollHandle = Style::withAlpha(tokens.neutral.strokeStrong,
                                                 tokens.dark ? 132 : 116);
    const QColor scrollHandleHover = Style::withAlpha(
        Style::mix(tokens.neutral.strokeStrong, colors.text, tokens.dark ? 0.16 : 0.10),
        tokens.dark ? 168 : 148);
    const QColor scrollHandlePressed = Style::withAlpha(
        Style::mix(tokens.neutral.strokeStrong, colors.text, tokens.dark ? 0.26 : 0.18),
        tokens.dark ? 204 : 180);

    QStringList parts;
    parts << QStringLiteral("base-style")
          << QString::number(tokens.typography.body)
          << colors.text.name(QColor::HexArgb)
          << tokens.neutral.background.name(QColor::HexArgb)
          << tokens.accent.base.name(QColor::HexArgb)
          << tooltipFill.name(QColor::HexArgb)
          << tooltipBorder.name(QColor::HexArgb)
          << scrollHandle.name(QColor::HexArgb)
          << scrollHandleHover.name(QColor::HexArgb)
          << scrollHandlePressed.name(QColor::HexArgb);
    return parts.join(QLatin1Char('|'));
}

static QString applicationPaletteSignature(const ThemeColors &colors)
{
    const auto tokens = Theme::tokens(colors);
    const QColor tooltipFill = Style::mix(tokens.neutral.layer,
                                          tokens.accent.base,
                                          tokens.dark ? 0.16 : 0.07);
    const QColor disabledSelection = Style::mix(tokens.neutral.card,
                                                tokens.neutral.background,
                                                tokens.dark ? 0.48 : 0.35);

    QStringList parts;
    parts << QStringLiteral("palette")
          << colors.text.name(QColor::HexArgb)
          << colors.disabledText.name(QColor::HexArgb)
          << colors.error.name(QColor::HexArgb)
          << tokens.neutral.background.name(QColor::HexArgb)
          << tokens.neutral.card.name(QColor::HexArgb)
          << tokens.neutral.cardHover.name(QColor::HexArgb)
          << tooltipFill.name(QColor::HexArgb)
          << tokens.neutral.strokeSubtle.name(QColor::HexArgb)
          << tokens.neutral.fillTertiary.name(QColor::HexArgb)
          << tokens.accent.base.name(QColor::HexArgb)
          << tokens.accent.dark1.name(QColor::HexArgb)
          << tokens.accent.dark2.name(QColor::HexArgb)
          << tokens.onAccent.name(QColor::HexArgb)
          << disabledSelection.name(QColor::HexArgb);
    return parts.join(QLatin1Char('|'));
}

static constexpr const char *kPaletteStyleDirtyProperty = "_fluentPaletteStyleDirty";
static constexpr const char *kPaletteStyleFilterInstalledProperty = "_fluentPaletteStyleFilterInstalled";
static constexpr const char *kPreserveNativeMenuActionProperty = "_fluentPreserveNativeMenuAction";

static bool hasPaletteDrivenStyleSheet(QWidget *widget)
{
    if (!widget) {
        return false;
    }

    const QString styleSheet = widget->styleSheet();
    return !styleSheet.isEmpty() && styleSheet.contains(QStringLiteral("palette("));
}

static void reapplyPaletteDrivenStyleSheet(QWidget *widget)
{
    if (!widget) {
        return;
    }

    const QString styleSheet = widget->styleSheet();
    if (styleSheet.isEmpty() || !styleSheet.contains(QStringLiteral("palette("))) {
        return;
    }

    // Qt style sheets resolve palette() while the sheet is parsed. A plain
    // polish is not enough after QApplication::setPalette(), so re-apply only
    // these local palette-driven sheets.
    widget->setStyleSheet(QString());
    widget->setStyleSheet(styleSheet);
    if (QStyle *style = widget->style()) {
        style->unpolish(widget);
        style->polish(widget);
    }
    widget->update();
}

static void applyApplicationPalette(QApplication *app, const ThemeColors &colors)
{
    if (!app) {
        return;
    }

    const auto tokens = Theme::tokens(colors);
    const QColor tooltipFill = Style::mix(tokens.neutral.layer,
                                          tokens.accent.base,
                                          tokens.dark ? 0.16 : 0.07);
    const QColor disabledSelection = Style::mix(tokens.neutral.card,
                                                tokens.neutral.background,
                                                tokens.dark ? 0.48 : 0.35);
    QPalette palette = app->palette();
    palette.setColor(QPalette::Window, tokens.neutral.background);
    palette.setColor(QPalette::WindowText, colors.text);
    palette.setColor(QPalette::Base, tokens.neutral.card);
    palette.setColor(QPalette::AlternateBase, tokens.neutral.cardHover);
    palette.setColor(QPalette::ToolTipBase, tooltipFill);
    palette.setColor(QPalette::ToolTipText, colors.text);
    palette.setColor(QPalette::Text, colors.text);
    palette.setColor(QPalette::Button, tokens.neutral.card);
    palette.setColor(QPalette::ButtonText, colors.text);
    palette.setColor(QPalette::BrightText, colors.error);
    palette.setColor(QPalette::Light, tokens.neutral.cardHover);
    palette.setColor(QPalette::Midlight, tokens.accent.dark2);
    palette.setColor(QPalette::Mid, tokens.neutral.strokeSubtle);
    palette.setColor(QPalette::Dark, tokens.neutral.fillTertiary);
    palette.setColor(QPalette::Shadow, tokens.accent.dark1);
    palette.setColor(QPalette::Highlight, tokens.accent.base);
    palette.setColor(QPalette::HighlightedText, tokens.onAccent);
    palette.setColor(QPalette::Link, tokens.accent.base);
    palette.setColor(QPalette::LinkVisited, tokens.accent.dark2);
    palette.setColor(QPalette::PlaceholderText, colors.disabledText);

    palette.setColor(QPalette::Disabled, QPalette::WindowText, colors.disabledText);
    palette.setColor(QPalette::Disabled, QPalette::Text, colors.disabledText);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, colors.disabledText);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, disabledSelection);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, colors.disabledText);

    app->setPalette(palette);
}

static int refreshPaletteDrivenStyleSheet(QWidget *widget, QObject *deferredFilter)
{
    if (!widget) {
        return 0;
    }

    int polished = 0;
    if (hasPaletteDrivenStyleSheet(widget)) {
        if (widget->isVisible()) {
            // ThemeManager blocks top-level updates while themeChanged is
            // emitted, so visible local re-parsing stays visually atomic.
            reapplyPaletteDrivenStyleSheet(widget);
            widget->setProperty(kPaletteStyleDirtyProperty, false);
            ++polished;
        } else {
            widget->setProperty(kPaletteStyleDirtyProperty, true);
            if (deferredFilter && !widget->property(kPaletteStyleFilterInstalledProperty).toBool()) {
                widget->installEventFilter(deferredFilter);
                widget->setProperty(kPaletteStyleFilterInstalledProperty, true);
            }
        }
    }

    const auto children = widget->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget *child : children) {
        polished += refreshPaletteDrivenStyleSheet(child, deferredFilter);
    }
    return polished;
}

static int refreshPaletteDrivenStyleSheets(QObject *deferredFilter)
{
    int polished = 0;
    const auto topLevels = QApplication::topLevelWidgets();
    for (QWidget *w : topLevels) {
        polished += refreshPaletteDrivenStyleSheet(w, deferredFilter);
    }
    return polished;
}

static bool isLogicallyVisible(const QWidget *widget)
{
    // Before a top-level window is shown, QWidget::isVisible() is false for all
    // children. isHidden() still tells us whether the widget was explicitly
    // hidden by the control logic.
    return widget && !widget->isHidden();
}

static int widgetHintWidth(const QWidget *widget)
{
    if (!isLogicallyVisible(widget)) {
        return 0;
    }
    const int minimumWidth = widget->minimumWidth();
    if (minimumWidth > 0) {
        return minimumWidth;
    }
    return widget->sizeHint().width();
}

static int widgetHintHeight(const QWidget *widget)
{
    if (!isLogicallyVisible(widget)) {
        return 0;
    }
    const int minimumHeight = widget->minimumHeight();
    if (minimumHeight > 0) {
        return minimumHeight;
    }
    if (const auto *label = qobject_cast<const QLabel *>(widget)) {
        return label->fontMetrics().height();
    }
    return widget->sizeHint().height();
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
        // Repaint on each shared flow tick so the window's accent border animates
        // the rotating Flow gradient in sync with all other accent borders. The
        // shared driver (ThemeManager) handles start/stop and app-active pausing.
        QObject::connect(&ThemeManager::instance(), &ThemeManager::flowTick, this, [this]() { update(); });
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
        frame.radius = effectiveWindowFrameRadius();
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

        p.setRenderHint(QPainter::Antialiasing, true);

        // Flow accent border uses the shared rotation (ThemeManager::flowAngle),
        // identical to the path dialogs/toasts take via paintFluentFrame.
        // fluentFlowBorderActive already excludes the maximized case.
        if (fluentFlowBorderActive(frame)) {
            paintFluentFlowStroke(p, panelRect, radius, frame.borderWidth);
            return;
        }

        const QColor border = fluentFrameBorder(colors, frame);
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

// FluentMainWindow uses QMainWindow's menu-widget slot for the custom title bar,
// but the host itself must stay a plain widget. If it inherits QMenuBar, promoted
// Designer forms end up with both a native-looking title-bar menubar host and the
// real FluentMenuBar embedded inside it.
class TitleBarHost final : public QWidget
{
public:
    explicit TitleBarHost(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_StyledBackground, false);
        setAutoFillBackground(false);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)

        QPainter p(this);
        if (!p.isActive()) {
            return;
        }
        p.setRenderHint(QPainter::Antialiasing, true);

        const auto &colors = ThemeManager::instance().colors();
        const auto &tokens = ThemeManager::instance().tokens();
        const qreal radius = property("_fluentTitleBarRadius").toReal();
        const QRectF r = QRectF(rect()).adjusted(0.0, 0.0, -0.5, -0.5);

        QPainterPath path;
        if (radius > 0.0) {
            path.moveTo(r.left(), r.bottom());
            path.lineTo(r.left(), r.top() + radius);
            path.quadTo(r.left(), r.top(), r.left() + radius, r.top());
            path.lineTo(r.right() - radius, r.top());
            path.quadTo(r.right(), r.top(), r.right(), r.top() + radius);
            path.lineTo(r.right(), r.bottom());
            path.closeSubpath();
        } else {
            path.addRect(r);
        }

        p.fillPath(path, tokens.neutral.card);

        if (property("_fluentTitleBarBottomRule").toBool()) {
            QColor divider = tokens.neutral.strokeSubtle;
            divider.setAlpha(Theme::isDark(colors) ? 180 : 130);
            p.setPen(QPen(divider, 1.0));
            p.drawLine(QPointF(r.left(), r.bottom()), QPointF(r.right(), r.bottom()));
        }
    }
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

        const auto &tokens = ThemeManager::instance().tokens();

        // Simple opaque fill — the border overlay paints the rounded outline and,
        // on Windows 11, DWM clips the window to rounded corners on top of this.
        // On Windows 10 there is no corner clipping, so the overlay/title bar draw
        // square corners (effectiveWindowFrameRadius() == 0) to match this square
        // fill — otherwise the fill would show through as white corners (issue #17).
        p.fillRect(rect(), tokens.neutral.card);
    }
};

} // namespace

FluentMainWindow::FluentMainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_resizeBorderWidth = Style::windowMetrics().resizeBorder;

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

int FluentMainWindow::titleBarLeftHintWidth() const
{
    int width = 0;
    int visibleItems = 0;

    auto addWidth = [&](const QWidget *widget) {
        const int itemWidth = widgetHintWidth(widget);
        if (itemWidth <= 0) {
            return;
        }
        width += itemWidth;
        ++visibleItems;
    };

    addWidth(m_iconLabel);
    if (m_leftCustomWidget) {
        addWidth(m_leftSlotHost);
    }
    addWidth(m_menuBar);

    if (visibleItems > 1) {
        width += (visibleItems - 1) * 8;
    }
    return width;
}

int FluentMainWindow::titleBarRightHintWidth() const
{
    int width = 0;
    int visibleItems = 0;

    auto addWidth = [&](const QWidget *widget) {
        const int itemWidth = widgetHintWidth(widget);
        if (itemWidth <= 0) {
            return;
        }
        width += itemWidth;
        ++visibleItems;
    };

    if (m_rightCustomWidget) {
        addWidth(m_rightSlotHost);
    }

    const int buttonCount =
        (m_windowButtons.testFlag(MinimizeButton) ? 1 : 0)
        + (m_windowButtons.testFlag(MaximizeButton) ? 1 : 0)
        + (m_windowButtons.testFlag(CloseButton) ? 1 : 0);
    if (buttonCount > 0) {
        const auto wm = Style::windowMetrics();
        width += buttonCount * wm.windowButtonWidth
            + qMax(0, buttonCount - 1) * wm.windowButtonSpacing;
        ++visibleItems;
    }

    if (visibleItems > 1) {
        width += (visibleItems - 1) * 8;
    }
    return width;
}

int FluentMainWindow::titleBarContentHintHeight() const
{
    int height = 0;
    auto addHeight = [&](const QWidget *widget) {
        height = qMax(height, widgetHintHeight(widget));
    };

    addHeight(m_iconLabel);
    if (m_leftCustomWidget) {
        addHeight(m_leftSlotHost);
    }
    // FluentMenuBar is hosted inside the title-bar chrome and should follow
    // WindowMetrics::titleBarHeight instead of making the title bar grow by a
    // few pixels. Querying QMenuBar height here can also trigger an expensive
    // QMainWindow layout pass during startup.
    addHeight(m_centerCustomWidget ? m_centerCustomWidget : m_titleLabel);
    if (m_rightCustomWidget) {
        addHeight(m_rightSlotHost);
    }
    if (m_windowButtons.testFlag(MinimizeButton)
        || m_windowButtons.testFlag(MaximizeButton)
        || m_windowButtons.testFlag(CloseButton)) {
        height = qMax(height, Style::windowMetrics().windowButtonHeight);
    }

    if (height <= 0) {
        return 0;
    }
    return height + Style::windowMetrics().titleBarPaddingY * 2;
}

void FluentMainWindow::refreshTitleBarLeftWidth()
{
    if (!m_leftHost) {
        return;
    }
    const int width = titleBarLeftHintWidth();
    if (m_leftHost->minimumWidth() != width) {
        m_leftHost->setMinimumWidth(width);
    }
}

void FluentMainWindow::refreshTitleBarRightWidth()
{
    if (!m_rightHost) {
        return;
    }
    const int width = titleBarRightHintWidth();
    if (m_rightHost->minimumWidth() != width) {
        m_rightHost->setMinimumWidth(width);
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
        statusbar->setProperty("_fluentIgnoredStatusBar", true);
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

    auto *layout = qobject_cast<QVBoxLayout *>(m_frameHost->layout());
    if (!layout) {
        layout = new QVBoxLayout(m_frameHost);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
    }

    QWidget *oldWidget = m_userCentralWidget;
    while (layout->count() > 0) {
        QLayoutItem *it = layout->takeAt(0);
        if (it) {
            if (QWidget *w = it->widget()) {
                w->hide();
                w->setParent(nullptr);
            }
            delete it;
        }
    }

    if (oldWidget && oldWidget != widget) {
        oldWidget->deleteLater();
    }

    m_userCentralWidget = widget;

    if (widget) {
        widget->setParent(m_frameHost);
        layout->addWidget(widget);
        widget->show();
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
    const QIcon icon = windowIcon();
    return icon.isNull() ? qApp->windowIcon() : icon;
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
        m_centerCustomWidget->show();
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
        m_leftCustomWidget->show();
        m_leftSlotHost->show();
    } else {
        m_leftSlotHost->hide();
    }

    refreshTitleBarLeftWidth();

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
        m_rightCustomWidget->show();
        m_rightSlotHost->show();
    } else {
        m_rightSlotHost->hide();
    }

    if (m_rightHost) {
        const bool anyButtons = m_windowButtons.testFlag(MinimizeButton) || m_windowButtons.testFlag(MaximizeButton) || m_windowButtons.testFlag(CloseButton);
        m_rightHost->setVisible(anyButtons || (m_rightCustomWidget != nullptr));
    }

    refreshTitleBarRightWidth();

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

    if (!m_frameHost->layout()) {
        auto *layout = new QVBoxLayout(m_frameHost);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
    }

    QMainWindow::setCentralWidget(m_frameHost);
}

void FluentMainWindow::setFluentWindowButtons(WindowButtons buttons)
{
    m_windowButtons = buttons;

    auto syncExplicitVisibility = [](QWidget *widget, bool visible) {
        if (!widget) {
            return;
        }

        if (visible) {
            if (widget->isHidden()) {
                widget->show();
            }
        } else {
            if (!widget->isHidden()) {
                widget->hide();
            }
        }
    };

    if (m_minBtn) {
        syncExplicitVisibility(m_minBtn, m_windowButtons.testFlag(MinimizeButton));
    }
    if (m_maxBtn) {
        syncExplicitVisibility(m_maxBtn, m_windowButtons.testFlag(MaximizeButton));
    }
    if (m_closeBtn) {
        syncExplicitVisibility(m_closeBtn, m_windowButtons.testFlag(CloseButton));
    }

    if (m_windowButtonsHost) {
        const bool any = m_windowButtons.testFlag(MinimizeButton) || m_windowButtons.testFlag(MaximizeButton) || m_windowButtons.testFlag(CloseButton);
        syncExplicitVisibility(m_windowButtonsHost, any);
    }

    if (m_rightHost) {
        const bool anyButtons = m_windowButtons.testFlag(MinimizeButton) || m_windowButtons.testFlag(MaximizeButton) || m_windowButtons.testFlag(CloseButton);
        const bool anyRight = (m_rightCustomWidget != nullptr);
        syncExplicitVisibility(m_rightHost, anyButtons || anyRight);
    }

    refreshTitleBarRightWidth();

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
    m_resizeBorderWidth = qMax(1, px);
    m_resizeBorderWidthExplicit = true;
    if (!m_resizeHelper) {
        updateResizeHelperState();
    }
    if (m_resizeHelper) {
        m_resizeHelper->setBorderWidth(m_resizeBorderWidth);
    }
}

int FluentMainWindow::fluentResizeBorderWidth() const
{
    return effectiveResizeBorderWidth();
}

int FluentMainWindow::effectiveResizeBorderWidth() const
{
    return qMax(1, m_resizeBorderWidthExplicit ? m_resizeBorderWidth : Style::windowMetrics().resizeBorder);
}

QMenuBar *FluentMainWindow::menuBar() const
{
    // Note: QMainWindow::menuBar() is not virtual; this is a convenience shim.
    auto *self = const_cast<FluentMainWindow *>(this);
    self->adoptNativeMenuBarIfNeeded();
    self->ensureTitleBar();

    if (!self->m_menuBar) {
        self->setFluentMenuBar(new FluentMenuBar(self->m_leftHost ? self->m_leftHost : self->m_titleBarHost));
    }

    return self->m_menuBar;
}

void FluentMainWindow::adoptNativeMenuBarIfNeeded()
{
    auto *nativeMenuBar = qobject_cast<QMenuBar *>(QMainWindow::menuWidget());
    if (!nativeMenuBar
        || nativeMenuBar == m_titleBarHost
        || nativeMenuBar == m_menuBar
        || nativeMenuBar == m_adoptedSource) {
        return;
    }

    setMenuBar(nativeMenuBar);
}

void FluentMainWindow::setMenuBar(QMenuBar *menuBar)
{
    auto clearAdoptedSource = [this]() {
        if (!m_adoptedSource) {
            m_adoptedSourceInNativeMenuSlot = false;
            return;
        }

        m_adoptedSource->removeEventFilter(this);
        m_adoptedSource->hide();
        if (m_adoptedSourceInNativeMenuSlot) {
            // The menu bar was installed by QMainWindow::setMenuBar() before we
            // could intercept it. Leave ownership with QMainWindow; reparenting
            // or deleting it here can leave QMainWindowLayout with a stale item.
            m_adoptedSource->setFixedHeight(0);
        } else {
            m_adoptedSource->deleteLater();
        }
        m_adoptedSource = nullptr;
        m_adoptedSourceInNativeMenuSlot = false;
    };

    if (!menuBar) {
        clearAdoptedSource();
        if (m_titleBarHost) {
            setFluentMenuBar(nullptr);
        } else {
            m_menuBar = nullptr;
        }
        return;
    }

    if (auto *fluent = qobject_cast<FluentMenuBar *>(menuBar)) {
        // Already a FluentMenuBar (e.g. Designer promoted ui->menubar). Drop any
        // previously adopted plain QMenuBar and install the fluent one directly.
        ensureTitleBar();
        if (m_adoptedSource && m_adoptedSource != menuBar) {
            clearAdoptedSource();
        }
        setFluentMenuBar(fluent);
        return;
    }

    // Adoption path for a plain QMenuBar (the common case when this is called
    // from uic-generated setupUi). The key observation: uic emits
    //     menubar = new QMenuBar(MainWindow);
    //     MainWindow->setMenuBar(menubar);   // <-- we are here, menubar is EMPTY
    //     menuFile = new QMenu(menubar);
    //     ...
    //     menubar->addAction(menuFile->menuAction());   // <-- happens AFTER
    // The old implementation copied the (still-empty) action list once and then
    // deleteLater()'d the source, so every menu added afterwards was attached to
    // a dangling object and never made it into the title bar.
    //
    // The new strategy: build the FluentMenuBar, move whatever is already on
    // the source, then keep the source alive (hidden) and install an event
    // filter that moves subsequent ActionAdded events into the FluentMenuBar.
    // That preserves uic's ordering without leaving one QAction owned by both
    // the hidden source menubar and the visible FluentMenuBar.
    const bool sourceInNativeMenuSlot = (QMainWindow::menuWidget() == menuBar);

    const QList<QAction *> initialActions = menuBar->actions();
    if (m_adoptedSource && m_adoptedSource != menuBar) {
        clearAdoptedSource();
    }
    m_adoptedSource = menuBar;
    m_adoptedSourceInNativeMenuSlot = sourceInNativeMenuSlot;
    menuBar->setVisible(false);
    menuBar->setFixedHeight(0);
    menuBar->installEventFilter(this);
    if (sourceInNativeMenuSlot && m_titleBarHost) {
        // QMainWindow::setMenuBar() replaced FluentMainWindow's title-bar host
        // with the uic-created source menubar. Put our title-bar host back into
        // the native slot first; our event filter below keeps Qt's deleteLater()
        // for the source menubar from destroying the object that user code still
        // knows as ui->menubar.
        auto forgetTitleBarPointers = [this]() {
            m_titleBarHost = nullptr;
            m_leftHost = nullptr;
            m_leftSlotHost = nullptr;
            m_centerHost = nullptr;
            m_rightHost = nullptr;
            m_rightSlotHost = nullptr;
            m_windowButtonsHost = nullptr;
            m_titleLabel = nullptr;
            m_iconLabel = nullptr;
            m_menuBar = nullptr;
            m_minBtn = nullptr;
            m_maxBtn = nullptr;
            m_closeBtn = nullptr;
        };
        forgetTitleBarPointers();
        ensureTitleBar();
        // Keep the source menubar parented to the window. Reparenting an object
        // that has already passed through QMainWindowLayout can trip Qt's
        // internal menu/style state once the window is actually shown.
        m_adoptedSourceInNativeMenuSlot = false;
    } else {
        // In the normal uic fallback path the constructor has not created a
        // native title-bar widget yet, so QMainWindow::setMenuBar() can complete
        // without tripping over a non-QMenuBar menu widget. Now that the source
        // menubar is hidden and protected by our event filter, create/restore the
        // Fluent title bar and replace the native menu slot with it.
        ensureTitleBar();
        if (sourceInNativeMenuSlot) {
            m_adoptedSourceInNativeMenuSlot = false;
        }
    }

    auto *fluent = new FluentMenuBar(m_leftHost ? m_leftHost : m_titleBarHost);
    auto moveActionToFluent = [this, menuBar, fluent](QAction *action, QAction *before = nullptr) {
        if (!action) {
            return;
        }
        action->setProperty(kPreserveNativeMenuActionProperty, true);
        if (before && fluent->actions().contains(before)) {
            fluent->insertAction(before, action);
        } else {
            fluent->addAction(action);
        }
        if (menuBar->actions().contains(action)) {
            m_movingAdoptedSourceAction = true;
            menuBar->removeAction(action);
            m_movingAdoptedSourceAction = false;
        }
    };
    for (QAction *a : initialActions) {
        // QMenuBar::addAction(QAction*) accepts both leaf actions and menu actions
        // (QMenu::menuAction()), so we don't need to special-case QMenu.
        moveActionToFluent(a);
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
    QElapsedTimer timer;
    timer.start();
    qint64 last = 0;
    auto mark = [&](const QString &step) {
        const qint64 now = timer.elapsed();
        qInfo().noquote() << QStringLiteral("[DemoStartup] FluentMainWindow::setFluentMenuBar %1 +%2 ms, total %3 ms")
                                 .arg(step)
                                 .arg(now - last)
                                 .arg(now);
        last = now;
    };

    ensureTitleBar();
    mark(QStringLiteral("ensure title bar"));
    if (m_menuBar == menuBar) {
        return;
    }

    if (m_menuBar) {
        if (m_leftHost && m_leftHost->layout()) {
            m_leftHost->layout()->removeWidget(m_menuBar);
        }
        m_menuBar->hide();
        m_menuBar->setParent(nullptr);
        m_menuBar->deleteLater();
    }
    mark(QStringLiteral("remove old"));

    m_menuBar = menuBar;
    if (!m_menuBar) {
        refreshTitleBarLeftWidth();
        updateTitleBarContent();
        mark(QStringLiteral("clear menu bar"));
        return;
    }

    m_menuBar->setParent(m_leftHost ? m_leftHost : m_titleBarHost);
    m_menuBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    m_menuBar->setFixedHeight(Style::windowMetrics().windowButtonHeight);
    m_menuBar->setMinimumWidth(m_menuBar->minimumSizeHint().width());
    mark(QStringLiteral("parent and size policy"));

    if (m_leftHost) {
        auto *layout = qobject_cast<QHBoxLayout *>(m_leftHost->layout());
        if (layout) {
            layout->addWidget(m_menuBar, 1);
        }
    }
    mark(QStringLiteral("add to left host"));

    refreshTitleBarLeftWidth();
    mark(QStringLiteral("left width"));
    updateTitleBarContent();
    mark(QStringLiteral("title content"));
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
    adoptNativeMenuBarIfNeeded();
    ensureTitleBar();

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
    if (event && event->type() == QEvent::Show) {
        if (auto *widget = qobject_cast<QWidget *>(watched)) {
            if (widget->property(kPaletteStyleDirtyProperty).toBool()) {
                widget->setProperty(kPaletteStyleDirtyProperty, false);
                reapplyPaletteDrivenStyleSheet(widget);
            }
        }
    }

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

                // Defensive fallback for a QMenuBar installed through
                // QMainWindow::setMenuBar(), bypassing our non-virtual shim.
                // The documented Designer path promotes the root widget so uic
                // calls FluentMainWindow::setMenuBar() directly; this hook exists
                // only to keep accidental static dispatch from leaving a native
                // menubar in the main-window layout.
                if (auto *mb = qobject_cast<QMenuBar *>(ce->child())) {
                    if (mb != m_titleBarHost && mb != m_menuBar && mb != m_adoptedSource) {
                        QPointer<QMenuBar> mbRef(mb);
                        QPointer<FluentMainWindow> selfRef(this);
                        // Defer to next event-loop tick so uic's `menubar->addAction(...)`
                        // calls (which follow setMenuBar(menubar)) finish first.
                        QTimer::singleShot(0, this, [selfRef, mbRef]() {
                            if (!selfRef || !mbRef) {
                                return;
                            }
                            if (mbRef.data() == selfRef->m_menuBar
                                || mbRef.data() == selfRef->m_adoptedSource) {
                                return;
                            }
                            selfRef->setMenuBar(mbRef.data());
                        });
                    }
                }

                if (auto *sb = qobject_cast<QStatusBar *>(ce->child())) {
                    QPointer<QStatusBar> sbRef(sb);
                    QPointer<FluentMainWindow> selfRef(this);
                    QTimer::singleShot(0, this, [selfRef, sbRef]() {
                        if (!selfRef || !sbRef) {
                            return;
                        }
                        if (sbRef->property("_fluentIgnoredStatusBar").toBool()) {
                            return;
                        }
                        selfRef->QMainWindow::setStatusBar(nullptr);
                        if (sbRef) {
                            sbRef->hide();
                            sbRef->deleteLater();
                        }
                    });
                }
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

    // Move QAction additions from the adopted source QMenuBar (the original
    // ui->menubar) into the live FluentMenuBar in the title bar. This is what
    // makes uic's `menubar->addAction(...)` calls (issued *after* setMenuBar())
    // show up in the title bar without also keeping those actions on the hidden
    // source menubar.
    if (m_adoptedSource && watched == m_adoptedSource.data()) {
        if (event->type() == QEvent::DeferredDelete) {
            return true;
        }
        if (m_movingAdoptedSourceAction) {
            return QMainWindow::eventFilter(watched, event);
        }
        if (!m_menuBar) {
            return QMainWindow::eventFilter(watched, event);
        }
        if (event->type() == QEvent::ActionAdded || event->type() == QEvent::ActionRemoved) {
            auto *ae = static_cast<QActionEvent *>(event);
            QAction *act = ae->action();
            if (act) {
                if (event->type() == QEvent::ActionAdded) {
                    QAction *before = ae->before();
                    act->setProperty(kPreserveNativeMenuActionProperty, true);
                    if (before && m_menuBar->actions().contains(before)) {
                        m_menuBar->insertAction(before, act);
                    } else {
                        m_menuBar->addAction(act);
                    }
                    m_movingAdoptedSourceAction = true;
                    m_adoptedSource->removeAction(act);
                    m_movingAdoptedSourceAction = false;
                } else {
                    m_menuBar->removeAction(act);
                }
            }
        }
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
                    // When the window is maximized, do NOT initiate a system move
                    // on press: on Windows 11, startSystemMove() while maximized
                    // immediately restores the window and enters a modal move
                    // loop, which swallows the second click of an incoming
                    // double-click. That manifests as "the first double-click on
                    // the title bar does nothing" — the window silently restores
                    // on the first press, then the dblclick toggles it back to
                    // maximized via showMaximized() (issue #10). Skip the move
                    // here so the dblclick branch below can handle the toggle.
                    if (isMaximized() || isFullScreen()) {
                        return QMainWindow::eventFilter(watched, event);
                    }
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
        const QPoint globalPos(GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam));
        const QPoint localPos = mapFromGlobal(globalPos);

        const int x = localPos.x();
        const int y = localPos.y();
        const int w = width();
        const int h = height();
        const int b = effectiveResizeBorderWidth();

        // Resize borders are meaningless when maximized/fullscreen; skip them so
        // they don't shadow the title bar's HTCAPTION area. But we still want to
        // report HTCAPTION for the title bar itself even when maximized, so that
        // the OS handles native drag-to-restore and double-click-to-restore
        // (DefWindowProc's WM_NCLBUTTONDBLCLK on HTCAPTION). Issue #10.
        const bool sizable = !isMaximized() && !isFullScreen();

        const bool left = sizable && x >= 0 && x < b;
        const bool right = sizable && x <= w && x > w - b;
        const bool top = sizable && y >= 0 && y < b;
        const bool bottom = sizable && y <= h && y > h - b;

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
    QElapsedTimer timer;
    timer.start();
    qint64 last = 0;
    auto mark = [&](const QString &step) {
        const qint64 now = timer.elapsed();
        qInfo().noquote() << QStringLiteral("[ThemeSwitch] FluentMainWindow::applyThemeToApplication %1 +%2 ms, total %3 ms")
                                 .arg(step)
                                 .arg(now - last)
                                 .arg(now);
        last = now;
    };

    qInfo().noquote() << QStringLiteral("[ThemeSwitch] FluentMainWindow::applyThemeToApplication begin visible=%1, windowTitle=\"%2\"")
                             .arg(isVisible() ? QStringLiteral("true") : QStringLiteral("false"))
                             .arg(windowTitle());

    // Keep border + trace animation in sync with theme.
    if (isVisible()) {
        m_border.onThemeChanged();
    } else {
        m_border.syncFromTheme();
    }
    syncBorderVisualState();
    mark(QStringLiteral("border state"));

    if (auto *app = qobject_cast<QApplication *>(QCoreApplication::instance())) {
        const auto &colors = ThemeManager::instance().colors();
        static QString s_lastApplicationPaletteSignature;
        const QString paletteSignature = applicationPaletteSignature(colors);
        if (s_lastApplicationPaletteSignature == paletteSignature) {
            mark(QStringLiteral("application palette skipped"));
        } else {
            QElapsedTimer paletteTimer;
            paletteTimer.start();
            applyApplicationPalette(app, colors);
            s_lastApplicationPaletteSignature = paletteSignature;
            qInfo().noquote() << QStringLiteral("[ThemeSwitch] QApplication::setPalette +%1 ms")
                                     .arg(paletteTimer.elapsed());
            mark(QStringLiteral("application palette"));

            QElapsedTimer polishTimer;
            polishTimer.start();
            const int polished = refreshPaletteDrivenStyleSheets(this);
            qInfo().noquote() << QStringLiteral("[ThemeSwitch] palette-driven stylesheet polish %1 widgets +%2 ms")
                                     .arg(polished)
                                     .arg(polishTimer.elapsed());
            mark(QStringLiteral("palette-driven stylesheet polish"));
        }

        static QString s_lastApplicationStyleSignature;
        const QString styleSignature = applicationStyleSignature(colors);
        if (s_lastApplicationStyleSignature == styleSignature) {
            qInfo().noquote() << QStringLiteral("[ThemeSwitch] QApplication::setStyleSheet skipped, base signature unchanged");
            mark(QStringLiteral("base stylesheet skipped"));
        } else {
            const QString next = Theme::baseStyleSheet(colors);
            mark(QStringLiteral("base stylesheet generated"));
            if (app->styleSheet() != next) {
                QElapsedTimer styleTimer;
                styleTimer.start();
                app->setStyleSheet(next);
                s_lastApplicationStyleSignature = styleSignature;
                qInfo().noquote() << QStringLiteral("[ThemeSwitch] QApplication::setStyleSheet +%1 ms")
                                         .arg(styleTimer.elapsed());
            } else {
                s_lastApplicationStyleSignature = styleSignature;
                qInfo().noquote() << QStringLiteral("[ThemeSwitch] QApplication::setStyleSheet skipped, unchanged");
            }
        }
    }
    mark(QStringLiteral("application stylesheet"));

    updateTitleBarContent();
    mark(QStringLiteral("title bar content"));

    updateWindowControlIcons();
    mark(QStringLiteral("window control icons"));

    updateFrameHost();
    mark(QStringLiteral("frame host"));

#ifdef Q_OS_WIN
    applyWindowsDwmAttributes();
    mark(QStringLiteral("DWM attributes"));
#endif

    qInfo().noquote() << QStringLiteral("[ThemeSwitch] FluentMainWindow::applyThemeToApplication done at %1 ms")
                             .arg(timer.elapsed());
}

void FluentMainWindow::ensureTitleBar()
{
    if (m_titleBarHost) {
        return;
    }

    QElapsedTimer timer;
    timer.start();
    qint64 last = 0;
    auto mark = [&](const QString &step) {
        const qint64 now = timer.elapsed();
        qInfo().noquote() << QStringLiteral("[DemoStartup] FluentMainWindow::ensureTitleBar %1 +%2 ms, total %3 ms")
                                 .arg(step)
                                 .arg(now - last)
                                 .arg(now);
        last = now;
    };

    auto *titleBarHost = new TitleBarHost();
    m_titleBarHost = titleBarHost;
    m_titleBarHost->setObjectName("FluentTitleBarHost");
    m_titleBarHost->setParent(this);
    m_titleBarHost->installEventFilter(this);
    connect(m_titleBarHost, &QObject::destroyed, this, [this, titleBarHost]() {
        if (m_titleBarHost != titleBarHost) {
            return;
        }
        m_titleBarHost = nullptr;
        m_leftHost = nullptr;
        m_leftSlotHost = nullptr;
        m_centerHost = nullptr;
        m_rightHost = nullptr;
        m_rightSlotHost = nullptr;
        m_windowButtonsHost = nullptr;
        m_titleLabel = nullptr;
        m_iconLabel = nullptr;
        m_menuBar = nullptr;
        m_minBtn = nullptr;
        m_maxBtn = nullptr;
        m_closeBtn = nullptr;
    });
    mark(QStringLiteral("host"));

    const auto wm = Style::windowMetrics();
    m_titleBarHost->setMinimumHeight(0);
    // Establish the common title-bar height before the host is handed to
    // QMainWindow::setMenuWidget(). Changing the menu-widget height after it is
    // installed can force an expensive main-window layout pass on startup.
    m_titleBarHost->setFixedHeight(wm.titleBarHeight);

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
    mark(QStringLiteral("left zone"));

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
    QFont titleFont = m_titleLabel->font();
    titleFont.setWeight(QFont::DemiBold);
    m_titleLabel->setFont(titleFont);
    centerLayout->addWidget(m_titleLabel);

    layout->addWidget(m_centerHost, 0, Qt::AlignCenter);

    layout->addStretch(1);
    mark(QStringLiteral("center zone"));

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
    mark(QStringLiteral("right zone"));

    setMenuWidget(m_titleBarHost);
    mark(QStringLiteral("set menu widget"));

    // Apply initial visibility flags without forcing `show()` on freshly created
    // child widgets. Calling show/setVisible(true) here makes Qt polish the
    // caption buttons immediately under the global stylesheet, which dominates
    // demo startup. Widgets that are not explicitly hidden will appear naturally
    // when the title bar host is shown.
    const bool initialMinVisible = m_windowButtons.testFlag(MinimizeButton);
    const bool initialMaxVisible = m_windowButtons.testFlag(MaximizeButton);
    const bool initialCloseVisible = m_windowButtons.testFlag(CloseButton);
    if (!initialMinVisible) {
        m_minBtn->hide();
    }
    if (!initialMaxVisible) {
        m_maxBtn->hide();
    }
    if (!initialCloseVisible) {
        m_closeBtn->hide();
    }
    const bool anyInitialButtons = initialMinVisible || initialMaxVisible || initialCloseVisible;
    if (!anyInitialButtons) {
        m_windowButtonsHost->hide();
        if (!m_rightCustomWidget) {
            m_rightHost->hide();
        }
    }
    refreshTitleBarRightWidth();
    mark(QStringLiteral("window buttons"));

    updateTitleBarContent();
    mark(QStringLiteral("title content"));
    updateWindowControlIcons();
    mark(QStringLiteral("window control icons"));
}

void FluentMainWindow::updateTitleBarContent()
{
    QElapsedTimer traceTimer;
    traceTimer.start();
    qint64 traceLast = 0;
    QStringList traceParts;
    auto traceMark = [&](const QString &step) {
        const qint64 now = traceTimer.elapsed();
        traceParts << QStringLiteral("%1 +%2 ms, total %3 ms")
                          .arg(step)
                          .arg(now - traceLast)
                          .arg(now);
        traceLast = now;
    };
    auto flushSlowTrace = [&]() {
        if (traceTimer.elapsed() >= 20) {
            qInfo().noquote() << QStringLiteral("[DemoStartup] FluentMainWindow::updateTitleBarContent slow: %1")
                                     .arg(traceParts.join(QStringLiteral(" | ")));
        }
    };

    if (!m_titleBarHost) {
        return;
    }

    const auto &colors = ThemeManager::instance().colors();
    traceMark(QStringLiteral("theme"));

    refreshTitleBarLeftWidth();
    refreshTitleBarRightWidth();
    traceMark(QStringLiteral("refresh widths"));

    // Elide title to avoid pushing menu/buttons.
    const int leftW = m_leftHost ? m_leftHost->minimumWidth() : 0;
    const int rightW = m_rightHost ? m_rightHost->minimumWidth() : 0;
    const int margins = Style::windowMetrics().titleBarPaddingX * 2 + 24;
    const int maxTitleWidth = qMax(0, width() - leftW - rightW - margins);
    const QString effectiveTitle = m_hasTitleOverride ? m_titleOverride : windowTitle();
    const QString titleText = fontMetrics().elidedText(effectiveTitle, Qt::ElideRight, maxTitleWidth);

    m_titleLabel->setMaximumWidth(maxTitleWidth);
    if (m_titleLabel->text() != titleText) {
        m_titleLabel->setText(titleText);
    }
    QPalette titlePalette = m_titleLabel->palette();
    if (titlePalette.color(QPalette::WindowText) != colors.text) {
        titlePalette.setColor(QPalette::WindowText, colors.text);
        m_titleLabel->setPalette(titlePalette);
    }
    traceMark(QStringLiteral("title label"));

    QIcon icon;
    if (m_hasIconOverride) {
        icon = m_iconOverride;
    } else {
        icon = windowIcon().isNull() ? qApp->windowIcon() : windowIcon();
    }
    const qint64 iconKey = icon.cacheKey();
    if (m_iconLabel->property("_fluentTitleBarIconKey").toLongLong() != iconKey) {
        m_iconLabel->setProperty("_fluentTitleBarIconKey", iconKey);
        m_iconLabel->setPixmap(icon.pixmap(16, 16));
    }
    traceMark(QStringLiteral("icon"));

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
    traceMark(QStringLiteral("margins and frame"));

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

    if (!titleBarVisible) {
        if (!m_titleBarHost->isHidden()) {
            m_titleBarHost->hide();
        }
        if (m_titleBarHost->height() != 0) {
            m_titleBarHost->setFixedHeight(0);
        }
        traceMark(QStringLiteral("hide"));
        flushSlowTrace();
        return;
    }
    if (isVisible() && m_titleBarHost->isHidden()) {
        m_titleBarHost->show();
    }
    traceMark(QStringLiteral("visibility"));

    // The frame host draws the outer border with the effective window radius
    // (0 on platforms that cannot clip corners, e.g. Windows 10 -- see issue #17).
    // The title bar is inside the 1px inset, so its effective inner radius is slightly smaller.
    const qreal innerRadius = maximizedOrFullscreen ? 0.0 : qMax(0.0, effectiveWindowFrameRadius() - 1.0);
    const int frameRadius = static_cast<int>(innerRadius);
    if (m_titleBarHost->property("_fluentTitleBarBottomRule").toBool() != anyButtonsVisible) {
        m_titleBarHost->setProperty("_fluentTitleBarBottomRule", anyButtonsVisible);
    }
    if (!qFuzzyCompare(m_titleBarHost->property("_fluentTitleBarRadius").toReal(), static_cast<qreal>(frameRadius))) {
        m_titleBarHost->setProperty("_fluentTitleBarRadius", frameRadius);
    }
    m_titleBarHost->update();
    traceMark(QStringLiteral("host properties"));

    int desiredH = 0;
    if (anyButtonsVisible || anyMenu) {
        desiredH = Style::windowMetrics().titleBarHeight;
    } else {
        const int contentHintH = titleBarContentHintHeight();
        desiredH = contentHintH;
    }
    if (m_titleBarHost->height() != desiredH) {
        m_titleBarHost->setFixedHeight(desiredH);
    }
    traceMark(QStringLiteral("height"));

    // NOTE: We intentionally do NOT set a QRegion mask on the title bar.
    // QRegion masks are pixel-based and produce visible jaggies on rounded corners.
    // On Windows 11, DWM DWMWA_WINDOW_CORNER_PREFERENCE = DWMWCP_ROUND provides
    // smooth system-level corner clipping; the title bar paints a rounded top.
    // On Windows 10 DWM does not round, so effectiveWindowFrameRadius() returns 0
    // and the title bar paints square corners flush with the window edge (no white
    // corner artifacts — issue #17).
    flushSlowTrace();
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
        }
        m_resizeHelper->setBorderWidth(effectiveResizeBorderWidth());
        m_resizeHelper->setEnabled(true);
    } else {
        if (m_resizeHelper) {
            m_resizeHelper->setEnabled(false);
        }
    }
}

} // namespace Fluent
