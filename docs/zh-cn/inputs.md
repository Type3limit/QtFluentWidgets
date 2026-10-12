# 输入与编辑

本模块为常见输入/编辑控件（不含日期时间/颜色等 Picker，也不含代码编辑器），同时收录角度输入类控件。

Demo 页面：Input（`demo/pages/PageInputs.cpp`，并合并了 `demo/pages/PageAngleControls.cpp` 的角度控件）与 Overview（`demo/pages/PageOverview.cpp`）。Input 页顶部提供 P0 Input State Matrix，可横向检查文本、数值/组合与选择控件的 Ready / Active / Disabled 状态。

## 控件清单（对应公开头文件）

- `FluentLineEdit`（include: `Fluent/FluentLineEdit.h`）
- `FluentAutoSuggestBox` / `FluentSearchBox`（include: `Fluent/FluentAutoSuggestBox.h`）
- `FluentKeySequenceEdit`（include: `Fluent/FluentKeySequenceEdit.h`）
- `FluentTextEdit`（include: `Fluent/FluentTextEdit.h`）
- `FluentComboBox`（include: `Fluent/FluentComboBox.h`）
- `FluentSpinBox` / `FluentDoubleSpinBox`（include: `Fluent/FluentSpinBox.h`）
- `FluentSlider`（include: `Fluent/FluentSlider.h`）
- `FluentProgressBar`（include: `Fluent/FluentProgressBar.h`）
- `FluentProgressRing`（include: `Fluent/FluentProgressRing.h`）
- `FluentDial`（include: `Fluent/FluentDial.h`）
- `FluentAngleSelector`（include: `Fluent/FluentAngleSelector.h`）

相关但归类到容器模块：

- `FluentScrollBar` / `FluentScrollArea`：见「容器 / 布局」文档。

> `FluentCodeEditor` 单独在「代码编辑器」文档说明。

---

## FluentLineEdit

用途：单行输入框（`QLineEdit`），使用 `Style::paintControlSurface()` 统一绘制圆角 surface + 1px border，并用完整 accent focus ring 表示焦点状态，提供 hover/focus 动效。

继承与构造：

- `class FluentLineEdit : public QLineEdit`
- 构造：`FluentLineEdit(QWidget*)`、`FluentLineEdit(const QString&, QWidget*)`

外观/交互要点：

- 关闭原生 frame（`setFrame(false)`），并用 `Style::metrics()` 设置左右内边距与最小高度。
- 选区背景使用 `accent.base` token 的半透明色（浅色/深色模式不同 alpha）。
- 光标（caret）尝试跟随 `accent.base`（通过 `QPalette::Text`），但文本颜色仍由样式表控制；placeholder（Qt>=5.12）保持可读性，不跟随 accent。
- 焦点状态使用完整 2px `accent.base` focus ring；fallback QSS 同样使用完整 focus border 并解析到当前 token。
- hover/focus 使用 `FluentMotionRole::Hover` / `Focus`；关闭全局动画后，下一次 hover/focus 会直接落到最终状态。

主题联动：监听 `ThemeManager::themeChanged` 与 `EnabledChange`，自动更新样式表/调色板并重绘。

关键 API：

- 继承自 `QLineEdit`，可直接使用 `setPlaceholderText()` / `setText()` 等 Qt API。
- `hoverLevel` / `focusLevel`（Q_PROPERTY）：动效层。

Demo：Inputs / Containers / Overview。

示例：

```cpp
#include "Fluent/FluentLineEdit.h"

auto *le = new Fluent::FluentLineEdit();
le->setPlaceholderText(QStringLiteral("输入…"));
```

建议用法：如果你需要“搜索框”一类的图标/按钮组合，推荐外层用 `FluentCard`/`FluentWidget` 包一层，再用布局组合（该控件本身不内置 leading/trailing icon）。

---

## FluentAutoSuggestBox / FluentSearchBox

用途：带候选建议的单行输入与搜索框组合。`FluentAutoSuggestBox` 负责输入建议，`FluentSearchBox` 在其基础上显示搜索按钮并发出提交信号。

继承与构造：

