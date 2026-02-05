# Windows / Menus / Dialogs

## Widgets

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

---

## FluentMenuBar / FluentMenu

Purpose: Fluent-styled menu bar and menus (hover highlight + popup animations; submenus stay Fluent).

Key APIs:

- `FluentMenuBar::addFluentMenu(const QString&) -> FluentMenu*`
- `FluentMenu::addFluentMenu(const QString&) -> FluentMenu*`

Demo: Windows / Overview.

---

## FluentToolBar

Purpose: Fluent-styled toolbar.

Key APIs:

- Inherits `QToolBar` (use `addAction()` / `addWidget()` etc.)

Demo: Windows / Overview.

---

## FluentStatusBar

Purpose: Fluent-styled status bar.

Key APIs:

- Inherits `QStatusBar` (use `showMessage()` / `addWidget()` etc.)

Demo: Windows / Overview.

---

## FluentDialog

Purpose: dialog with a Fluent title bar, optional modal mask and optional resize.

Key APIs:

- `setMaskEnabled(bool)` / `maskEnabled()`
- `setMaskOpacity(qreal)` / `maskOpacity()`
- `setFluentWindowButtons(WindowButtons)`
- `setFluentResizeEnabled(bool)`, `setFluentResizeBorderWidth(int)`

Demo: Windows / Overview.

---

## FluentMessageBox

Purpose: Fluent-styled message box (supports detail text, optional link, optional modal mask).

Key APIs:

- Static helpers: `information()` / `warning()` / `error()` / `question()`
- `setMaskEnabled(bool)` / `setMaskOpacity(qreal)`

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

---

## FluentResizeHelper

Purpose: edge-drag resize support for frameless windows/dialogs (event filter + hit test).

Key APIs:

- `setEnabled(bool)` / `isEnabled()`
- `setBorderWidth(int)` / `borderWidth()`

Demo: mainly used internally by `FluentMainWindow` / `FluentDialog`.
