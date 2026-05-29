# Buttons & Toggles

This module contains common clickable/toggle controls with Fluent-style rounded corners and hover/press/focus animations.

## Widget list (public headers)

- `FluentButton` (include: `Fluent/FluentButton.h`)
- `FluentAnimatedButton` (include: `Fluent/FluentAnimatedButton.h`)
- `FluentAnimatedIcon` (include: `Fluent/FluentAnimatedIcon.h`)
- `FluentLottieWidget` (include: `Fluent/FluentLottieWidget.h`)
- `FluentToolButton` (include: `Fluent/FluentToolButton.h`)
- `FluentDropDownButton` (include: `Fluent/FluentDropDownButton.h`)
- `FluentSplitButton` (include: `Fluent/FluentSplitButton.h`)
- `FluentCommandBar` (include: `Fluent/FluentCommandBar.h`)
- `FluentToggleSwitch` (include: `Fluent/FluentToggleSwitch.h`)
- `FluentCheckBox` (include: `Fluent/FluentCheckBox.h`)
- `FluentRadioButton` (include: `Fluent/FluentRadioButton.h`)
- `FluentProgressRing` (include: `Fluent/FluentProgressRing.h`)

Demo pages: Buttons (`demo/pages/PageButtons.cpp`) and Overview (`demo/pages/PageOverview.cpp`).

---

## FluentLottieWidget / FluentAnimatedIcon / FluentAnimatedButton

Purpose: `FluentLottieWidget` is a general Lottie JSON playback widget. `FluentAnimatedIcon` layers WinUI-style state semantics on top. `FluentAnimatedButton` wires an animated icon into `QPushButton` semantics for search affordances, navigation icons, button icons, and other hover / pressed / selected animations.

For complete usage, theme tinting, asset authoring, and performance guidance, see [lottie.md](lottie.md). This section keeps a quick reference for the Buttons page.

Dependency: rlottie is integrated from the `third_party/rlottie` Git submodule. After a fresh clone, run:

```bash
git submodule update --init --recursive
```

Inheritance & construction:

- `class FluentLottieWidget : public QWidget`
- `class FluentAnimatedIcon : public FluentLottieWidget`
- `class FluentAnimatedButton : public QPushButton`
- Constructors: `FluentLottieWidget(QWidget*)`, `FluentAnimatedIcon(QWidget*)`, `FluentAnimatedButton(QWidget*)`, `FluentAnimatedButton(const QString&, QWidget*)`

Key APIs:

- `load(const QString&)`: load a `.json` Lottie file.
- `loadData(const QByteArray&, const QString& cacheKey = {}, const QString& resourcePath = {})`: load from memory.
- `play()` / `pause()` / `stop()`: playback control.
- `setCurrentFrame(int)` / `setProgress(qreal)`: frame positioning.
- `playSegment(int, int)` / `playMarker(const QString&)`: play a frame range or marker range.
- `markerNames()` / `hasMarker()` / `markerFrame()` / `markerEndFrame()`: query top-level Lottie markers.
- `FluentLottieWidget::setTintColor(const QColor&)` / `resetTintColor()`: optionally recolor rendered frames by alpha; useful for icon-like Lottie files.
- `FluentAnimatedIcon::setState(const QString&, bool animated = true)`: switch marker states.
- `FluentAnimatedIcon::setInteractive(true)`: automatically map hover / press to `Normal`, `PointerOver`, and `Pressed`.
- `FluentAnimatedButton::loadAnimation()` / `loadAnimationData()`: load the button icon animation.
- `FluentAnimatedButton::animatedIcon()`: access the internal icon widget for fallback icons, marker queries, and related setup.
- `FluentAnimatedButton::setAnimatedIconSize()`: set the icon size inside the button.
- `FluentAnimatedButton` automatically tints its internal animated icon to the current foreground color, so primary buttons use white icons and disabled buttons use the disabled text color.

`FluentAnimatedIcon` marker resolution:

- `[PreviousState]To[NewState]_Start` + `_End`: play the segment.
- `[PreviousState]To[NewState]`: play or jump to the transition marker.
- `[NewState]`: jump to the state marker.
- Any `To[NewState]_End`: fallback state frame.
- Numeric state: target frame.
- Missing marker: frame 0.

Example:

```cpp
#include "Fluent/FluentAnimatedIcon.h"

auto *icon = new Fluent::FluentAnimatedIcon();
icon->setFixedSize(48, 48);
icon->load(QStringLiteral(":/lottie/search.json"));
icon->setInteractive(true);
icon->setState(QStringLiteral("Normal"), false);
```

Button example:

```cpp
#include "Fluent/FluentAnimatedButton.h"

auto *button = new Fluent::FluentAnimatedButton(QStringLiteral("Search"));
button->loadAnimation(QStringLiteral(":/lottie/search.json"));
connect(button, &QPushButton::clicked, this, [] {
    qDebug() << "Search clicked";
});
```

Notes:

