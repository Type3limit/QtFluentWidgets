# Pickers

## Widgets

- `FluentCalendarPicker` (include: `Fluent/FluentCalendarPicker.h`)
- `FluentDatePicker` (include: `Fluent/FluentDatePicker.h`)
- `FluentDateRangePicker` (include: `Fluent/FluentDateRangePicker.h`)
- `FluentCalendarPopup` (include: `Fluent/datePicker/FluentCalendarPopup.h`)
- `FluentTimePicker` (include: `Fluent/FluentTimePicker.h`)
- `FluentColorPicker` (include: `Fluent/FluentColorPicker.h`)
- `FluentColorDialog` (include: `Fluent/FluentColorDialog.h`)

Demo pages: Pickers (`demo/pages/PagePickers.cpp`) and Overview (`demo/pages/PageOverview.cpp`).

## FluentCalendarPicker

```cpp
#include "Fluent/FluentCalendarPicker.h"

auto *picker = new Fluent::FluentCalendarPicker();
picker->setTodayText(QStringLiteral("Go to today"));
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
- Weekday headers, month names, and the first day of week follow the widget's own `locale()`. The default locale is Chinese (China).
- The Today button label is configurable through `setTodayText()`, so the popup text does not have to stay fixed.

Keyboard shortcuts:

- `Alt+Down` or `F4`: toggle popup (similar to `QComboBox`).
- `Esc`: closes the popup when open.

Selection / caret: selection uses a semi-transparent accent; caret tries to follow accent via `QPalette`. Placeholder text keeps readability.

Caveat (popup toggle timing): clicks on the text area land on the internal `QLineEdit`. The implementation uses an event filter + `QTimer::singleShot(0, ...)` to defer toggling, avoiding Qt immediately dismissing a `Qt::Popup` during the same mouse event.

Key APIs:

- Inherits `QDateEdit` (use `setDate()` / `date()` / `setDisplayFormat()` etc.)
- `setTodayText()` / `todayText()` to configure the popup's Today button text.
- Inherited `setLocale()` / `locale()` from `QWidget` to control weekday labels, month names, and first-day-of-week behavior.
- `hoverLevel` / `focusLevel` (Q_PROPERTY)

Demo: Pickers / Overview.

---

## FluentDatePicker

```cpp
#include "Fluent/FluentDatePicker.h"

auto *picker = new Fluent::FluentDatePicker();
picker->setDate(QDate::currentDate());
picker->setMonthPlaceholderText(QStringLiteral("Month"));
picker->setDayPlaceholderText(QStringLiteral("Day"));
picker->setYearPlaceholderText(QStringLiteral("Year"));
picker->setVisibleParts(Fluent::FluentDatePicker::MonthPart
						| Fluent::FluentDatePicker::DayPart
						| Fluent::FluentDatePicker::YearPart);
```

Purpose: a wheel-style date picker. Clicking the control opens a column-based popup where month / day / year snap to the centered selection slot, then commit through the bottom accept / cancel bar.

Inheritance & construction:

- `class FluentDatePicker : public QWidget`
- Constructor: `FluentDatePicker(QWidget*)`

Visual / interaction notes:

- The closed control renders segmented fields for `month / day / year`, and keeps placeholder text when no date is set.
- The popup uses the same Fluent popup surface treatment as combo boxes and menus.
- Each column supports mouse wheel, drag scrolling, and keyboard up/down navigation. Wheel changes now animate before snapping into the centered slot.
- The bottom action bar is split into Accept / Cancel regions, matching the gallery-style picker flow more closely.
- The default locale is Chinese (China), and date formatting text is resolved through the widget's own `locale()`.

Key APIs:

- `setDate()` / `date()` / `clearDate()` / `hasDate()`
- `setDateRange()` / `setMinimumDate()` / `setMaximumDate()`
- `setVisibleParts()` to control whether month / day / year are shown
- `setMonthDisplayFormat()` / `setDayDisplayFormat()` / `setYearDisplayFormat()` for per-column text formatting
- `setMonthPlaceholderText()` / `setDayPlaceholderText()` / `setYearPlaceholderText()` for empty-state labels. The defaults are the Chinese labels for month / day / year.
- `setLocale()` / `locale()` to localize format strings such as `MMMM` and `ddd`.

Typical use cases:

- compact form input
- month + day only scenarios such as birthdays or reminders
- lightweight scheduling forms alongside `FluentTimePicker`

Demo: Pickers / Overview.

---

## FluentDateRangePicker

```cpp
#include "Fluent/FluentDateRangePicker.h"

