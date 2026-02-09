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

Purpose: a `QLineEdit` painted with `Style::paintControlSurface()` for a unified rounded surface + 1px border + focus ring, with hover/focus animations.

Inheritance & construction:

- `class FluentLineEdit : public QLineEdit`
- Constructors: `FluentLineEdit(QWidget*)`, `FluentLineEdit(const QString&, QWidget*)`

Visual / interaction notes:

- Disables the native frame (`setFrame(false)`), and uses `Style::metrics()` for paddings and minimum height.
- Selection background is a semi-transparent accent (different alpha for light/dark).
- Caret tries to follow accent (via `QPalette::Text`), while actual text color is controlled by stylesheet; placeholder text stays readable and does not follow accent.

Theme coupling: listens to `ThemeManager::themeChanged` and `EnabledChange` to update stylesheet/palette and repaint.

Key APIs:

- Inherits `QLineEdit`: use `setPlaceholderText()` / `setText()` etc.
- `hoverLevel` / `focusLevel` (Q_PROPERTY)

Demo: Inputs / Containers / Overview.

Example:

```cpp
#include "Fluent/FluentLineEdit.h"

auto *le = new Fluent::FluentLineEdit();
le->setPlaceholderText(QStringLiteral("Type..."));
```

Suggested pattern (search box): this widget does not provide leading/trailing icons; wrap it in `FluentCard` / `FluentWidget` and compose icons/buttons via layout.

---

## FluentTextEdit

Purpose: a `QTextEdit` painted like Fluent inputs via `Style::paintControlSurface()`, plus a transparent border overlay so the border/focus ring stays on top of the viewport content (avoids the “scroll content covers the border” feel).

Inheritance & construction:

- `class FluentTextEdit : public QTextEdit`
- Constructor: `FluentTextEdit(QWidget*)`

Visual / interaction notes:

- Disables native frame (`QFrame::NoFrame`), uses `Style::metrics()` for viewport margins and minimum height.
- Replaces scrollbars with `FluentScrollBar` by default.
- Selection background and caret behavior mirror `FluentLineEdit` (semi-transparent accent selection; caret tries to follow accent).
- Default `sizeHint()` is `200x80` (form-friendly).

Theme coupling: applies theme once on first show (to avoid early viewport paint before shown), then follows `themeChanged`.

Key APIs:

- Inherits `QTextEdit`: use `setPlaceholderText()` / `setPlainText()` etc.
- `hoverLevel` / `focusLevel` (Q_PROPERTY)

Example:

```cpp
#include "Fluent/FluentTextEdit.h"

auto *te = new Fluent::FluentTextEdit();
te->setPlaceholderText(QStringLiteral("Multi-line..."));
te->setAcceptRichText(false);
```

Demo: Inputs / Containers / Windows / Overview.

---

## FluentComboBox

Purpose: a `QComboBox` with custom-painted surface/border/focus ring, plus a patched popup view + delegate so the dropdown is rounded, Fluent-hover/selected, and uses Fluent scrollbars.

Inheritance & construction:

- `class FluentComboBox : public QComboBox`
- Constructor: `FluentComboBox(QWidget*)`

Popup behavior:

- Uses a custom list view (`FluentComboPopupView`).
- The popup container is patched to be frameless and disables drop shadow; rounded clipping uses a mask to avoid light-mode black background artifacts on Windows translucent popups.
- `showPopup()` adds a small gap (~5px) on top of Qt's default position and clamps the popup to the available screen area.

Key APIs:

- Inherits `QComboBox`: `addItem(s)` / `setCurrentIndex()` etc.
- `hoverLevel` (Q_PROPERTY)

Demo: Pickers / Overview.

---

## FluentSpinBox / FluentDoubleSpinBox

Purpose: numeric inputs (`QSpinBox` / `QDoubleSpinBox`) that remove native buttons and custom-paint a right-side stepper area (up/down), with hover/focus animations and press feedback.

Inheritance & construction:

- `class FluentSpinBox : public QSpinBox`
- `class FluentDoubleSpinBox : public QDoubleSpinBox`

Visual / interaction notes:

- Uses `setButtonSymbols(QAbstractSpinBox::NoButtons)`; stepper width follows `Style::metrics().iconAreaWidth`.
- The stepper area is hit-tested as two halves:
	- left press triggers a single `stepUp()` / `stepDown()`.
	- cursor becomes `PointingHandCursor` over the stepper; restores to `IBeamCursor` outside.
- Installs an event filter on the internal `QLineEdit` to map mouse events back to the spinbox, so the stepper works even if the editor covers that region.
- `sizeHint()` estimates a better minimum width from `prefix/suffix/min/max/value/specialValueText` to reduce truncation (you can still override with `setMinimumWidth()`).
- Selection/caret behavior follows `FluentLineEdit`.

Key APIs:

- Inherits Qt APIs: `setRange()` / `setValue()` / `setDecimals()` etc.

Example:

```cpp
#include "Fluent/FluentSpinBox.h"

auto *sb = new Fluent::FluentSpinBox();
sb->setRange(0, 100);
sb->setValue(42);

auto *dsb = new Fluent::FluentDoubleSpinBox();
dsb->setRange(0.0, 1.0);
dsb->setDecimals(3);
dsb->setSingleStep(0.005);
```

Demo: Inputs / Overview.

---

## FluentSlider

Purpose: a `QSlider` with custom-painted track/fill/handle. The handle position is animated for non-dragging value changes, and it supports “click track to jump and immediately drag”.

Inheritance & construction:

- `class FluentSlider : public QSlider`
- Constructor: `FluentSlider(Qt::Orientation, QWidget*)`

Interaction notes:

- `handlePos` (0..1) is used for painting; when `valueChanged` happens and `!isSliderDown()`, it animates to the new position over ~100ms.
- Clicking the handle enters dragging (records drag offset).
- Clicking the track jumps to the clicked position and immediately enters dragging (no second click required).
- On hover, the handle grows and shows an accent inner dot.

Key APIs:

- Inherits `QSlider`: `setRange()` / `setValue()` etc.
- `handlePos` / `hoverLevel` (Q_PROPERTY)

Example:

```cpp
#include "Fluent/FluentSlider.h"

auto *sl = new Fluent::FluentSlider(Qt::Horizontal);
sl->setRange(0, 100);
sl->setValue(30);
connect(sl, &QSlider::valueChanged, this, [](int v) {
	qDebug() << "value:" << v;
});
```

Demo: Inputs / Overview.

---

## FluentProgressBar

Purpose: a thin progress bar (~4px) with border track + accent fill, and a smooth transition on value changes. Supports placing the percentage text at left/center/right or hiding it.

Inheritance & construction:

- `class FluentProgressBar : public QProgressBar`
- Constructor: `FluentProgressBar(QWidget*)`

Painting / behavior notes:

- `displayValue` is the animated display value: on `valueChanged(int)`, a property animation transitions from old→new.
- Text is painted as a percentage derived from `displayValue / (maximum-minimum)`, not `QProgressBar::text()`.
- Track uses `colors.border`, fill uses `colors.accent`, with fixed left/right insets (~12px).

Key APIs:

- `displayValue` (Q_PROPERTY)
- `setTextPosition(TextPosition)` (Left/Center/Right/None)
- `setTextColor(const QColor&)`

Example:

```cpp
#include "Fluent/FluentProgressBar.h"

auto *pb = new Fluent::FluentProgressBar();
pb->setRange(0, 100);
pb->setValue(66);
pb->setTextPosition(Fluent::FluentProgressBar::TextPosition::Right);
```

Demo: Buttons / Containers / Overview.
