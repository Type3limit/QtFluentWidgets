# 选择器

## 控件清单

- `FluentCalendarPicker`（include: `Fluent/FluentCalendarPicker.h`）
- `FluentDatePicker`（include: `Fluent/FluentDatePicker.h`）
- `FluentDateRangePicker`（include: `Fluent/FluentDateRangePicker.h`）
- `FluentCalendarPopup`（include: `Fluent/datePicker/FluentCalendarPopup.h`）
- `FluentTimePicker`（include: `Fluent/FluentTimePicker.h`）
- `FluentColorPicker`（include: `Fluent/FluentColorPicker.h`）
- `FluentColorDialog`（include: `Fluent/FluentColorDialog.h`）

Demo 页面：Pickers（`demo/pages/PagePickers.cpp`）与 Overview（`demo/pages/PageOverview.cpp`）。

## FluentCalendarPicker

```cpp
#include "Fluent/FluentCalendarPicker.h"

auto *picker = new Fluent::FluentCalendarPicker();
picker->setTodayText(QStringLiteral("回到今天"));
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
- 弹层里的星期标题、月份名称和每周起始列会跟随控件自身 `locale()`；默认 locale 为中文（中国）。
- Today 按钮文案可通过 `setTodayText()` 自定义，例如“回到今天”。

键盘快捷键：

- `Alt+Down` 或 `F4`：切换弹窗显示/隐藏（与 `QComboBox` 类似）。
- `Esc`：当弹窗已打开时关闭弹窗。

文本选区/光标：选区背景使用 accent 半透明色；caret 尝试跟随 accent（通过 `QPalette`），placeholder（Qt>=5.12）保持可读性。

关键 API：

- 继承自 `QDateEdit`，可直接使用 `setDate()` / `date()` / `setDisplayFormat()` 等 Qt API。
- `setTodayText()` / `todayText()`：配置弹层头部 Today 按钮文本。
- 继承自 `QWidget` 的 `setLocale()` / `locale()`：控制星期、月份名和一周起始日。
- `hoverLevel` / `focusLevel`（Q_PROPERTY）：动效层。

注意事项：

- 点击输入框文本区域（落在内部 `QLineEdit` 上）也会打开弹窗；内部通过 eventFilter + `QTimer::singleShot(0, ...)` 延迟切换，避免 `Qt::Popup` 立即被 Qt 自己 dismiss。

Demo：Pickers / Overview。

---

## FluentDatePicker

```cpp
#include "Fluent/FluentDatePicker.h"

auto *picker = new Fluent::FluentDatePicker();
picker->setDate(QDate::currentDate());
picker->setMonthPlaceholderText(QStringLiteral("月份"));
picker->setDayPlaceholderText(QStringLiteral("日期"));
picker->setYearPlaceholderText(QStringLiteral("年份"));
picker->setVisibleParts(Fluent::FluentDatePicker::MonthPart
						| Fluent::FluentDatePicker::DayPart
						| Fluent::FluentDatePicker::YearPart);
```

用途：滚轮式日期选择器。点击后弹出列式选择面板，月 / 日 / 年会吸附到中间高亮位，并在底部通过确认 / 取消完成提交。

继承与构造：

- `class FluentDatePicker : public QWidget`
- 构造：`FluentDatePicker(QWidget*)`

外观 / 交互要点：

- 关闭状态下使用分段字段展示 `month / day / year`，空状态保留占位文案。
- 弹层使用 `Qt::Popup` + Fluent popup surface，风格与组合框 / 菜单弹层一致。
- 每一列都支持鼠标滚轮、拖动滚动和键盘上下键；滚轮切换带缓动动画，结束后会自动吸附到中心选中位。
- 底部操作栏分为确认 / 取消两个区域，行为更接近 WinUI Gallery 的 DatePicker。
- 默认 locale 为中文（中国），月份/日期/年份格式化文本走控件自身 `locale()`。

关键 API：

- `setDate()` / `date()` / `clearDate()` / `hasDate()`
- `setDateRange()` / `setMinimumDate()` / `setMaximumDate()`
- `setVisibleParts()`：控制是否显示月份 / 日期 / 年份列
- `setMonthDisplayFormat()` / `setDayDisplayFormat()` / `setYearDisplayFormat()`：分别配置三列的显示文本
- `setMonthPlaceholderText()` / `setDayPlaceholderText()` / `setYearPlaceholderText()`：分别配置空状态下三列占位文案，默认是“月 / 日 / 年”。
- `setLocale()` / `locale()`：控制 `MMMM`、`ddd` 等日期格式对应的本地化文本。

典型用法：

- 普通表单日期录入
- 仅显示“月 + 日”的生日 / 提醒场景
- 需要和 TimePicker 一起使用的轻量级时间计划录入

Demo：Pickers / Overview。

---

## FluentDateRangePicker

```cpp
#include "Fluent/FluentDateRangePicker.h"

