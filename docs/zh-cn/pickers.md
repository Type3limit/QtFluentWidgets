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

继承与构造：

- `class FluentCalendarPicker : public QDateEdit`
- 构造：`FluentCalendarPicker(QWidget*)`

外观/交互要点：

- 禁用 Qt 自带日历弹窗（`setCalendarPopup(false)`），并去掉原生 spinbox 按钮（`NoButtons`），改为自绘右侧下拉箭头区域。
- 右侧箭头区域宽度来自 `Style::metrics().iconAreaWidth`，并绘制分隔线 + chevron。
- 内部 `QLineEdit` 会设置右侧 text margin 以避开箭头区域（保证文字与光标不被遮挡）。

键盘快捷键：

- `Alt+Down` 或 `F4`：切换弹窗显示/隐藏（与 `QComboBox` 类似）。
- `Esc`：当弹窗已打开时关闭弹窗。

文本选区/光标：选区背景使用 accent 半透明色；caret 尝试跟随 accent（通过 `QPalette`），placeholder（Qt>=5.12）保持可读性。

关键 API：

- 继承自 `QDateEdit`，可直接使用 `setDate()` / `date()` / `setDisplayFormat()` 等 Qt API。
- `hoverLevel` / `focusLevel`（Q_PROPERTY）：动效层。

注意事项：

- 点击输入框文本区域（落在内部 `QLineEdit` 上）也会打开弹窗；内部通过 eventFilter + `QTimer::singleShot(0, ...)` 延迟切换，避免 `Qt::Popup` 立即被 Qt 自己 dismiss。

Demo：Pickers / Overview。

---

## FluentCalendarPopup

用途：`FluentCalendarPicker` 使用的弹出式日历（自绘 `Qt::Popup`），也可单独作为 popup 使用。

继承与构造：

- `class FluentCalendarPopup : public QWidget`
- 构造：`FluentCalendarPopup(QWidget *anchor = nullptr)`（anchor 用于定位）

弹层行为（Popup 语义）：

- Window flags：`Qt::Popup + Frameless + NoDropShadow`，并避免 translucent background（Windows 上浅色模式容易黑底）。
- 自动关闭：
	- 窗口失去激活（`WindowDeactivate` / `ApplicationDeactivate`）时关闭。
	- 通过安装 `qApp` 事件过滤器检测“点击 popup 外部”并关闭；若点击的是 anchor，则会吞掉该次点击，避免“先关闭又立刻被 anchor 打开”的抖动。
- 定位：`popup()` 时会尝试把弹层放在 anchor 下方（或空间不足时放上方），并加一个 gap；内部也会对圆角进行 mask 裁剪。
- 动画：打开时会做淡入 + 轻微上滑；视图模式切换（天/月份/年份）也有过渡动画。

视图模式与交互：

- 模式：Days / Months / Years
- Header：
	- 点击“月份 pill”切换 Days↔Months
	- 点击“年份 pill”切换 Days↔Years
	- Today 按钮可快速跳回今天
- 导航：Prev/Next 在不同模式下分别步进 月 / 年 / 年份分页；鼠标滚轮同样会触发步进。
- 键盘：`Esc` 在非 Days 模式会先回到 Days；在 Days 模式则关闭 popup。

关键 API：

- `setAnchor(QWidget*)`：设置锚点控件（用于定位）。
- `setDate(const QDate&)` / `date()`：当前选择日期。
- `popup()` / `dismiss()`：显示/关闭。
- `datePicked(const QDate&)` / `dismissed()`：信号。

单独使用示例：

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

Demo：由 `FluentCalendarPicker` 间接展示（Pickers / Overview）。

## FluentTimePicker

```cpp
#include "Fluent/FluentTimePicker.h"

auto *tp = new Fluent::FluentTimePicker();
tp->setTime(QTime::currentTime());
```

用途：时间选择输入框（自绘 stepper + hover/focus 动效）。

继承与构造：

- `class FluentTimePicker : public QTimeEdit`
- 构造：`FluentTimePicker(QWidget*)`

外观/交互要点：

- 禁用原生按钮（`NoButtons`），自绘右侧 stepper（上/下）区域；宽度来自 `Style::metrics().iconAreaWidth`。
- stepper 区域 hover/press 会有更明显的 accent tint（提升小点击目标可见性）。
- 内部 `QLineEdit` 通过 eventFilter 转发鼠标事件，确保 stepper 区域可点击且光标形态正确。
- 选区背景/caret/placeholder 处理与 `FluentLineEdit` 一致（accent selection + caret accent）。

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

结构与交互：

- 由一个只读预览输入框（显示 `#RRGGBB`）+ “选择颜色”按钮组成。
- 预览框左侧会显示一个 16x16 的颜色方块（通过 `QLineEdit::addAction(LeadingPosition)` 实现）。
- 点击按钮会打开 `FluentColorDialog`：
	- 对话框打开期间会实时发出 `colorChanged` 并同步到该控件。
	- 若用户取消/对话框自动关闭，会回滚到打开前的颜色（避免“预览改了但没确认”的语义歧义）。

关键 API：

- `setColor(const QColor&)` / `color()`
- `colorChanged(const QColor&)`

示例：

```cpp
#include "Fluent/FluentColorPicker.h"

auto *cp = new Fluent::FluentColorPicker();
cp->setColor(QColor("#2D7DFF"));
connect(cp, &Fluent::FluentColorPicker::colorChanged, this, [](const QColor &c) {
	qDebug() << "color:" << c;
});
```

Demo：Pickers / Overview。

## FluentColorDialog

```cpp
#include "Fluent/FluentColorDialog.h"

auto *dlg = new Fluent::FluentColorDialog(QColor("#2D7DFF"), parent);
dlg->exec();
```

用途：Fluent 风格颜色对话框（支持重置颜色、拖动窗口、边框效果）。

窗口行为要点：

- Window flags：`Qt::Tool + Frameless + NoDropShadow`；避免使用 `Qt::Popup`（在部分 Windows 组合下与透明/无边框窗口的 show/hide/激活切换较容易不稳定）。
- “类 popup”的自动关闭：窗口失去激活时会自动 `reject()`（除非 `m_suppressAutoClose` 为 true，例如吸管取色过程中）。
- 可拖拽标题栏：header widget 安装事件过滤器，按下拖拽移动窗口。

边框/强调效果：内部使用 `FluentBorderEffect` + `FluentFramePainter` 绘制圆角 surface + 1px border + 可选 accent trace；`showEvent` 会播放一次初始 trace。

数据模型（语义）：

- `currentColor()`：当前正在编辑/预览的颜色（内部就是 selected）。
- `selectedColor()`：用户点“确定”后的最终颜色（同上；是否采纳由 `Accepted/Rejected` 决定）。
- `resetColor()`：重置按钮回到的颜色。

最近颜色：点击“确定”会把当前颜色写入 `QSettings`（key：`QtFluent/FluentColorDialog/recent`，最多保留 12 条），并在下次打开显示。

关键 API：

- `currentColor()` / `setCurrentColor(const QColor&)`
- `selectedColor()`：用户确认后的颜色。
- `setResetColor(const QColor&)` / `resetColor()`
- `colorChanged(const QColor&)`

用法建议：

- 如果你希望对话框是“所见即所得”并实时更新外部预览，可连接 `colorChanged`；若你需要“只在确定时生效”，则只在 `Accepted` 后读取 `selectedColor()`。

Demo：Pickers / Overview。
