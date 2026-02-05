# Inputs

This module covers common input/edit widgets (excluding date/time/color pickers and the code editor).

Demo pages: Inputs (`demo/pages/PageInputs.cpp`) and Overview (`demo/pages/PageOverview.cpp`).

## Widget list (public headers)

- `FluentLineEdit` (include: `Fluent/FluentLineEdit.h`)
- `FluentTextEdit` (include: `Fluent/FluentTextEdit.h`)
- `FluentComboBox` (include: `Fluent/FluentComboBox.h`)
- `FluentSpinBox` / `FluentDoubleSpinBox` (include: `Fluent/FluentSpinBox.h`)
- `FluentSlider` (include: `Fluent/FluentSlider.h`)
- `FluentProgressBar` (include: `Fluent/FluentProgressBar.h`)

Related but documented under Containers/Layout:

- `FluentScrollBar` / `FluentScrollArea`

> `FluentCodeEditor` is documented separately.

---

## FluentLineEdit

Purpose: single-line input with unified Fluent surface/border and focus ring.

Key APIs:

- Inherits `QLineEdit` (use `setPlaceholderText()` / `setText()` etc.)
- `hoverLevel` / `focusLevel` (Q_PROPERTY): animation layer

Demo: Inputs / Containers / Overview.

Example:

```cpp
#include "Fluent/FluentLineEdit.h"

auto *le = new Fluent::FluentLineEdit();
le->setPlaceholderText(QStringLiteral("Type..."));
```

---

## FluentTextEdit

Purpose: multi-line editor (uses a border overlay to keep border/focus ring visible).

Key APIs:

- Inherits `QTextEdit` (use `setPlaceholderText()` / `setPlainText()` etc.)
- `hoverLevel` / `focusLevel` (Q_PROPERTY)

Demo: Inputs / Containers / Windows / Overview.

---

## FluentComboBox

Purpose: drop-down selector with Fluent painting and hover animation.

Key APIs:

- Inherits `QComboBox` (use `addItem(s)` / `setCurrentIndex()` etc.)
- `hoverLevel` (Q_PROPERTY)

Demo: Pickers / Overview.

---

## FluentSpinBox / FluentDoubleSpinBox

Purpose: numeric inputs with Fluent stepper interaction and hover/focus animations.

Key APIs:

- Inherits `QSpinBox` / `QDoubleSpinBox` (use `setRange()` / `setValue()` / `setDecimals()` etc.)

Demo: Inputs / Overview.

---

## FluentSlider

Purpose: slider with animated handle position.

Key APIs:

- Inherits `QSlider` (use `setRange()` / `setValue()` etc.)
- `handlePos` / `hoverLevel` (Q_PROPERTY)

Demo: Inputs / Overview.

---

## FluentProgressBar

Purpose: progress bar with animated display value and configurable text placement.

Key APIs:

- `displayValue` (Q_PROPERTY): animation display layer
- `setTextPosition(TextPosition)` (Left/Center/Right/None)
- `setTextColor(const QColor&)`

Demo: Buttons / Containers / Overview.