- `class FluentAutoSuggestBox : public QWidget`
- `class FluentSearchBox : public FluentAutoSuggestBox`
- 构造：`FluentAutoSuggestBox(QWidget*)`、`FluentSearchBox(QWidget*)`

关键 API：

- `setSuggestions(const QStringList&)` / `suggestions()`：设置候选项。
- `setPlaceholderText()` / `text()` / `setText()`：输入文本。
- `lineEdit()`：访问内部 `FluentLineEdit` 以做更细粒度配置。
- `submitted(const QString&)`：回车或点击搜索按钮时发出。
- `suggestionChosen(const QString&)`：用户接受补全项时发出。
- `searchRequested(const QString&)`：搜索提交语义信号。

示例：

```cpp
#include "Fluent/FluentAutoSuggestBox.h"

auto *search = new Fluent::FluentSearchBox();
search->setPlaceholderText(QStringLiteral("Search controls"));
search->setSuggestions({QStringLiteral("FluentButton"), QStringLiteral("FluentCard")});
connect(search, &Fluent::FluentSearchBox::submitted, this, [](const QString &text) {
    qDebug() << "search:" << text;
});
```

注意事项：

- 建议弹层使用库内自绘 popup，与 `FluentComboBox` 一致采用圆角面板、Fluent 滚动条和自绘候选项。
- 输入框重新获得焦点且已有查询文本时，会重新显示匹配建议。弹层位置使用共享 Fluent placement helper，根据空间向上或向下展开，并保持内容区域不压到输入锚点。
- 支持 reduced motion：关闭全局动画后，popup reveal 会立即完成并清除临时 mask/opacity。
- `FluentSearchBox` 默认显示搜索按钮；如果只需要建议输入，用 `FluentAutoSuggestBox`。

Demo：Inputs / Overview。

---

## FluentKeySequenceEdit

用途：快捷键输入框（`QKeySequenceEdit`），视觉上与其他输入入口保持一致，使用统一 rounded surface、完整 accent focus ring 与 hover/focus 动效。

继承与构造：

- `class FluentKeySequenceEdit : public QKeySequenceEdit`
- 构造：`FluentKeySequenceEdit(QWidget*)`、`FluentKeySequenceEdit(const QKeySequence&, QWidget*)`

外观/交互要点：

- 内部编辑器会同步 theme palette，选区与 placeholder 颜色遵循输入控件 token。
- `sizeHint()` / `minimumSizeHint()` 使用 Fluent 输入高度与内边距，适合直接放入表单行。
- hover/focus 使用 `FluentMotionRole::Hover` / `Focus`；关闭全局动画后会直接落到最终状态。

关键 API：

- 继承自 `QKeySequenceEdit`，可直接使用 `setKeySequence()` / `keySequence()`。
- `hoverLevel` / `focusLevel`（Q_PROPERTY）：动效层。

示例：

```cpp
#include "Fluent/FluentKeySequenceEdit.h"

auto *shortcut = new Fluent::FluentKeySequenceEdit(QKeySequence(QStringLiteral("Ctrl+K")));
```

Demo：Inputs / Overview。

---

## FluentTextEdit

用途：多行文本编辑器（`QTextEdit`），同样使用 `Style::paintControlSurface()` 绘制输入面板风格，并额外用一个透明的 border overlay 保证 border 与整体 focus ring 始终盖在 viewport 内容之上（避免滚动内容覆盖边框/焦点反馈的观感问题）。

继承与构造：

- `class FluentTextEdit : public QTextEdit`
- 构造：`FluentTextEdit(QWidget*)`

外观/交互要点：

- 关闭原生 frame（`QFrame::NoFrame`），并用 `Style::metrics()` 设置 viewport margins（文本内边距）与最小高度。
- 默认替换水平/垂直滚动条为 `FluentScrollBar`（保持 Fluent 风格）。
- 选区背景与 caret 行为与 `FluentLineEdit` 一致：选区用半透明 `accent.base`；caret 尝试跟随 `accent.base`。
- 多行编辑器聚焦时与单行输入保持一致，使用完整 `accent.base` focus ring，避免大高度文本框出现突兀的整宽底线。
- `sizeHint()` 默认返回 `200x80`，适合表单场景。
- hover/focus 使用 `FluentMotionRole::Hover` / `Focus`；关闭全局动画后会直接落到最终状态。

