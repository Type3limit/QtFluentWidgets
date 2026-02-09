# Windows / Menus / Dialogs

## Widget list

- `FluentMainWindow` (include: `Fluent/FluentMainWindow.h`)
- `FluentMenuBar` (include: `Fluent/FluentMenuBar.h`)
- `FluentMenu` (include: `Fluent/FluentMenu.h`)
- `FluentToolBar` (include: `Fluent/FluentToolBar.h`)
- `FluentStatusBar` (include: `Fluent/FluentStatusBar.h`)
- `FluentDialog` (include: `Fluent/FluentDialog.h`)
- `FluentMessageBox` (include: `Fluent/FluentMessageBox.h`)
- `FluentToast` (include: `Fluent/FluentToast.h`)
- `FluentResizeHelper` (include: `Fluent/FluentResizeHelper.h`)

Demo pages: Windows (`demo/pages/PageWindows.cpp`) and Overview (`demo/pages/PageOverview.cpp`).

## FluentMainWindow

```cpp
#include "Fluent/FluentMainWindow.h"

class MainWindow : public Fluent::FluentMainWindow {
public:
    using Fluent::FluentMainWindow::FluentMainWindow;
};
```

Purpose: main window with a Fluent title bar (menu bar embedded into the title bar) and optional frameless resize.

Key APIs:

- `setFluentTitleBarEnabled(bool)`
- `setFluentWindowButtons(WindowButtons)`
- Title bar content:
    - `setFluentTitleBarTitle()` / `clearFluentTitleBarTitle()`
    - `setFluentTitleBarIcon()` / `clearFluentTitleBarIcon()`
    - `setFluentTitleBarCenterWidget(QWidget*)`
    - `setFluentTitleBarLeftWidget(QWidget*)` / `setFluentTitleBarRightWidget(QWidget*)`
- Menu bar: `setFluentMenuBar(FluentMenuBar*)` / `fluentMenuBar()`
- Resize: `setFluentResizeEnabled(bool)`, `setFluentResizeBorderWidth(int)`

Demo: Windows / Overview.

### Default behavior and internal structure

- **Does not use QMainWindow's ToolBar/StatusBar areas**: calling `addToolBar()` / `insertToolBar()` / `setStatusBar()` will hide and `deleteLater()` the passed objects. `FluentMainWindow` expects you to handle layout inside the central area.
- **Central widget is wrapped**: on the first `setCentralWidget()`, the window creates internal hosts (central border host + clip host) and reparents your widget into that host.
    - The internal host reserves ~1px space (left/right/bottom) for border and rounded clipping.
    - Rounded clipping uses `setMask()`: when the title bar is visible, only the bottom two corners are clipped; when the title bar is collapsed/hidden, all four corners are clipped to avoid seams.
- **The title bar is installed as `menuWidget()`**: the title bar host is placed at the top via `setMenuWidget()`.

### Title bar content: overrides / slots / auto-collapse

- `setFluentTitleBarTitle()` / `setFluentTitleBarIcon()` override the displayed title/icon; `clear...()` falls back to `windowTitle()` / `windowIcon()` (if window icon is empty, it falls back to `qApp->windowIcon()`).
- `setFluentTitleBarCenterWidget(QWidget*)`:
    - non-null: hides the default title `QLabel` and places your widget in the center area;
    - null: restores the default title label.
- `setFluentTitleBarLeftWidget(QWidget*)` / `setFluentTitleBarRightWidget(QWidget*)`:
    - act as slot containers (layout spacing=8); passing null hides that slot.
    - the right host is shown when any window button is visible or the right slot is non-empty.
- **Auto-collapse**: the title bar collapses to height 0 (so the central content reaches the top) iff *none* of the following exists:
    - any window button is visible (min/max/close);
    - any custom slot widget exists (left/center/right);
    - a menu bar exists;
    - a title/icon override is set.

### Title eliding and adaptive height

- The title text is elided with `Qt::ElideRight` based on **reserved widths** of left/right areas (max of `minimumWidth` and `sizeHint`) to avoid transient squeezes that would push `QMenuBar` into Qt's native overflow (`qt_menubar_ext_button`).
- Title bar height is content-adaptive:
    - if window buttons are present, height is at least `Style::windowMetrics().titleBarHeight`;
    - it also takes the max with the layout `sizeHint().height()` so custom widgets (e.g. a `FluentLineEdit` in the title bar) are not clipped.

### Drag / double click behavior

