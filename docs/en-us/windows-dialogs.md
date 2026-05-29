# Windows / Menus / Dialogs

## Widget list

- `FluentMainWindow` (include: `Fluent/FluentMainWindow.h`)
- `FluentMenuBar` (include: `Fluent/FluentMenuBar.h`)
- `FluentMenu` (include: `Fluent/FluentMenu.h`)
- `FluentToolBar` (include: `Fluent/FluentToolBar.h`)
- `FluentStatusBar` (include: `Fluent/FluentStatusBar.h`)
- `FluentDialog` (include: `Fluent/FluentDialog.h`)
- `FluentMessageBox` (include: `Fluent/FluentMessageBox.h`)
- `FluentInfoBar` (include: `Fluent/FluentInfoBar.h`)
- `FluentFlyout` (include: `Fluent/FluentFlyout.h`)
- `FluentTeachingTip` (include: `Fluent/FluentTeachingTip.h`)
- `FluentToast` (include: `Fluent/FluentToast.h`)
- `FluentResizeHelper` (include: `Fluent/FluentResizeHelper.h`)

Demo pages: Windows (`demo/pages/PageWindows.cpp`) and Overview (`demo/pages/PageOverview.cpp`).

## FluentInfoBar

```cpp
#include "Fluent/FluentInfoBar.h"

auto *bar = new Fluent::FluentInfoBar(
    Fluent::FluentInfoBar::Severity::Warning,
    QStringLiteral("Heads up"),
    QStringLiteral("This is an inline message."));
bar->setActionText(QStringLiteral("View"));
```

Purpose: inline feedback for validation results, sync status, lightweight errors, and other messages that should not interrupt the user.

Key APIs:

- `setSeverity(Info/Success/Warning/Error)`: semantic color.
- `setTitle()` / `setMessage()`: title and body.
- `setActionText()`: optional action button.
- `setClosable(bool)`: show or hide the close button.
- `actionTriggered()` / `closed()`: action and dismissal signals.

Demo: Windows / Overview.

## Popup / FluentFlyout / FluentTeachingTip

```cpp
#include "Fluent/FluentFlyout.h"
#include "Fluent/FluentTeachingTip.h"

auto *flyout = new Fluent::FluentFlyout(parent);
flyout->setContentWidget(new QLabel(QStringLiteral("Flyout content")));
flyout->showFor(anchorButton);

auto *tip = new Fluent::FluentTeachingTip(parent);
tip->setTitle(QStringLiteral("TeachingTip"));
tip->setSubtitle(QStringLiteral("Explain a new feature."));
tip->setContentWidget(new QLabel(QStringLiteral("Place a lightweight QWidget in the body area.")));
tip->setActionText(QStringLiteral("Got it"));
tip->setTarget(anchorButton);
tip->setMaskEnabled(true);
tip->setMaskOpacity(0.46);
tip->open();

// Multi-step guide: describe the target list and each step's visual style.
auto *tour = new Fluent::FluentTeachingTip(parent);
tour->setMaskEnabled(true);
tour->setGuideTargets({ searchButton, previewSwitch, exportButton });
tour->setGuideStyles({
    { QStringLiteral("1 / 3  Search entry"), QStringLiteral("Tell users where to start."), QStringLiteral("Next") },
    { QStringLiteral("2 / 3  Preview toggle"), QStringLiteral("Explain the current state control."), QStringLiteral("Next"), QStringLiteral("Back") },
    { QStringLiteral("3 / 3  Export action"), QStringLiteral("Point to the final action."), QStringLiteral("Done"), QStringLiteral("Back") },
});
tour->setGuideContentWidgets({ nullptr, new QLabel(QStringLiteral("Custom body for step 2.")), nullptr });
connect(tour, &Fluent::FluentTeachingTip::guideFinished, tour, &QObject::deleteLater);
tour->startGuide();
```

Purpose: all three can appear near a target control, but they serve different jobs:

- **Popup/Menu**: a short-lived selection or command surface, such as `FluentMenu`, a ComboBox popup, or a calendar popup. It is usually managed by a concrete control and is best for lists, menus, dates, and one-shot choices.
- **FluentFlyout**: a contextual mini panel created by application code. It hosts arbitrary QWidget content and fits lightweight interactions such as local view settings or small detail panels.
- **FluentTeachingTip**: a guidance-oriented Flyout variant with title, subtitle, close button, and primary action. Use it to explain a target control or introduce a feature; avoid complex forms inside it.

