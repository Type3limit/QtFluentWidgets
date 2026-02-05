# 按钮与开关

本模块包含常见的可点击/可切换控件，均做了 Fluent 风格的圆角、hover/press/focus 动效。

## 控件清单（对应公开头文件）

- `FluentButton`（include: `Fluent/FluentButton.h`）
- `FluentToolButton`（include: `Fluent/FluentToolButton.h`）
- `FluentToggleSwitch`（include: `Fluent/FluentToggleSwitch.h`）
- `FluentCheckBox`（include: `Fluent/FluentCheckBox.h`）
- `FluentRadioButton`（include: `Fluent/FluentRadioButton.h`）

Demo 页面：Buttons（`demo/pages/PageButtons.cpp`），以及 Overview（`demo/pages/PageOverview.cpp`）。

---

## FluentButton

用途：标准按钮，支持 Primary（强调）样式。

关键 API：

- `setPrimary(bool)` / `isPrimary()`：切换 Primary 样式。
- `hoverLevel` / `pressLevel`（Q_PROPERTY）：用于动效（一般无需手动设置）。

Demo：Buttons / Containers / Pickers / Windows / Overview。

示例：

```cpp
#include "Fluent/FluentButton.h"

auto *btn = new Fluent::FluentButton(QStringLiteral("Primary"));
btn->setPrimary(true);
```

---

## FluentToolButton

用途：工具按钮（常用于标题栏/工具栏/卡片折叠箭头等）。

关键 API：

- `setCheckable(bool)` / `setChecked(bool)`：使用 Qt 原生 API。
- `hoverLevel` / `pressLevel`（Q_PROPERTY）：动效层。

Demo：Buttons / Overview（也用于若干容器控件内部）。

---

## FluentToggleSwitch

用途：Win11 风格的开关。

关键 API：

- `setChecked(bool)` / `isChecked()`：开关状态。
- `toggled(bool)`：状态变化信号。
- `setText(const QString&)`：显示标签。
- `progress` / `hoverLevel` / `focusLevel`（Q_PROPERTY）：动画/交互层。

Demo：Buttons / Containers / Windows / Overview。

---

## FluentCheckBox

用途：复选框（含 hover/focus 动效）。

关键 API：

- `setChecked(bool)` / `isChecked()`：使用 Qt 原生 API。
- `hoverLevel` / `focusLevel`（Q_PROPERTY）：动效层。

Demo：Buttons / Overview。

---

## FluentRadioButton

用途：单选按钮（含 hover/focus 动效）。

关键 API：

- `setChecked(bool)` / `isChecked()`：使用 Qt 原生 API。
- `hoverLevel` / `focusLevel`（Q_PROPERTY）：动效层。

Demo：Buttons / Overview。
