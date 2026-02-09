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

用途：标准按钮（`QPushButton`），Fluent 风格圆角/边框 + hover/press 动效 + focus ring。支持 Primary（强调/填充）样式，并对“可勾选按钮”（`setCheckable(true)`）做了更接近 Win11 的选中表现。

继承与构造：

- `class FluentButton : public QPushButton`
- 构造：`FluentButton(QWidget*)`、`FluentButton(const QString&, QWidget*)`

外观/交互要点：

- 最小高度来自 `Style::metrics().height`（更贴近 Win11 控件高度）。
- Secondary（默认）：surface 填充 + border；若设置为 checkable 且 checked，会出现轻微 accent tint + accent 边框，并把文本颜色略偏向 accent。
- Primary：accent 填充；若 checkable 且 checked，会额外绘制一层内侧高光以强化“已选中”。
- 有图标时：图标绘制在左侧，文本居中对齐图标组（icon + gap + text）。

主题联动：控件会监听 `ThemeManager::themeChanged`，以及自身 `EnabledChange`，自动触发重绘。

关键 API：

- `setPrimary(bool)` / `isPrimary()`：切换 Primary 样式。
- `setCheckable(bool)` / `setChecked(bool)` / `toggled(bool)`：继承自 `QAbstractButton` 的原生能力；本控件对 checked 状态有额外绘制细节。
- `setIcon(const QIcon&)` / `setIconSize(const QSize&)`：原生 API；本控件会使用 `icon()` + `iconSize()` 参与布局。
- `hoverLevel` / `pressLevel`（Q_PROPERTY）：动效层（通常由控件内部动画驱动；仅在你要做自定义动画/测试时手动设置）。

Demo：Buttons / Containers / Pickers / Windows / Overview。

示例：

```cpp
#include "Fluent/FluentButton.h"

auto *btn = new Fluent::FluentButton(QStringLiteral("Primary"));
btn->setPrimary(true);
```

带图标与 checkable 的示例：

```cpp
auto *toggle = new Fluent::FluentButton(QStringLiteral("Pin"));
toggle->setCheckable(true);
toggle->setIcon(QIcon(":/icons/pin.svg"));
toggle->setIconSize(QSize(16, 16));
connect(toggle, &QAbstractButton::toggled, this, [](bool on) {
	qDebug() << "Pinned:" << on;
});
```

注意事项：

- 这是自绘按钮（重载 `paintEvent`），如果你依赖复杂的 QSS 对按钮背景/边框进行覆盖，可能不会生效；推荐通过 `ThemeManager`/`ThemeColors` 调整整体配色。

---

## FluentToolButton

用途：工具按钮（`QToolButton`），适合标题栏/工具栏/卡片折叠箭头等紧凑交互。默认启用 `autoRaise`（更接近 Win11 工具按钮观感），并提供 hover/press 动效与 focus ring。

继承与构造：

- `class FluentToolButton : public QToolButton`
- 构造：`FluentToolButton(QWidget*)`、`FluentToolButton(const QString&, QWidget*)`

外观/交互要点：

- 默认 `setAutoRaise(true)`，并使用 `Style::metrics().height` 作为最小高度。
- 若 `setCheckable(true)` 且 checked，会使用“轻微 accent tint + accent 边框”的方式表现选中状态，并把 label 颜色略偏向 accent。

标题栏窗口按钮（内部约定）：

当设置动态属性 `fluentWindowGlyph`（int）时，控件会按“窗口 caption 按钮”方式绘制背景和 glyph：

- `0`：最小化
- `1`：最大化
- `2`：还原
- `3`：关闭（hover 会用更明显的红色）

该属性主要由 `FluentMainWindow` 内部使用；一般业务代码无需设置。

关键 API：

- `setCheckable(bool)` / `setChecked(bool)`：使用 Qt 原生 API。
- `hoverLevel` / `pressLevel`（Q_PROPERTY）：动效层（内部动画驱动）。

Demo：Buttons / Overview（也用于若干容器控件内部）。

---

## FluentToggleSwitch

用途：Win11 风格开关（独立自绘 `QWidget`），包含 track + knob 动效、hover 行高亮与 focus ring。

继承与构造：

- `class FluentToggleSwitch : public QWidget`
- 构造：`FluentToggleSwitch(QWidget*)`、`FluentToggleSwitch(const QString&, QWidget*)`

外观/交互要点：

- Track 尺寸固定为约 `40x20`，knob 为 Islands 风格圆点（无阴影，偏性能优先）。
- `setText()` 可显示右侧标签（会自动省略）。
- `setChecked()` 会触发平滑的 progress 动画并发出 `toggled(bool)`。

关键 API：

- `setChecked(bool)` / `isChecked()`：开关状态。
- `toggled(bool)`：状态变化信号。
- `setText(const QString&)`：显示标签。
- `progress` / `hoverLevel` / `focusLevel`（Q_PROPERTY）：动画/交互层（内部 `QPropertyAnimation` 驱动）。

示例：

```cpp
#include "Fluent/FluentToggleSwitch.h"

auto *sw = new Fluent::FluentToggleSwitch(QStringLiteral("自动保存"));
sw->setChecked(true);
connect(sw, &Fluent::FluentToggleSwitch::toggled, this, [](bool on) {
	qDebug() << "AutoSave:" << on;
});
```

注意事项：

- 该控件是自绘 `QWidget`，不是 `QAbstractButton`；如果你需要 button group/autoExclusive 等能力，建议使用 `FluentRadioButton` 或自行封装。

Demo：Buttons / Containers / Windows / Overview。

---

## FluentCheckBox

用途：复选框（`QCheckBox`），包含 hover 行高亮、focus ring，以及勾选动画（checkmark 渐入）。

继承与构造：

- `class FluentCheckBox : public QCheckBox`
- 构造：`FluentCheckBox(QWidget*)`、`FluentCheckBox(const QString&, QWidget*)`

关键 API：

- `setChecked(bool)` / `isChecked()`：使用 Qt 原生 API。
- `stateChanged(int)`：原生信号（`Qt::Unchecked/Checked/PartiallyChecked`）；控件内部也用它驱动 check 动画。
- `hoverLevel` / `focusLevel`（Q_PROPERTY）：动效层（内部动画驱动）。

尺寸策略：重载了 `sizeHint()`/`minimumSizeHint()`，会基于文本宽度给出更贴近 Win11 的占位。

Demo：Buttons / Overview。

---

## FluentRadioButton

用途：单选按钮（`QRadioButton`），包含 hover 行高亮、focus ring，以及选中动画（dot 缩放 + accent border 渐入）。

继承与构造：

- `class FluentRadioButton : public QRadioButton`
- 构造：`FluentRadioButton(QWidget*)`、`FluentRadioButton(const QString&, QWidget*)`

关键 API：

- `setChecked(bool)` / `isChecked()`：使用 Qt 原生 API。
- `toggled(bool)`：原生信号；控件内部用它驱动选中动画。
- `hoverLevel` / `focusLevel`（Q_PROPERTY）：动效层（内部动画驱动）。

尺寸策略：重载了 `sizeHint()`/`minimumSizeHint()`，会基于文本宽度给出更贴近 Win11 的占位。

Demo：Buttons / Overview。
