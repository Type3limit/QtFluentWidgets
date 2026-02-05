# 选择器

## 控件清单

- `FluentCalendarPicker`（include: `Fluent/FluentCalendarPicker.h`）
- `FluentCalendarPopup`（include: `Fluent/datePicker/FluentCalendarPopup.h`）
- `FluentTimePicker`（include: `Fluent/FluentTimePicker.h`）
- `FluentColorPicker`（include: `Fluent/FluentColorPicker.h`）
- `FluentColorDialog`（include: `Fluent/FluentColorDialog.h`）

Demo 页面：Pickers（`demo/pages/PagePickers.cpp`）与 Overview（`demo/pages/PageOverview.cpp`）。

## FluentCalendarPicker

```cpp
#include "Fluent/FluentCalendarPicker.h"

auto *picker = new Fluent::FluentCalendarPicker();
picker->setDate(QDate::currentDate());
```

用途：日期选择输入框，点击/按键弹出 Fluent 风格日历弹窗。

关键 API：

- 继承自 `QDateEdit`，可直接使用 `setDate()` / `date()` / `setDisplayFormat()` 等 Qt API。
- `hoverLevel` / `focusLevel`（Q_PROPERTY）：动效层。

Demo：Pickers / Overview。

---

## FluentCalendarPopup

用途：`FluentCalendarPicker` 使用的弹出式日历（自绘 `Qt::Popup`），也可单独作为 popup 使用。

关键 API：

- `setAnchor(QWidget*)`：设置锚点控件（用于定位）。
- `setDate(const QDate&)` / `date()`：当前选择日期。
- `popup()` / `dismiss()`：显示/关闭。
- `datePicked(const QDate&)` / `dismissed()`：信号。

Demo：由 `FluentCalendarPicker` 间接展示（Pickers / Overview）。

## FluentTimePicker

```cpp
#include "Fluent/FluentTimePicker.h"

auto *tp = new Fluent::FluentTimePicker();
tp->setTime(QTime::currentTime());
```

用途：时间选择输入框（自绘 stepper + hover/focus 动效）。

关键 API：

- 继承自 `QTimeEdit`：`setTime()` / `time()` / `setDisplayFormat()`。
- `hoverLevel` / `focusLevel`（Q_PROPERTY）：动效层。

Demo：Pickers / Overview。

## FluentColorPicker

```cpp
#include "Fluent/FluentColorPicker.h"

auto *cp = new Fluent::FluentColorPicker();
cp->setColor(QColor("#2D7DFF"));
```

用途：颜色选择输入控件（预览 + 按钮打开颜色对话框）。

关键 API：

- `setColor(const QColor&)` / `color()`
- `colorChanged(const QColor&)`

Demo：Pickers / Overview。

## FluentColorDialog

```cpp
#include "Fluent/FluentColorDialog.h"

auto *dlg = new Fluent::FluentColorDialog(QColor("#2D7DFF"), parent);
dlg->exec();
```

用途：Fluent 风格颜色对话框（支持重置颜色、拖动窗口、边框效果）。

关键 API：

- `currentColor()` / `setCurrentColor(const QColor&)`
- `selectedColor()`：用户确认后的颜色。
- `setResetColor(const QColor&)` / `resetColor()`
- `colorChanged(const QColor&)`

Demo：Pickers / Overview。
