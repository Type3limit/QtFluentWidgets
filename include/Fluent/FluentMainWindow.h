#pragma once

#include <QIcon>
#include <QMainWindow>
#include <QtGlobal>

#include "Fluent/FluentBorderEffect.h"

class QMenuBar;
class QStatusBar;
class QToolBar;

class QLabel;
class QShowEvent;
class QVariantAnimation;

namespace Fluent { class FluentMenuBar; class FluentToolButton; class FluentResizeHelper; }

namespace Fluent {

class FluentMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit FluentMainWindow(QWidget *parent = nullptr);

    // Hide QMainWindow::setCentralWidget to inject a border host that guarantees the
    // accent border is visible (even when the central widget fills the whole area).
    void setCentralWidget(QWidget *widget);

    // Fluent Design: this window uses FluentMenuBar embedded into the title bar.
    // Traditional QMainWindow tool bars / status bars are intentionally not supported.
    // These hide (non-virtual) QMainWindow methods when called on FluentMainWindow.
    void addToolBar(QToolBar *toolbar);
    void addToolBar(Qt::ToolBarArea area, QToolBar *toolbar);
    void insertToolBar(QToolBar *before, QToolBar *toolbar);
    void setStatusBar(QStatusBar *statusbar);

    enum WindowButton {
        MinimizeButton = 0x1,
        MaximizeButton = 0x2,
        CloseButton = 0x4,
    };
    Q_DECLARE_FLAGS(WindowButtons, WindowButton)
    Q_FLAG(WindowButtons)

    void setFluentWindowButtons(WindowButtons buttons);
    WindowButtons fluentWindowButtons() const;

    // Convenience: make FluentMenuBar the default menu bar for FluentMainWindow.
    // These hide (non-virtual) QMainWindow methods when called on FluentMainWindow.
    QMenuBar *menuBar() const;
    void setMenuBar(QMenuBar *menuBar);

    FluentMenuBar *fluentMenuBar() const;

    void setFluentTitleBarEnabled(bool enabled);
    bool fluentTitleBarEnabled() const;

    // Title bar customization (like FluentDialog):
    // - By default it mirrors windowTitle()/windowIcon().
    // - You can override displayed title/icon without changing the OS-level window title.
    void setFluentTitleBarTitle(const QString &title);
    void clearFluentTitleBarTitle();
    QString fluentTitleBarTitle() const;

    void setFluentTitleBarIcon(const QIcon &icon);
    void clearFluentTitleBarIcon();
    QIcon fluentTitleBarIcon() const;

    // Replace the center content area of the title bar.
    // Passing nullptr restores the default centered title label.
    void setFluentTitleBarCenterWidget(QWidget *widget);
    QWidget *fluentTitleBarCenterWidget() const;

    // Insert custom widgets into the title bar.
    // - Left widget lives between the window icon and the menu bar.
    // - Right widget lives next to the window control buttons.
    void setFluentTitleBarLeftWidget(QWidget *widget);
    QWidget *fluentTitleBarLeftWidget() const;

    void setFluentTitleBarRightWidget(QWidget *widget);
    QWidget *fluentTitleBarRightWidget() const;

    void setFluentMenuBar(FluentMenuBar *menuBar);

    // Optional window resize support for frameless mode (implemented via mouse hit-test + setGeometry).
    // Enabled by default when Fluent title bar is enabled.
    void setFluentResizeEnabled(bool enabled);
    bool fluentResizeEnabled() const;

    void setFluentResizeBorderWidth(int px);
    int fluentResizeBorderWidth() const;

protected:
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

#ifdef Q_OS_WIN
    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    #else
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
    #endif
#endif

private slots:
    void applyThemeToApplication();

private:
    void ensureTitleBar();
    void updateTitleBarContent();
    void updateWindowControlIcons();
    void ensureCentralBorderHost();

    void syncBorderVisualState();

#ifdef Q_OS_WIN
    void applyWindowsDwmAttributes();
#endif

    void updateResizeHelperState();

    bool m_titleBarEnabled = true;

    WindowButtons m_windowButtons = WindowButtons(MinimizeButton | MaximizeButton | CloseButton);

    QWidget *m_titleBarHost = nullptr;
    QWidget *m_leftHost = nullptr;
    QWidget *m_leftSlotHost = nullptr;
    QWidget *m_centerHost = nullptr;
    QWidget *m_rightHost = nullptr;
    QWidget *m_rightSlotHost = nullptr;
    QWidget *m_windowButtonsHost = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_iconLabel = nullptr;

    QWidget *m_centerCustomWidget = nullptr;
    QWidget *m_leftCustomWidget = nullptr;
    QWidget *m_rightCustomWidget = nullptr;
    QString m_titleOverride;
    bool m_hasTitleOverride = false;
    QIcon m_iconOverride;
    bool m_hasIconOverride = false;

    FluentMenuBar *m_menuBar = nullptr;
    FluentToolButton *m_minBtn = nullptr;
    FluentToolButton *m_maxBtn = nullptr;
    FluentToolButton *m_closeBtn = nullptr;

    bool m_resizeEnabled = true;
    FluentResizeHelper *m_resizeHelper = nullptr;

    QWidget *m_borderOverlay = nullptr;

    QWidget *m_borderMarqueeOverlay = nullptr;
    FluentBorderEffect m_border{this};
    bool m_initialTracePlayed = false;
    bool m_initialTracePending = false;

    QWidget *m_centralBorderHost = nullptr;
    QWidget *m_centralClipHost = nullptr;
    QWidget *m_userCentralWidget = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FluentMainWindow::WindowButtons)

} // namespace Fluent
