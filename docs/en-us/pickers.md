# Pickers

## Widgets

- `FluentCalendarPicker` (include: `Fluent/FluentCalendarPicker.h`)
- `FluentCalendarPopup` (include: `Fluent/datePicker/FluentCalendarPopup.h`)
- `FluentTimePicker` (include: `Fluent/FluentTimePicker.h`)
- `FluentColorPicker` (include: `Fluent/FluentColorPicker.h`)
- `FluentColorDialog` (include: `Fluent/FluentColorDialog.h`)

Demo pages: Pickers (`demo/pages/PagePickers.cpp`) and Overview (`demo/pages/PageOverview.cpp`).

## FluentCalendarPicker

```cpp
#include "Fluent/FluentCalendarPicker.h"

auto *picker = new Fluent::FluentCalendarPicker();
picker->setDate(QDate::currentDate());
```

Purpose: a date edit that shows a Fluent calendar popup.

Inheritance & construction:

- `class FluentCalendarPicker : public QDateEdit`
- Constructor: `FluentCalendarPicker(QWidget*)`

Visual / interaction notes:

- Disables Qt's built-in calendar popup (`setCalendarPopup(false)`) and removes native spin buttons (`QAbstractSpinBox::NoButtons`). The right side becomes a custom-painted chevron area.
- Chevron area width follows `Style::metrics().iconAreaWidth`, and it paints a separator + chevron.
- The internal `QLineEdit` adds a right text margin so text/caret never sits under the chevron area.

Keyboard shortcuts:

- `Alt+Down` or `F4`: toggle popup (similar to `QComboBox`).
- `Esc`: closes the popup when open.

Selection / caret: selection uses a semi-transparent accent; caret tries to follow accent via `QPalette`. Placeholder text keeps readability.

Caveat (popup toggle timing): clicks on the text area land on the internal `QLineEdit`. The implementation uses an event filter + `QTimer::singleShot(0, ...)` to defer toggling, avoiding Qt immediately dismissing a `Qt::Popup` during the same mouse event.

Key APIs:

- Inherits `QDateEdit` (use `setDate()` / `date()` / `setDisplayFormat()` etc.)
- `hoverLevel` / `focusLevel` (Q_PROPERTY)

Demo: Pickers / Overview.

---

## FluentCalendarPopup

Purpose: the popup calendar widget used by `FluentCalendarPicker` (custom-painted `Qt::Popup`). Can also be used standalone.

Inheritance & construction:

- `class FluentCalendarPopup : public QWidget`
- Constructor: `FluentCalendarPopup(QWidget *anchor = nullptr)` (anchor is used for positioning)

Popup behavior ("popup" semantics):

- Window flags: `Qt::Popup` + frameless + no drop shadow. It avoids translucent backgrounds to reduce Windows light-mode black background artifacts.
- Auto close:
	- closes on `WindowDeactivate` / `ApplicationDeactivate`
	- installs a `qApp` event filter to detect clicks outside and dismiss
	- if the click target is the anchor, it consumes that click to avoid a close→reopen flicker
- Positioning: `popup()` tries to place the widget below the anchor (or above if space is insufficient), with a small gap, and clamps to the available screen.
- Rounded clipping: uses a mask for corners.
- Animations: fade-in + slight slide on show; switching view modes (days/months/years) also has a transition.

View modes & interaction:

- Modes: Days / Months / Years
- Header:
	- click the month pill to toggle Days↔Months
	- click the year pill to toggle Days↔Years
	- Today button jumps back to today
- Navigation:
	- Prev/Next step month/year/page depending on current mode
	- mouse wheel also triggers stepping
- Keyboard:
	- `Esc` returns to Days first (if not in Days); `Esc` in Days closes the popup

Key APIs:

- `setAnchor(QWidget*)`
- `setDate(const QDate&)` / `date()`
- `popup()` / `dismiss()`
- `datePicked(const QDate&)` / `dismissed()` signals

Demo: shown indirectly via `FluentCalendarPicker`.

Standalone usage example:

```cpp
#include "Fluent/datePicker/FluentCalendarPopup.h"

auto *popup = new Fluent::FluentCalendarPopup(someButton);
popup->setAnchor(someButton);
popup->setDate(QDate::currentDate());
connect(popup, &Fluent::FluentCalendarPopup::datePicked, this, [](const QDate &d) {
	qDebug() << "picked" << d;
});
popup->popup();
```

---

## FluentTimePicker

Purpose: time edit with custom-painted steppers and hover/focus animations.

Inheritance & construction:

- `class FluentTimePicker : public QTimeEdit`
- Constructor: `FluentTimePicker(QWidget*)`

Visual / interaction notes:

- Removes native buttons (`NoButtons`) and custom-paints a right-side stepper (up/down) area. Width follows `Style::metrics().iconAreaWidth`.
- Stepper hover/press has a stronger accent tint to make the small hit target visible.
- Installs an event filter on the internal `QLineEdit` to forward mouse events, so the stepper remains clickable even if the editor covers that region.
- Selection/caret/placeholder behavior matches `FluentLineEdit` (accent selection + caret accent).

Key APIs:

- Inherits `QTimeEdit` (use `setTime()` / `time()` / `setDisplayFormat()` etc.)
- `hoverLevel` / `focusLevel` (Q_PROPERTY)

Demo: Pickers / Overview.

---

## FluentColorPicker

Purpose: color picker input (preview + button that opens the Fluent color dialog).

Structure & behavior:

- Composed of a read-only preview field (shows `#RRGGBB`) + a "pick color" button.
- The preview field shows a 16x16 color swatch on the left (via `QLineEdit::addAction(LeadingPosition)`).
- Clicking the button opens `FluentColorDialog`:
	- while the dialog is open, it emits `colorChanged` and this control updates live
	- if the user cancels (or the dialog auto-closes), it rolls back to the color from before opening, avoiding "changed but not confirmed" ambiguity

Key APIs:

- `setColor(const QColor&)` / `color()`
- `colorChanged(const QColor&)` signal

Demo: Pickers / Overview.

---

## FluentColorDialog

Purpose: Fluent-styled color dialog (supports reset color, draggable frame, accent border effect).

Window behavior notes:

- Window flags: `Qt::Tool` + frameless + no drop shadow. It intentionally does not use `Qt::Popup` (popup activation/deactivation can be fragile for translucent frameless windows on Windows).
- "Popup-like" auto close: rejects on deactivate unless `m_suppressAutoClose` is enabled (e.g. during an eyedropper operation).
- Draggable header: the header widget uses an event filter to implement click-drag window move.

Border / emphasis effect: internally uses `FluentBorderEffect` + `FluentFramePainter` to paint a rounded surface + 1px border + optional accent trace; `showEvent` plays an initial trace once.

Data model (semantics):

- `currentColor()`: the color currently being edited/previewed.
- `selectedColor()`: the "final" color if accepted; whether it takes effect depends on `Accepted` / `Rejected`.
- `resetColor()`: the color used when pressing Reset.

Recent colors: on Accept it writes to `QSettings` (`QtFluent/FluentColorDialog/recent`, max 12), and shows them next time.

Key APIs:

- Constructor: `FluentColorDialog(const QColor &initial, QWidget *parent = nullptr)`
- `currentColor()` / `setCurrentColor(const QColor&)`
- `selectedColor()`
- `setResetColor(const QColor&)` / `resetColor()`
- `colorChanged(const QColor&)` signal

Demo: Pickers / Overview.

Usage guidance:

- If you want WYSIWYG preview, connect to `colorChanged` and update your UI live.
- If you want "apply only on OK", only read `selectedColor()` after the dialog is accepted.