主题联动：为避免控件未显示时触发 viewport 的早期 paint，主题应用会在首次 show 时完成一次（之后跟随 `themeChanged` 更新）。

关键 API：

- 继承自 `QTextEdit`，可直接使用 `setPlaceholderText()` / `setPlainText()` 等 Qt API。
- `hoverLevel` / `focusLevel`（Q_PROPERTY）：动效层。

示例：

```cpp
#include "Fluent/FluentTextEdit.h"

auto *te = new Fluent::FluentTextEdit();
te->setPlaceholderText(QStringLiteral("输入多行文本…"));
te->setAcceptRichText(false);
```

Demo：Inputs / Containers / Windows / Overview。

---

## FluentComboBox

用途：下拉选择框（`QComboBox`），自绘输入面板（surface/border/完整 focus ring），并替换 popup view + item delegate，使下拉弹层拥有圆角、Fluent 风格 hover/selected、以及 Fluent 滚动条。

继承与构造：

- `class FluentComboBox : public QComboBox`
- 构造：`FluentComboBox(QWidget*)`

弹层（popup）行为：

- 含换行符的项目按各自文本计算行高，单行项目保持最小行高；项目顺序不会改变其他行的高度。
- `setView(new FluentComboPopupView)`：内部使用 `QListView` 的自定义子类。
- popup 容器使用库内 `FluentComboPopup`，自绘 rounded panel / shadow / accent trace，并复用 ComboBox / AutoSuggest 的 popup placement 与 reveal 行为。
- `showPopup()` 会根据屏幕空间决定向下或向上展开，并保持约 `5px` 的 anchor 间距。
- 每行最小高度为 `36px`，enabled hover item 使用 neutral token tint 和 5px 圆角；enabled selected item 左侧绘制 3px accent indicator；disabled item 即使成为 current row，也只使用 disabled text/neutral 处理，不显示 accent indicator。
- 多选模式的 checkbox glyph 在 enabled checked 行使用 accent / `onAccent` token，在 bright accent 和暗色模式下保持可读；disabled checked 行保留 neutral disabled surface，并用 `disabledText` 表示勾选状态。

关键 API：

- 继承自 `QComboBox`，可直接使用 `addItem(s)` / `setCurrentIndex()`。
- `hoverLevel`（Q_PROPERTY）：动效层。
- `setPopupScrollThreshold(int)`：超过该项数时弹层启用垂直滚动条（默认值取 `maxVisibleItems()`）。
- 多选模式（`SelectionMode`）：
	- `setSelectionMode(FluentComboBox::MultiSelection)` 启用多选；默认 `SingleSelection`。
	- 多选模式下弹层每行前会绘制复选框，点击切换勾选且不会关闭弹窗（按 Esc 或点击外部关闭）。
	- 输入框文本显示已勾选项的拼接；为空时显示 `setMultiSelectionPlaceholder()` 设置的占位文本。
	- `setItemChecked(i,bool)` / `isItemChecked(i)` / `setCheckedIndexes(QList<int>)` / `checkedIndexes()` / `checkedTexts()` / `clearChecked()`。
	- 信号 `checkedIndexesChanged(QList<int>)` 在勾选集合变化时发出。

Demo：Pickers / Overview / Inputs（"FluentComboBox（多选）" 段落）。

---

## FluentSpinBox / FluentDoubleSpinBox

用途：数字输入（`QSpinBox` / `QDoubleSpinBox`），去掉原生按钮并自绘右侧 stepper 区域（上/下），包含 hover/focus 动效与 stepper 按压反馈。

继承与构造：

- `class FluentSpinBox : public QSpinBox`
- `class FluentDoubleSpinBox : public QDoubleSpinBox`
- 构造：各自 `(..., QWidget*)`

外观/交互要点：

- `setButtonSymbols(QAbstractSpinBox::NoButtons)`，stepper 区域宽度来自 `Style::metrics().iconAreaWidth`。
- stepper 区域会被 hit-test 为 Up/Down 两个半区：
	- 左键按下即触发一次 `stepUp()` / `stepDown()`。
	- 鼠标移入 stepper 区会把光标变为 `PointingHandCursor`；移出恢复 `IBeamCursor`。
