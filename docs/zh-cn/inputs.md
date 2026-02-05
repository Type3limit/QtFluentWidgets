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

用途：单行输入框，统一的边框/圆角与 focus ring。

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

---

## FluentTextEdit

用途：多行文本编辑器（自带边框 overlay，保证 border/focus ring 可见）。

关键 API：

- 继承自 `QTextEdit`，可直接使用 `setPlaceholderText()` / `setPlainText()` 等 Qt API。
- `hoverLevel` / `focusLevel`（Q_PROPERTY）：动效层。

Demo：Inputs / Containers / Windows / Overview。

---

## FluentComboBox

用途：下拉选择框（hover 动效 + Fluent 绘制）。

关键 API：

- 继承自 `QComboBox`，可直接使用 `addItem(s)` / `setCurrentIndex()`。
- `hoverLevel`（Q_PROPERTY）：动效层。

Demo：Pickers / Overview。

---

## FluentSpinBox / FluentDoubleSpinBox

用途：数字输入（含 stepper 交互与 hover/focus 动效）。

关键 API：

- 继承自 `QSpinBox` / `QDoubleSpinBox`，使用 Qt 原生 API：`setRange()` / `setValue()` / `setDecimals()`。

Demo：Inputs / Overview。

---

## FluentSlider

用途：滑动条（带 handle 位置动画）。

关键 API：

- 继承自 `QSlider`，使用 Qt 原生 API：`setRange()` / `setValue()`。
- `handlePos` / `hoverLevel`（Q_PROPERTY）：位置与交互动效层。

Demo：Inputs / Overview。

---

## FluentProgressBar

用途：进度条（支持显示数值文本位置与动画显示值）。

关键 API：

- `displayValue`（Q_PROPERTY）：用于动画显示（通常由控件内部驱动）。
- `setTextPosition(TextPosition)`：控制文本显示位置（Left/Center/Right/None）。
- `setTextColor(const QColor&)`：文本颜色。

Demo：Buttons / Containers / Overview。
