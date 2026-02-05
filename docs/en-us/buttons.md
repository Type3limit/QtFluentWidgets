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

Purpose: standard push button with optional **Primary** (accent) style.

Key APIs:

- `setPrimary(bool)` / `isPrimary()`
- `hoverLevel` / `pressLevel` (Q_PROPERTY): animation layer (usually not set manually)

Demo: Buttons / Containers / Pickers / Windows / Overview.

Example:

```cpp
#include "Fluent/FluentButton.h"

auto *btn = new Fluent::FluentButton(QStringLiteral("Primary"));
btn->setPrimary(true);
```

---

## FluentToolButton

Purpose: tool button (commonly used in title bars, toolbars, and internal controls like collapsible card chevrons).

Key APIs:

- `setCheckable(bool)` / `setChecked(bool)`: standard Qt APIs
- `hoverLevel` / `pressLevel` (Q_PROPERTY): animation layer

Demo: Buttons / Overview (also used internally by other widgets).

---

## FluentToggleSwitch

Purpose: Win11-like toggle switch.

Key APIs:

- `setChecked(bool)` / `isChecked()`
- `toggled(bool)` signal
- `setText(const QString&)`
- `progress` / `hoverLevel` / `focusLevel` (Q_PROPERTY): animation/interaction layer

Demo: Buttons / Containers / Windows / Overview.

---

## FluentCheckBox

Purpose: checkbox with hover/focus animation.

Key APIs:

- Standard Qt APIs: `setChecked(bool)` / `isChecked()`
- `hoverLevel` / `focusLevel` (Q_PROPERTY)

Demo: Buttons / Overview.

---

## FluentRadioButton

Purpose: radio button with hover/focus animation.

Key APIs:

- Standard Qt APIs: `setChecked(bool)` / `isChecked()`
- `hoverLevel` / `focusLevel` (Q_PROPERTY)

Demo: Buttons / Overview.