- 内部 `QLineEdit` 会安装事件过滤器，把发生在编辑器上的鼠标事件映射回 SpinBox，从而保证 stepper 区域即使被 editor 覆盖也能正确响应。
- `sizeHint()` 会根据 `prefix/suffix/min/max/value/specialValueText` 估算一个更合理的最小宽度，尽量避免数字被截断（你仍可用 `setMinimumWidth()` 覆盖）。
- 选区背景/placeholder/caret 的处理与 `FluentLineEdit` 类似（`accent.base` selection + caret accent）。
- 焦点反馈使用与其他输入入口一致的完整 2px `accent.base` focus ring；右侧 stepper 分隔线复用输入描边 token（禁用态混入 `disabledText`），hover/pressed 填充也由 theme token 派生。
- hover/focus 与 stepper 强调使用 `FluentMotionRole::Hover` / `Focus`；关闭全局动画后会直接落到最终状态。

关键 API：

- 继承自 `QSpinBox` / `QDoubleSpinBox`，使用 Qt 原生 API：`setRange()` / `setValue()` / `setDecimals()`。

示例：

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

Demo：Inputs / Overview。

---

## FluentSlider

用途：滑动条（`QSlider`），自绘 track + fill + handle；handle 位置使用动画过渡（非拖拽时），并支持“点击轨道跳转 + 立即进入拖拽”。

继承与构造：

- `class FluentSlider : public QSlider`
- 构造：`FluentSlider(Qt::Orientation, QWidget*)`

交互要点：

- `handlePos`（0~1）用于绘制位置；当 `valueChanged` 且未处于 `isSliderDown()` 时会按 `FluentMotionRole::Selection` 的时长与曲线平滑过渡；关闭全局动效或把 Selection 时长设为 0 时会直接落到目标位置。
- 点击 handle：进入拖拽（记录 drag offset）；点击轨道：先跳转到点击位置，再进入拖拽（无需再次点击 handle）。
- track、active fill、handle surface 与边框均从当前 theme token 派生；active fill 和 hover handle accent border 使用 `accent.base`，暗色模式不会回退到固定白色 handle。
- hover 时 handle 会变大，并显示 accent 内点。

关键 API：

- 继承自 `QSlider`，使用 Qt 原生 API：`setRange()` / `setValue()`。
- `handlePos` / `hoverLevel`（Q_PROPERTY）：位置与交互动效层。

示例：

```cpp
#include "Fluent/FluentSlider.h"

auto *sl = new Fluent::FluentSlider(Qt::Horizontal);
sl->setRange(0, 100);
sl->setValue(30);
connect(sl, &QSlider::valueChanged, this, [](int v) {
	qDebug() << "value:" << v;
});
```

Demo：Inputs / Overview。

---

## FluentProgressBar

用途：进度条（`QProgressBar`），细条（约 4px）track + `accent.base` fill，并对 value 变化做平滑过渡（避免跳变）。支持把百分比文本放在左/中/右或隐藏。

继承与构造：

- `class FluentProgressBar : public QProgressBar`
- 构造：`FluentProgressBar(QWidget*)`

交互/绘制要点：

- `displayValue` 用于动画显示值：`valueChanged(int)` 时会驱动属性动画把 `displayValue` 从旧值过渡到新值。
- 文本内容默认绘制为百分比（基于 `displayValue / (maximum-minimum)`），而不是 `QProgressBar::text()`。
- 轨道使用当前 neutral token 的 subtle stroke/fill，填充使用 `accent.base`；禁用态会把 `accent.base` 混合到 neutral stroke 并降低 alpha。

关键 API：

- `displayValue`（Q_PROPERTY）：用于动画显示（通常由控件内部驱动）。
- `setTextPosition(TextPosition)`：控制文本显示位置（Left/Center/Right/None）。
- `setTextColor(const QColor&)`：文本颜色。

示例：

```cpp
#include "Fluent/FluentProgressBar.h"

auto *pb = new Fluent::FluentProgressBar();
pb->setRange(0, 100);
pb->setValue(66);
pb->setTextPosition(Fluent::FluentProgressBar::TextPosition::Right);
```

