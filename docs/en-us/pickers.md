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

Key APIs:

- Inherits `QDateEdit` (use `setDate()` / `date()` / `setDisplayFormat()` etc.)
- `hoverLevel` / `focusLevel` (Q_PROPERTY)

Demo: Pickers / Overview.

---

## FluentCalendarPopup

Purpose: the popup calendar widget used by `FluentCalendarPicker` (custom-painted `Qt::Popup`). Can also be used standalone.

Key APIs:

- `setAnchor(QWidget*)`
- `setDate(const QDate&)` / `date()`
- `popup()` / `dismiss()`
- `datePicked(const QDate&)` / `dismissed()` signals

Demo: shown indirectly via `FluentCalendarPicker`.

---

## FluentTimePicker

Purpose: time edit with custom-painted steppers and hover/focus animations.

Key APIs:

- Inherits `QTimeEdit` (use `setTime()` / `time()` / `setDisplayFormat()` etc.)
- `hoverLevel` / `focusLevel` (Q_PROPERTY)

Demo: Pickers / Overview.

---

## FluentColorPicker

Purpose: color picker input (preview + button that opens the Fluent color dialog).

Key APIs:

- `setColor(const QColor&)` / `color()`
- `colorChanged(const QColor&)` signal

Demo: Pickers / Overview.

---

## FluentColorDialog

Purpose: Fluent-styled color dialog (supports reset color, draggable frame, accent border effect).

Key APIs:

- Constructor: `FluentColorDialog(const QColor &initial, QWidget *parent = nullptr)`
- `currentColor()` / `setCurrentColor(const QColor&)`
- `selectedColor()`
- `setResetColor(const QColor&)` / `resetColor()`
- `colorChanged(const QColor&)` signal

Demo: Pickers / Overview.