auto *picker = new Fluent::FluentDateRangePicker();
picker->setDateRange(QDate::currentDate(), QDate::currentDate().addDays(7));
picker->setStartPrefix(QStringLiteral("From: "));
picker->setSeparator(QStringLiteral("  to  "));
picker->setEndPrefix(QStringLiteral("To: "));
```

Purpose: a date-range input widget. Clicking it opens a dual-panel calendar popup: the left panel represents the start month, the right panel represents the end month, and the selected span is painted with a continuous accent-colored range band.

Inheritance & construction:

- `class FluentDateRangePicker : public QWidget`
- Constructor: `FluentDateRangePicker(QWidget*)`

Visual / interaction notes:

- The control itself uses `Style::paintControlSurface()` and paints a right-side chevron.
- The popup switches `FluentCalendarPopup` into `Range` mode, with left/right panels defaulting to one month apart.
- First click selects the start date, second click selects the end date; hover previews the pending range.
- The in-range area uses a continuous accent band without visible vertical gaps.
- `Esc`: cancels the current range-in-progress first; pressing again closes the popup.

Text customization APIs:

- `setStartPrefix()` / `setStartSuffix()`
- `setEndPrefix()` / `setEndSuffix()`
- `setSeparator()` (default: `"  →  "`)
- `setStartPlaceholder()` / `setEndPlaceholder()`
- `setDisplayFormat()` (default: `"yyyy-MM-dd"`)

Data APIs:

- `setDateRange(const QDate &start, const QDate &end)`
- `startDate()` / `endDate()`
- `dateRangeChanged(const QDate&, const QDate&)`

Typical use cases:

- hotel check-in / check-out
- report filters
- billing / settlement periods
- task start/end dates

Demo: Pickers.

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
- `setTodayText()` / `todayText()` to configure the header button text.
- `setLocale()` / `locale()` to control weekday labels, month names, and first-day-of-week layout.
- `setSelectionMode(SelectionMode::Single / Range)`
- `setDateRange(const QDate&, const QDate&)` / `rangeStart()` / `rangeEnd()`
- `popup()` / `dismiss()`
- `datePicked(const QDate&)` / `rangePicked(const QDate&, const QDate&)` / `dismissed()` signals

Demo: shown indirectly via `FluentCalendarPicker`.

Standalone usage example:

```cpp
#include "Fluent/datePicker/FluentCalendarPopup.h"

auto *popup = new Fluent::FluentCalendarPopup(someButton);
popup->setAnchor(someButton);
popup->setTodayText(QStringLiteral("Go to today"));
popup->setDate(QDate::currentDate());
connect(popup, &Fluent::FluentCalendarPopup::datePicked, this, [](const QDate &d) {
	qDebug() << "picked" << d;
});
popup->popup();
```

---

## FluentTimePicker

```cpp
#include "Fluent/FluentTimePicker.h"

auto *tp = new Fluent::FluentTimePicker();
tp->setHourPlaceholderText(QStringLiteral("Hour"));
tp->setMinutePlaceholderText(QStringLiteral("Minute"));
tp->setAnteMeridiemText(QStringLiteral("AM"));
tp->setPostMeridiemText(QStringLiteral("PM"));
tp->setTime(QTime::currentTime());
```

Purpose: a wheel-style time input. Clicking the control opens hour / minute / AM-PM (or 24-hour) columns and commits the selected value when accepted.

Inheritance & construction:

- `class FluentTimePicker : public QTimeEdit`
- Constructor: `FluentTimePicker(QWidget*)`

Visual / interaction notes:

- The closed control shows placeholder text until a value is chosen. By default those labels are Chinese (`时 / 分 / 上午`).
- The popup uses the same wheel picker surface as `FluentDatePicker`, with snapping columns and a bottom accept / cancel bar.
- Wheel switching is animated before the column snaps back to the centered selection slot.
- The right-side chevron region is preserved so the control still reads like a form field.
- Supports empty state, minute stepping, and switching between 12-hour and 24-hour layouts.

Key APIs:

- Inherits `QTimeEdit` (use `setTime()` / `time()` / `setDisplayFormat()` etc.)
- `clearTime()` / `hasTime()` for empty-state handling
- `setUse24HourClock(bool)` to switch 12h / 24h layout
- `setMinuteIncrement(int)` to configure minute stepping (for example 5-minute intervals)
- `setHourPlaceholderText()` / `setMinutePlaceholderText()` for empty-state labels.
- `setAnteMeridiemText()` / `setPostMeridiemText()` for the 12-hour period labels.
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