Demo：Buttons / Containers / Overview。

---

## FluentProgressRing

用途：环形进度指示器，支持确定进度与 indeterminate 忙碌态，适合紧凑状态区域、按钮旁状态、加载中的卡片等场景。

继承与构造：

- `class FluentProgressRing : public QProgressBar`
- 构造：`FluentProgressRing(QWidget*)`

关键 API：

- `setRange()` / `setValue()`：继承自 `QProgressBar` 的确定进度。
- `setIndeterminate(true)`：启用忙碌态旋转。
- `setRingWidth(qreal)`：设置圆环宽度。
- 轨道与 `FluentProgressBar` 使用同一 neutral token 派生；确定进度/忙碌弧段使用 `accent.base`，禁用态使用 muted accent token 派生色。

示例：

```cpp
#include "Fluent/FluentProgressRing.h"

auto *ring = new Fluent::FluentProgressRing();
ring->setFixedSize(42, 42);
ring->setRange(0, 100);
ring->setValue(66);

auto *busy = new Fluent::FluentProgressRing();
busy->setIndeterminate(true);
```

Demo：Buttons / Overview。

---

## FluentDial

```cpp
#include "Fluent/FluentDial.h"

auto *dial = new Fluent::FluentDial();
dial->setValue(135);
dial->setTicksVisible(true);
dial->setMajorTickStep(45);
```

用途：紧凑的圆形角度旋钮，适合做渐变角度、旋转角度、朝向选择等输入。

角度语义：

- `0°` = 左 → 右（3 点钟方向）
- 按顺时针递增
- accent 弧从 3 点钟方向开始绘制

关键 API：

- `setValue(int)` / `value()`
- `setTicksVisible(bool)` / `ticksVisible()`
- `setTickStep(int)` / `tickStep()`
- `setMajorTickStep(int)` / `majorTickStep()`
- `setPointerVisible(bool)` / `pointerVisible()`

实现语义：

- 自绘 `QWidget`，内部维护 hover/focus 动画层。
- `setValue(int)` 会规整到 `0..359`；值变化时发出 `valueChanged(int)`。
- 鼠标拖拽会按当前位置换算角度；滚轮可做细粒度调节。
- 可同时绘制外圈轨道、当前 accent 弧、可选刻度 / 主刻度、可选指针和 focus ring；轨道/刻度来自 neutral token，弧线、指针、指示点与 focus ring 来自 `accent.base` ramp，禁用态使用 muted accent token 派生色。

Demo：Input（角度控件部分）/ Overview。

---

## FluentAngleSelector

```cpp
#include "Fluent/FluentAngleSelector.h"

auto *editor = new Fluent::FluentAngleSelector();
editor->setValue(128);
editor->setSuffix(QStringLiteral("°"));
```

用途：由 `FluentLabel + FluentDial + FluentSpinBox` 组合而成的一体化角度编辑器，对外暴露统一的 `valueChanged(int)` 语义。

关键 API：

- `setValue(int)` / `value()`
- `setRange(int minimum, int maximum)`：默认 `0..359`
- `setWrapping(bool)` / `wrapping()`
- `setLabelText(const QString&)` / `labelText()`
- `setSuffix(const QString&)` / `suffix()`
- `setDialVisible(bool)` / `dialVisible()`
- `setLabelVisible(bool)` / `labelVisible()`
- `setSpinBoxVisible(bool)` / `spinBoxVisible()`
- `dial()` / `spinBox()`：获取内部子控件以做更细粒度定制

实现语义：

- Dial 与 SpinBox 双向同步，内部使用临时 `blockSignals(true)` 避免信号回环。
- `setValue(int)` 触发实际值变化时会发出 `valueChanged(int)`。
- wrapping 模式下会把值规整回范围内；非 wrapping 模式下会对值做 clamp。
- 可见性开关可快速做“仅 Dial”“仅 SpinBox”“无标题紧凑表单”等变体。

典型场景：

- `FluentColorDialog` 里的渐变角度编辑
- 图形 / 画布对象旋转
- 任何需要“数字值 + 可视旋钮”双输入方式的场景

Demo：Input（角度控件部分）。