auto *picker = new Fluent::FluentDateRangePicker();
picker->setDateRange(QDate::currentDate(), QDate::currentDate().addDays(7));
picker->setStartPrefix(QStringLiteral("开始："));
picker->setSeparator(QStringLiteral("  至  "));
picker->setEndPrefix(QStringLiteral("结束："));
```

用途：日期范围选择输入框。点击后会弹出双面板日历：左侧选择开始月份，右侧选择结束月份，范围区间使用连续的 accent 背景高亮。

继承与构造：

- `class FluentDateRangePicker : public QWidget`
- 构造：`FluentDateRangePicker(QWidget*)`

外观/交互要点：

- 控件自身使用 `Style::paintControlSurface()` 绘制输入框表面，右侧带下拉 chevron。
- 弹出的 `FluentCalendarPopup` 会切到 `Range` 模式，左右两个月份面板默认相差 1 个月。
- 第一次点击日期选开始，第二次点击选结束；悬停时会实时预览范围。
- 开始/结束日期之间的区间使用连续 accent 带填充，中间不留竖线缝隙。
- `Esc`：若当前正在选结束日期，则取消本次范围选择；再次按下关闭弹窗。

文本配置 API：

- `setStartPrefix()` / `setStartSuffix()`
- `setEndPrefix()` / `setEndSuffix()`
- `setSeparator()`：设置中间分隔文本（默认 `"  →  "`）
- `setStartPlaceholder()` / `setEndPlaceholder()`
- `setDisplayFormat()`：日期格式（默认 `"yyyy-MM-dd"`）

数据 API：

- `setDateRange(const QDate &start, const QDate &end)`
- `startDate()` / `endDate()`
- `dateRangeChanged(const QDate&, const QDate&)`

适用场景：

- 酒店入住 / 离店
- 报表时间范围
- 账期、结算区间、任务起止时间

Demo：Pickers。

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
- `setTodayText()` / `todayText()`：配置头部 Today 按钮文案。
- `setLocale()` / `locale()`：控制星期标题、月份名和一周起始日。
- `setSelectionMode(SelectionMode::Single / Range)`：切换单日 / 范围模式。
- `setDateRange(const QDate&, const QDate&)` / `rangeStart()` / `rangeEnd()`：范围模式下的起止日期。
- `popup()` / `dismiss()`：显示/关闭。
- `datePicked(const QDate&)` / `rangePicked(const QDate&, const QDate&)` / `dismissed()`：信号。

单独使用示例：

```cpp
#include "Fluent/datePicker/FluentCalendarPopup.h"

auto *popup = new Fluent::FluentCalendarPopup(someButton);
popup->setAnchor(someButton);
popup->setTodayText(QStringLiteral("回到今天"));
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
tp->setHourPlaceholderText(QStringLiteral("小时"));
tp->setMinutePlaceholderText(QStringLiteral("分钟"));
tp->setAnteMeridiemText(QStringLiteral("上午"));
tp->setPostMeridiemText(QStringLiteral("下午"));
tp->setTime(QTime::currentTime());
```

用途：滚轮式时间选择输入框。点击后会弹出小时 / 分钟 / AM-PM（或 24 小时制）列式选择面板，并在确认后提交结果。

继承与构造：

- `class FluentTimePicker : public QTimeEdit`
- 构造：`FluentTimePicker(QWidget*)`

外观/交互要点：

- 关闭状态下展示占位文案；默认是中文的“时 / 分 / 上午”，如果已有值则显示当前时间。
- 弹层使用与 `FluentDatePicker` 相同的 wheel picker popup，所有列支持滚动吸附。
- 滚轮切换带缓动动画，快速切换时仍会稳定吸附到中心选中位。
- 保留右侧 chevron 区域，外观上仍然是表单输入控件，而不是单独的按钮。
- 支持空状态、`minuteIncrement` 和 24 小时制切换。

关键 API：

- 继承自 `QTimeEdit`：`setTime()` / `time()` / `setDisplayFormat()`。
- `clearTime()` / `hasTime()`：控制空状态。
- `setUse24HourClock(bool)`：切换 12 / 24 小时制。
- `setMinuteIncrement(int)`：配置分钟列步进值，例如 5 分钟一档。
- `setHourPlaceholderText()` / `setMinutePlaceholderText()`：配置空状态下小时 / 分钟占位文案。
- `setAnteMeridiemText()` / `setPostMeridiemText()`：配置 12 小时制下的上午 / 下午文案。
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