- `FluentAnimatedButton` is best for action affordances; avoid replacing every ordinary form button with animation.
- The first version does not automatically wire animated icons into every navigation control yet; use it as a standalone QWidget or button.
- If Lottie loading fails, the widget paints a fallback icon or a default static glyph.
- Public headers do not expose rlottie types, so backend changes should not break user code.

See [animated-icon-plan.md](animated-icon-plan.md) for the broader integration plan.

---

## FluentButton

Purpose: a `QPushButton` with Fluent rounded corners/border, hover/press animations, and a focus ring. Supports a Primary (accent-filled) style and a Win11-like checked appearance for checkable buttons.

Inheritance & construction:

- `class FluentButton : public QPushButton`
- Constructors: `FluentButton(QWidget*)`, `FluentButton(const QString&, QWidget*)`

Visual / interaction notes:

- Minimum height follows `Style::metrics().height` (closer to Win11 control sizing).
- Secondary (default): surface fill + border.
	- If `setCheckable(true)` and checked, it adds a subtle accent tint + accent border, and shifts text color slightly toward accent.
- Primary: accent fill.
	- If checkable and checked, it draws an extra inner highlight to strengthen the “selected” state.
- With an icon: the icon is painted on the left; text is laid out as a group (icon + gap + text) and visually centered.

Theme coupling: listens to `ThemeManager::themeChanged` and its own enabled-state changes to repaint.

Key APIs:

- `setPrimary(bool)` / `isPrimary()`
- `setCheckable(bool)` / `setChecked(bool)` / `toggled(bool)` (inherited from `QAbstractButton`; checked state has extra painting)
- `setIcon(const QIcon&)` / `setIconSize(const QSize&)`
- `hoverLevel` / `pressLevel` (Q_PROPERTY): animation layers (normally driven internally)

Demo: Buttons / Containers / Pickers / Windows / Overview.

Examples:

```cpp
#include "Fluent/FluentButton.h"

auto *btn = new Fluent::FluentButton(QStringLiteral("Primary"));
btn->setPrimary(true);
```

Icon + checkable:

```cpp
auto *toggle = new Fluent::FluentButton(QStringLiteral("Pin"));
toggle->setCheckable(true);
toggle->setIcon(QIcon(QStringLiteral(":/icons/pin.svg")));
toggle->setIconSize(QSize(16, 16));
connect(toggle, &QAbstractButton::toggled, this, [](bool on) {
		qDebug() << "Pinned:" << on;
});
```

Caveat:

- This is a custom-painted button (overrides `paintEvent`). Heavy QSS overrides for background/border may not apply; prefer adjusting palette via `ThemeManager` / `ThemeColors`.

---

## FluentToolButton

Purpose: a `QToolButton` tuned for compact interactions (title bars, toolbars, collapse chevrons, etc.). Uses `autoRaise` by default, and provides hover/press animations and a focus ring.

Inheritance & construction:

- `class FluentToolButton : public QToolButton`
- Constructors: `FluentToolButton(QWidget*)`, `FluentToolButton(const QString&, QWidget*)`

Visual / interaction notes:

- Defaults to `setAutoRaise(true)` and uses `Style::metrics().height` as minimum height.
- If `setCheckable(true)` and checked, it shows a subtle accent tint + accent border, and shifts label color toward accent.

Title-bar window button convention (internal):

If the dynamic property `fluentWindowGlyph` (int) is set, the button paints as a caption button glyph:

- `0`: minimize
- `1`: maximize
- `2`: restore
- `3`: close (hover uses a stronger red)

This is mainly used by `FluentMainWindow`; typical app code does not need to set it.

Key APIs:

- Standard Qt APIs: `setCheckable(bool)` / `setChecked(bool)`
- `hoverLevel` / `pressLevel` (Q_PROPERTY)

Demo: Buttons / Overview (also used internally by other widgets).

---

## FluentDropDownButton / FluentSplitButton / FluentCommandBar

Purpose: command-entry controls for tool surfaces, page command rows, card headers, and other places where related operations need to be grouped. They have different roles:

- `FluentDropDownButton`: one button that opens a set of secondary actions.
- `FluentSplitButton`: a high-frequency default action on the left, with variants or more options on the right.
- `FluentCommandBar`: a `QAction`-centered surface for frequent commands, menu commands, separators, and a small number of custom widgets. When an action's enabled/text/icon state changes, the matching command button updates with it.

Inheritance & construction:

- `class FluentDropDownButton : public FluentButton`
- `class FluentSplitButton : public QWidget`
- `class FluentCommandBar : public QWidget`

Key APIs:

- `FluentDropDownButton::setMenu(QMenu*)` / `addAction()`: open a menu from the button.
- `FluentSplitButton::setDefaultAction(QAction*)`: run the primary action from the left side.
- `FluentSplitButton::setMenu(QMenu*)`: open secondary actions from the right arrow.
- `FluentCommandBar::addAction()` / `addCommand(QAction*)` / `addSeparator()` / `addWidget()`: compose command buttons, menu commands, separators, and custom widgets; when space runs out, trailing commands automatically move into a right-side overflow FluentMenu.

