# 输入与编辑

本模块为常见输入/编辑控件（不含日期时间/颜色等 Picker，也不含代码编辑器）。

Demo 页面：Inputs（`demo/pages/PageInputs.cpp`）与 Overview（`demo/pages/PageOverview.cpp`）。

## 控件清单（对应公开头文件）

- `FluentLineEdit`（include: `Fluent/FluentLineEdit.h`）
- `FluentTextEdit`（include: `Fluent/FluentTextEdit.h`）
- `FluentComboBox`（include: `Fluent/FluentComboBox.h`）
- `FluentSpinBox` / `FluentDoubleSpinBox`（include: `Fluent/FluentSpinBox.h`）
- `FluentSlider`（include: `Fluent/FluentSlider.h`）
- `FluentProgressBar`（include: `Fluent/FluentProgressBar.h`）

相关但归类到容器模块：

- `FluentScrollBar` / `FluentScrollArea`：见「容器 / 布局」文档。

> `FluentCodeEditor` 单独在「代码编辑器」文档说明。

---

## FluentLineEdit

用途：单行输入框（`QLineEdit`），使用 `Style::paintControlSurface()` 统一绘制圆角 surface + 1px border + focus ring，并提供 hover/focus 动效。

继承与构造：

- `class FluentLineEdit : public QLineEdit`
- 构造：`FluentLineEdit(QWidget*)`、`FluentLineEdit(const QString&, QWidget*)`

外观/交互要点：

- 关闭原生 frame（`setFrame(false)`），并用 `Style::metrics()` 设置左右内边距与最小高度。
- 选区背景使用 accent 的半透明色（浅色/深色模式不同 alpha）。
- 光标（caret）尝试跟随 accent（通过 `QPalette::Text`），但文本颜色仍由样式表控制；placeholder（Qt>=5.12）保持可读性，不跟随 accent。

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

## FluentTextEdit

用途：多行文本编辑器（`QTextEdit`），同样使用 `Style::paintControlSurface()` 绘制输入面板风格，并额外用一个透明的 border overlay 保证 border/focus ring 始终盖在 viewport 内容之上（避免滚动内容覆盖边框/焦点描边的观感问题）。

继承与构造：

- `class FluentTextEdit : public QTextEdit`
- 构造：`FluentTextEdit(QWidget*)`

外观/交互要点：

- 关闭原生 frame（`QFrame::NoFrame`），并用 `Style::metrics()` 设置 viewport margins（文本内边距）与最小高度。
- 默认替换水平/垂直滚动条为 `FluentScrollBar`（保持 Fluent 风格）。
- 选区背景与 caret 行为与 `FluentLineEdit` 一致：选区用半透明 accent；caret 尝试跟随 accent。
- `sizeHint()` 默认返回 `200x80`，适合表单场景。

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

用途：下拉选择框（`QComboBox`），自绘输入面板（surface/border/focus ring），并替换 popup view + item delegate，使下拉弹层拥有圆角、Fluent 风格 hover/selected、以及 Fluent 滚动条。

继承与构造：

- `class FluentComboBox : public QComboBox`
- 构造：`FluentComboBox(QWidget*)`

弹层（popup）行为：

- `setView(new FluentComboPopupView)`：内部使用 `QListView` 的自定义子类。
- popup 容器会被 patch 为无边框（`FramelessWindowHint`）且关闭 drop shadow（`NoDropShadowWindowHint`），并通过 mask 做圆角裁剪，避免 Windows 上 translucent popup 在浅色模式出现黑底。
- `showPopup()` 会在 Qt 默认位置基础上加一个约 `5px` 的间距，并把 popup 位置 clamp 到屏幕可用区域。

关键 API：

- 继承自 `QComboBox`，可直接使用 `addItem(s)` / `setCurrentIndex()`。
- `hoverLevel`（Q_PROPERTY）：动效层。

Demo：Pickers / Overview。

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
- 选区背景/placeholder/caret 的处理与 `FluentLineEdit` 类似（accent selection + caret accent）。

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

- `handlePos`（0~1）用于绘制位置；当 `valueChanged` 且未处于 `isSliderDown()` 时会以 100ms 动画平滑过渡。
- 点击 handle：进入拖拽（记录 drag offset）；点击轨道：先跳转到点击位置，再进入拖拽（无需再次点击 handle）。
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

用途：进度条（`QProgressBar`），细条（约 4px）track + accent fill，并对 value 变化做平滑过渡（避免跳变）。支持把百分比文本放在左/中/右或隐藏。

继承与构造：

- `class FluentProgressBar : public QProgressBar`
- 构造：`FluentProgressBar(QWidget*)`

交互/绘制要点：

- `displayValue` 用于动画显示值：`valueChanged(int)` 时会驱动属性动画把 `displayValue` 从旧值过渡到新值。
- 文本内容默认绘制为百分比（基于 `displayValue / (maximum-minimum)`），而不是 `QProgressBar::text()`。
- 轨道使用 `colors.border`，填充使用 `colors.accent`；左右有固定 inset（约 12px）。

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