- Drag to move: on left press in the title bar area, if the hit target is an interactive widget (buttons/focusable widgets/menu bar area), the event goes to that widget; otherwise the window triggers `startSystemMove()`.
- Double click toggles maximize/restore and updates button glyphs.
- When the title bar is collapsed/hidden, the window installs event filters on newly added child widgets so you can still drag from **non-interactive regions** (it will not steal events from buttons, focusable widgets, or selectable-text labels).

### Menu bar integration and migration

- `menuBar()` here is a convenience shim (Qt's `QMainWindow::menuBar()` is not virtual). The first call creates and installs a `FluentMenuBar`.
- `setMenuBar(QMenuBar*)`:
    - passing a `FluentMenuBar*`: installs it directly.
    - passing a native `QMenuBar*`: best-effort migrates its actions/menus into a new `FluentMenuBar` (preserving the original `QAction` instances), and `deleteLater()` the old bar if it belongs to this window.
- To reduce native overflow, the title bar menu bar uses `QSizePolicy::Minimum` and sets `minimumWidth(minimumSizeHint().width())`.

### Windows: frameless resize (WM_NCHITTEST)

- With the Fluent title bar (frameless) enabled, Windows handles `WM_NCHITTEST`:
    - uses `Style::windowMetrics().resizeBorder` as the hit-test thickness and returns `HTLEFT/HTTOP/HTBOTTOMRIGHT/...` for edge resize.
    - in the title bar area: if the cursor is over interactive widgets (buttons/focusable widgets/menu bar), returns `HTCLIENT` so clicks reach the widgets; otherwise returns `HTCAPTION` so you can drag.

### Theme coupling and border

- On theme changes, the window updates the global `QApplication` stylesheet with `Theme::baseStyleSheet(...)`, with an equality check to avoid redundant repolish.
- Frame/title-bar border color follows `ThemeManager::accentBorderEnabled()`; the accent trace animation is drawn by an internal overlay.

---

## FluentMenuBar / FluentMenu

Purpose: Fluent-styled menu bar and menus (hover highlight + popup animations; submenus stay Fluent).

Key APIs:

- `FluentMenuBar::addFluentMenu(const QString&) -> FluentMenu*`
- `FluentMenu::addFluentMenu(const QString&) -> FluentMenu*`

Demo: Windows / Overview.

### FluentMenuBar: interaction & painting semantics

- **Click-to-popup**: menu opens on left mouse press (`popup()`); `mouseReleaseEvent` swallows the left release to avoid QMenuBar's native popup logic.
- **Automatic QMenu → FluentMenu conversion**: whenever actions are added/changed, it scans and converts native `QMenu` into `FluentMenu` (preserves the original `QAction` instances, migrates actions, then deletes the old menu).
- **Disable native overflow button**: hides/disables `qt_menubar_ext_button` and clears its menu to avoid the native-styled overflow UI.
- **Highlight animations**:
    - hoverLevel: 120ms linear;
    - highlightRect: 160ms `OutCubic` sliding;
    - custom rounded highlight (radius=6) and a bottom divider (alpha ~70).
- **Stable width**: `sizeHint()` / `minimumSizeHint()` compute a conservative “enough width” from action texts to reduce overflow probability.

### FluentMenu: popup animation, submenus, glyphs

- Uses a `QProxyStyle` override: submenu delay is 120ms and sloppy submenu behavior is disabled.
- On `aboutToShow`, ensures all submenus are converted to `FluentMenu` recursively.
- **Popup animation**: on show, starts at `opacity=0` and Y offset +6px, then runs a 140ms `OutCubic` fade+slide to the final geometry.
- **Painting**:
    - panel radius=10; clears to transparent first, then uses `paintFluentPanel()`.
    - hover highlight radius=6, based on `colors.hover` and hoverLevel.
    - draws extra glyphs: accent checkmark for checked actions, and chevron for actions with submenus.

---

## FluentToolBar

Purpose: Fluent-styled toolbar.

Key APIs:

- Inherits `QToolBar` (use `addAction()` / `addWidget()` etc.)

Key APIs / behavior:

- Inherits `QToolBar`: you still call `addAction()` / `addWidget()`.
- When you `addAction(QAction*)` and that action is not a separator and not a `QWidgetAction`, the toolbar removes the action and inserts a `QWidgetAction` whose default widget is a `FluentToolButton` bound via `setDefaultAction(action)`.
    - If you already add a `QWidgetAction` or `addWidget()`, it will not be replaced.

Demo: Windows / Overview.

---

## FluentStatusBar

Purpose: Fluent-styled status bar.

Key APIs:

- Inherits `QStatusBar` (use `showMessage()` / `addWidget()` etc.)

Key APIs / behavior:

- Inherits `QStatusBar`: use `showMessage()` / `addWidget()`.
- Size grip is disabled by default (`setSizeGripEnabled(false)`), and `WA_StyledBackground` is enabled for QSS.

Demo: Windows / Overview.

---

## FluentDialog

Purpose: dialog with a Fluent title bar, optional modal mask and optional resize.

Key APIs:

- `setMaskEnabled(bool)` / `maskEnabled()`
- `setMaskOpacity(qreal)` / `maskOpacity()`
- `setFluentWindowButtons(WindowButtons)`
- `setFluentResizeEnabled(bool)`, `setFluentResizeBorderWidth(int)`

Implementation notes:

- Frameless + translucent: `WA_TranslucentBackground` + `Qt::FramelessWindowHint`, and draws a rounded frame via `paintFluentFrame()` (radius=10).
- **First-paint guard**: the first `paintEvent` is skipped and triggers a `singleShot(0)` `update()` to avoid platform-specific `QPainter::begin engine==0` warnings.
- **Drag regions**:
    - when title bar is visible: only title bar area drags, and it never steals window button events.
    - when title bar is hidden/collapsed: allows dragging from non-interactive regions (still does not steal from buttons/focusable widgets/selectable-text labels).
- Resize helper is disabled by default; `setFluentResizeEnabled(true)` creates and enables `FluentResizeHelper`.
- **Mask overlay**: when enabled, creates a mask widget on the parent top-level window, follows parent resize/show/state changes, and swallows mouse events to block background interaction.

Demo: Windows / Overview.

---

## MessageBox

```cpp
#include "Fluent/FluentMessageBox.h"

Fluent::FluentMessageBox::information(parent,
                                     QStringLiteral("Title"),
                                     QStringLiteral("Message"),
                                     QStringLiteral("Detail"));
```

Purpose: Fluent-styled message box (supports detail text, optional link, optional modal mask).

Key APIs:

- Static helpers: `information()` / `warning()` / `error()` / `question()`
- `setMaskEnabled(bool)` / `setMaskOpacity(qreal)`

Implementation notes:

- Supports the same mask overlay behavior (dim parent window + block input).
- **Adaptive text height + multi-line elide**:
    - max dialog height is `max(240, availableScreenHeight * 0.72)`;
    - it first tries full text layout; only if it truly exceeds the max height it switches to multi-line elide;
    - when elided, the full text is stored in a tooltip.
- **Drag**: allows dragging from the top region (default ~48px).

Demo: Windows / Overview.

---

## FluentToast

```cpp
#include "Fluent/FluentToast.h"

Fluent::FluentToast::showToast(parent,
                              QStringLiteral("Toast"),
                              QStringLiteral("Hello"),
                              Fluent::FluentToast::Position::BottomRight,
                              2600);
```

Purpose: toast notification anchored to a top-level window (auto dismiss with animations).

Key APIs:

- `FluentToast::showToast(window, title, message, durationMs)`
- `FluentToast::showToast(window, title, message, Position, durationMs)`

Demo: Windows / Pickers / Overview.

Implementation notes:

- Toasts live in an internal `ToastOverlay` attached to the target window (object name `FluentToastOverlay`), and use a **mask region** so the overlay only intercepts input around the toast area:
    - when toasts exist, the overlay blocks input only over toast regions; the rest of the window remains interactive.
    - when no toasts exist, the overlay becomes fully mouse-transparent (`WA_TransparentForMouseEvents=true`).
- Layout: each position (TopLeft/TopCenter/...) has its own queue; new toasts are inserted at the head. Existing toasts are moved smoothly via `QPropertyAnimation(pos)` (150~180ms, `OutCubic`).
- Size stabilization: to avoid word-wrapped `QLabel` “one extra line jump” during fast creation, the overlay fixes the wrap width, calls `adjustSize()`, and stabilizes again on the next event loop turn.
- Appear/disappear:
    - appear opacity: 180ms `OutCubic` from 0→1;
    - auto-dismiss uses a linear progress animation with minimum duration `max(800, durationMs)`;
    - close opacity: 160ms `InCubic` to 0 (or immediate destroy without animation).
- Progress bar expands/shrinks symmetrically from the center of the label area.

---

## FluentResizeHelper

Purpose: edge-drag resize support for frameless windows/dialogs (event filter + hit test).

Key APIs:

- `setEnabled(bool)` / `isEnabled()`
- `setBorderWidth(int)` / `borderWidth()`

Demo: mainly used internally by `FluentMainWindow` / `FluentDialog`.
