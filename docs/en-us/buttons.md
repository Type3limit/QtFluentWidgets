# Buttons & Toggles

This module contains common clickable/toggle controls with Fluent-style rounded corners and hover/press/focus animations.

## Widget list (public headers)

- `FluentButton` (include: `Fluent/FluentButton.h`)
- `FluentToolButton` (include: `Fluent/FluentToolButton.h`)
- `FluentToggleSwitch` (include: `Fluent/FluentToggleSwitch.h`)
- `FluentCheckBox` (include: `Fluent/FluentCheckBox.h`)
- `FluentRadioButton` (include: `Fluent/FluentRadioButton.h`)

Demo pages: Buttons (`demo/pages/PageButtons.cpp`) and Overview (`demo/pages/PageOverview.cpp`).

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