Key APIs:

- `FluentFlyout::setContentWidget(QWidget*)`: set popup content.
- `FluentFlyout::showAt(QPoint)` / `showFor(QWidget*, Placement)`: open by global point or target widget.
- `FluentTeachingTip::setTitle()` / `setSubtitle()` / `setActionText()`: tip content.
- `FluentTeachingTip::setContentWidget(QWidget*)` / `contentWidget()`: set the custom QWidget body area inside the TeachingTip.
- `FluentTeachingTip::setTarget(QWidget*)` / `open()`: bind a target and open.
- `FluentTeachingTip::setGuideTargets(QList<QWidget*>)` / `setGuideStyles(QList<GuideStyle>)`: declaratively configure a multi-step guide.
- `FluentTeachingTip::setGuideContentWidgets(QList<QWidget*>)`: set an optional body QWidget for each guide step.
- `FluentTeachingTip::startGuide()` / `nextGuideStep()` / `previousGuideStep()` / `finishGuide()`: start, advance, go back, or finish a multi-step guide.
- `FluentTeachingTip::guideStarted()` / `guideStepChanged(int)` / `guideFinished()`: guide lifecycle signals.
- `FluentTeachingTip::setMaskEnabled(bool)` / `maskEnabled()`: optional mask for guided flows.
- `FluentTeachingTip::setMaskOpacity(qreal)` / `maskOpacity()`: mask opacity.
- `FluentTeachingTip::actionTriggered()`: primary action signal.

Notes:

- Flyout uses `Qt::Popup`; clicking outside follows platform popup-dismiss behavior.
- Use `FluentMenu` / `FluentDropDownButton` for command lists, `FluentFlyout` for custom contextual content, and `FluentTeachingTip` when the surface is explaining a target control.
- A single tip can still use `setTarget()` + `open()`. For sequential guidance, prefer `setGuideTargets()` + `setGuideStyles()` + `startGuide()` so application code does not need to maintain a custom step state machine.
- The first guide step hides the back action; later steps show `Back` by default, or use `GuideStyle::previousActionText` for custom text.
- TeachingTip's mask overlay is attached to the target's top-level window. It dims the background, blocks background mouse input, and leaves a highlighted cutout around the current `target`.
- TeachingTip uses a rectangular mask cutout and does not draw an extra highlight ring, avoiding both jagged `QRegion` rounded clipping and visual ambiguity from a separate stroke layer. The target remains visible, while the background area is still blocked by the mask.
- TeachingTip mask fade-in/fade-out uses `FluentMotionRole::PopupOpen` / `PopupClose`; with global animations disabled the mask appears or is removed immediately, and the TeachingTip popup itself always stays above the overlay.

Demo: Windows / Overview.

## FluentMainWindow

```cpp
#include "Fluent/FluentMainWindow.h"

class MainWindow : public Fluent::FluentMainWindow {
public:
    using Fluent::FluentMainWindow::FluentMainWindow;
};
```

Purpose: main window with a Fluent title bar (menu bar embedded into the title bar), title-bar slots, custom accent border painting, and optional frameless resize.

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

- **Does not use QMainWindow's ToolBar/StatusBar areas**: calling `addToolBar()` / `insertToolBar()` / `setStatusBar()` hides and `deleteLater()` the passed objects. `FluentMainWindow` expects your actual app layout to live inside the central area.
- **Central widget is wrapped**: on the first `setCentralWidget()`, the window creates an internal `WindowFrameHost` and reparents your widget into it.
    - `WindowFrameHost` becomes the actual `QMainWindow::centralWidget()` and paints the central `surface` background.
    - Your app widget still fills that host with 0 margins / 0 spacing, so central layout code stays straightforward.