Example:

```cpp
#include "Fluent/FluentDropDownButton.h"
#include "Fluent/FluentSplitButton.h"
#include "Fluent/FluentCommandBar.h"
#include "Fluent/FluentMenu.h"

#include <QAction>

auto *drop = new Fluent::FluentDropDownButton(QStringLiteral("More"));
drop->addAction(QStringLiteral("Copy"));
drop->addAction(QStringLiteral("Move"));

auto *split = new Fluent::FluentSplitButton(QStringLiteral("Save"));
split->setDefaultAction(new QAction(QStringLiteral("Save"), split));
auto *menu = new Fluent::FluentMenu(split);
menu->addAction(QStringLiteral("Save as"));
split->setMenu(menu);

auto *bar = new Fluent::FluentCommandBar();
auto *newAction = new QAction(QStringLiteral("New"), bar);
auto *editAction = new QAction(QStringLiteral("Edit"), bar);
bar->addCommand(newAction);
bar->addCommand(editAction);
bar->addSeparator();

auto *exportMenu = new Fluent::FluentMenu(bar);
exportMenu->setTitle(QStringLiteral("Export"));
exportMenu->addAction(QStringLiteral("PDF"));
bar->addCommand(exportMenu->menuAction());

editAction->setEnabled(false); // The corresponding CommandBar button is disabled too.
```

Notes:

- `FluentCommandBar` is a lightweight command container, not a replacement for `QToolBar`; use `FluentToolBar` when you need main-window toolbar semantics.
- `FluentSplitButton` separates default action and secondary menu. If the control only opens a menu, `FluentDropDownButton` is clearer.
- `FluentCommandBar` is valuable when commands are managed centrally; it is not just a styled replacement for `QHBoxLayout + Button`. Overflow menu items reuse the same `QAction` instances, so enabled, text, icon, and submenu state stay synchronized.

Demo: Buttons / Overview.

---

## FluentToggleSwitch

Purpose: Win11-like toggle switch implemented as a custom-painted `QWidget`, with track/knob animation, hover row highlight, and a focus ring.

Inheritance & construction:

- `class FluentToggleSwitch : public QWidget`
- Constructors: `FluentToggleSwitch(QWidget*)`, `FluentToggleSwitch(const QString&, QWidget*)`

Visual / interaction notes:

- Track size is roughly `40x20`, knob is an Islands-style dot (no shadow; performance-oriented).
- `setText()` shows a right-side label (auto-elided).
- `setChecked()` starts a smooth progress animation and emits `toggled(bool)`.

Key APIs:

- `setChecked(bool)` / `isChecked()`
- `toggled(bool)`
- `setText(const QString&)`
- `progress` / `hoverLevel` / `focusLevel` (Q_PROPERTY)

Example:

```cpp
#include "Fluent/FluentToggleSwitch.h"

auto *sw = new Fluent::FluentToggleSwitch(QStringLiteral("Auto save"));
sw->setChecked(true);
connect(sw, &Fluent::FluentToggleSwitch::toggled, this, [](bool on) {
	qDebug() << "AutoSave:" << on;
});
```

Caveat:

- This is not a `QAbstractButton`. If you need button-group semantics (autoExclusive, etc.), prefer `FluentRadioButton` or wrap your own.

Demo: Buttons / Containers / Windows / Overview.

---

## FluentCheckBox

Purpose: a `QCheckBox` with hover row highlight, focus ring, and a checkmark fade-in animation.

Inheritance & construction:

- `class FluentCheckBox : public QCheckBox`
- Constructors: `FluentCheckBox(QWidget*)`, `FluentCheckBox(const QString&, QWidget*)`

Key APIs:

- Standard Qt APIs: `setChecked(bool)` / `isChecked()`
- `stateChanged(int)` (drives the internal check animation)
- `hoverLevel` / `focusLevel` (Q_PROPERTY)

Sizing: overrides `sizeHint()` / `minimumSizeHint()` to be closer to Win11 spacing based on text width.

Demo: Buttons / Overview.

---

## FluentRadioButton

Purpose: a `QRadioButton` with hover row highlight, focus ring, and a selection animation (dot scale + accent border fade-in).

Inheritance & construction:

- `class FluentRadioButton : public QRadioButton`
- Constructors: `FluentRadioButton(QWidget*)`, `FluentRadioButton(const QString&, QWidget*)`

Key APIs:

- Standard Qt APIs: `setChecked(bool)` / `isChecked()`
- `toggled(bool)` (drives the internal selection animation)
- `hoverLevel` / `focusLevel` (Q_PROPERTY)

Sizing: overrides `sizeHint()` / `minimumSizeHint()` to be closer to Win11 spacing based on text width.

Demo: Buttons / Overview.