- **The window itself reserves a 1px gutter in restored state**: `contentsMargins(1,1,1,1)` is used to leave room for the custom border; maximized / fullscreen state collapses that gutter back to 0.
- **The accent border is not painted inside the central widget**: an internal `WindowBorderOverlay` is a direct child of `FluentMainWindow`, covers the full content area (`title bar + central widget`), and is always raised above other children so opaque widgets cannot cover the border.
- **The title bar is installed as `menuWidget()`**: the title bar host is placed at the top via `setMenuWidget()`.
- **Rounded clipping no longer uses `QRegion::setMask()`**: the current implementation relies on antialiased custom painting plus Windows 11 DWM corner rounding (`DWMWCP_ROUND`) to avoid the visible jaggies of the old mask-based approach.

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

One subtle detail: calling only `setWindowTitle()` / `setWindowIcon()` does **not** force the custom title bar to stay visible. If there is no menu bar, no visible window buttons, no slot content, and no explicit Fluent title/icon override, the title bar may still collapse intentionally for a cleaner content-first layout.

### Title eliding and adaptive height

- The title text is elided with `Qt::ElideRight` based on **reserved widths** of left/right areas (max of `minimumWidth` and `sizeHint`) to avoid transient squeezes that would push `QMenuBar` into Qt's native overflow (`qt_menubar_ext_button`).
- Title bar height is content-adaptive:
    - if window buttons are present, height is at least `Style::windowMetrics().titleBarHeight`;
    - it also takes the max with the layout `sizeHint().height()` so custom widgets (e.g. a `FluentLineEdit` in the title bar) are not clipped.
- The title bar background uses a slightly smaller **inner radius** (`kWindowFrameRadius - 1`) than the outer window border so the background aligns with the 1px border gutter instead of visually pushing into it.

### Drag / double click behavior

- Drag to move: on left press in the title bar area, if the hit target is an interactive widget (buttons/focusable widgets/menu bar area), the event goes to that widget; otherwise the window triggers `startSystemMove()`.
- Double click toggles maximize/restore and updates button glyphs.
- When the title bar is collapsed/hidden, the window installs event filters on newly added child widgets so you can still drag from **non-interactive regions** (it will not steal events from buttons, focusable widgets, or selectable-text labels).
- When child widgets are added later (for example after replacing the central widget), both the title bar and the border overlay are raised again so the visual stacking order remains correct.

### Menu bar integration and migration

- `menuBar()` here is a convenience shim (Qt's `QMainWindow::menuBar()` is not virtual). The first call creates and installs a `FluentMenuBar`.
- `setMenuBar(QMenuBar*)`:
    - passing a `FluentMenuBar*`: installs it directly.
    - passing a plain `QMenuBar*` (typically from a `.ui` file whose root is promoted but whose menubar is not): **adopts** it — keeps the original object alive and hidden, and installs an event filter that moves uic's later `ActionAdded` events into the `FluentMenuBar`. Consequences:
        1. The `ui->menubar` pointer stays valid; later `ui->menubar->addMenu(...)` calls keep working.
        2. The standard uic order — `setMenuBar(menubar)` on an *empty* menubar, then `menubar->addAction(menuFile->menuAction())` *afterwards* — works correctly. The previous implementation copied the (still empty) action list once and then lost every menu added after that point.
        3. The adopted source menubar stays hidden and does not retain actions, so the same actions are not owned by both a native `QMenuBar` and the visible `FluentMenuBar`.
    - If the `.ui` root is still declared as `QMainWindow`, uic generates `setupUi(QMainWindow*)` and statically bypasses several hidden `FluentMainWindow` methods. This is not the recommended workflow; promote the root widget at minimum.
- To reduce native overflow, the title bar menu bar uses `QSizePolicy::Minimum` and sets `minimumWidth(minimumSizeHint().width())`.

**Recommended Qt Designer workflow:**

1. Promote the `.ui` root widget to `Fluent::FluentMainWindow` (header `Fluent/FluentMainWindow.h`) so that `setupUi()` takes a `Fluent::FluentMainWindow*` and `MainWindow->setMenuBar(...)` statically binds to our overload.
2. Prefer promoting the menubar to `Fluent::FluentMenuBar` (header `Fluent/FluentMenuBar.h`) and menus to `Fluent::FluentMenu` (header `Fluent/FluentMenu.h`). Then `ui->menubar` is the final visible Fluent menubar and can use `FluentMenuBar`-specific APIs directly.
3. If only the root is promoted, the library uses the adoption/action-move compatibility path. It is fine for uic-generated menu structure, but later action removal/reordering should be done on `window->fluentMenuBar()`.
4. **Don't** call `setFluentMenuBar(ui->menubar)` again from `initUI()` — promoted `setupUi()` has already installed it into the title bar. Calling it again is redundant, and the line would not compile if `ui->menubar`'s static type is `QMenuBar*`.

### Windows: frameless resize (WM_NCHITTEST)

- With the Fluent title bar (frameless) enabled, Windows handles `WM_NCHITTEST`:
    - uses `Style::windowMetrics().resizeBorder` as the hit-test thickness and returns `HTLEFT/HTTOP/HTBOTTOMRIGHT/...` for edge resize.
    - in the title bar area: if the cursor is over interactive widgets (buttons/focusable widgets/menu bar), returns `HTCLIENT` so clicks reach the widgets; otherwise returns `HTCAPTION` so you can drag.
- The implementation also requests DWM window attributes:
    - restored state uses `DWMWCP_ROUND` rounded corners;
    - maximized state switches to `DWMWCP_DONOTROUND`;
    - immersive dark/light non-client appearance follows the current theme.

### Theme coupling and border

- On theme changes, the window updates the global `QApplication` stylesheet with `Theme::baseStyleSheet(...)`, with an equality check to avoid redundant repolish.
- Frame/title-bar border color follows `ThemeManager::accentBorderEnabled()`; both the border and the trace animation are driven by `FluentBorderEffect + WindowBorderOverlay`.
- On first show, if the accent border is enabled, the window plays a one-shot trace-in animation after the first paint; later theme/accent changes reuse the same border state machine.
- The current border radius is aligned with the default Windows 11 DWM rounded-corner clip (8 DIPs) so the custom accent border arc matches the actual window silhouette.

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
- **Single popup path**: the custom menubar no longer calls `QMenuBar::setActiveAction()`, so clicking a top-level menu only shows the Fluent popup and does not also activate Qt's native `QMenu` popup.
- **Neutral menu border**: menu popups use the normal popup border instead of the window accent border, so a one-item menu does not read as a second button/menu strip.
- **Disable native overflow button**: hides/disables `qt_menubar_ext_button` and clears its menu to avoid the native-styled overflow UI.
- **Highlight animations**:
    - hoverLevel uses `FluentMotionRole::Hover`;
    - highlightRect uses `FluentMotionRole::Selection`;
    - custom rounded highlight (radius=6) and a bottom divider (alpha ~70).
- **Stable width**: `sizeHint()` / `minimumSizeHint()` compute a conservative “enough width” from action texts to reduce overflow probability.

### FluentMenu: popup animation, submenus, glyphs

- Uses a `QProxyStyle` override: submenu delay is 120ms and sloppy submenu behavior is disabled.
- On `aboutToShow`, ensures all submenus are converted to `FluentMenu` recursively.
- **Popup animation**: on show, starts at `opacity=0` with a small `FluentMotion::popupSlideOffset()` displacement, then uses `FluentMotionRole::PopupOpen` for fade+slide to the final geometry.
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
- Layout: each position (TopLeft/TopCenter/...) has its own queue; new toasts are inserted at the head. Existing toasts are moved smoothly via `QPropertyAnimation(pos)` using `FluentMotionRole::Toast`.
- Size stabilization: to avoid word-wrapped `QLabel` “one extra line jump” during fast creation, the overlay fixes the wrap width, calls `adjustSize()`, and stabilizes again on the next event loop turn.
- Appear/disappear:
    - appear and close opacity use `FluentMotionRole::Toast`;
    - auto-dismiss uses a linear progress animation with minimum duration `max(800, durationMs)`;
    - disabling global animations or setting the Toast duration to 0 makes appear immediate and close destroy the toast directly.
- Progress bar expands/shrinks symmetrically from the center of the label area.

---

## FluentResizeHelper

Purpose: edge-drag resize support for frameless windows/dialogs (event filter + hit test).

Key APIs:

- `setEnabled(bool)` / `isEnabled()`
- `setBorderWidth(int)` / `borderWidth()`

Demo: mainly used internally by `FluentMainWindow` / `FluentDialog`.
