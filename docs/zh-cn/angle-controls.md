# 角度控件

## 控件清单

- `FluentDial`（include: `Fluent/FluentDial.h`）
- `FluentAngleSelector`（include: `Fluent/FluentAngleSelector.h`）

Demo 页面：Angle Controls（`demo/pages/PageAngleControls.cpp`）与 Overview（部分场景会复用）。

## FluentDial

```cpp
#include "Fluent/FluentDial.h"

auto *dial = new Fluent::FluentDial();
dial->setValue(135);
dial->setTicksVisible(true);
dial->setMajorTickStep(45);
```

用途：紧凑的圆形角度旋钮，适合渐变角度、图形旋转、方向选择等场景。

语义约定：

- `0°` = 从左到右（east / 3 点钟方向起算）
- 角度按**顺时针**递增
- 填充弧从 3 点钟方向开始，表示当前角度进度

关键 API：

- `setValue(int)` / `value()`：当前角度值
- `setTicksVisible(bool)` / `ticksVisible()`：是否显示刻度
- `setTickStep(int)` / `tickStep()`：普通刻度步进（默认 15°）
- `setMajorTickStep(int)` / `majorTickStep()`：主刻度步进（默认 45°）
- `setPointerVisible(bool)` / `pointerVisible()`：是否显示指针

实现语义：

- 这是自绘 `QWidget`，包含 hover/focus 动画层（内部 `QVariantAnimation` 驱动）。
- 鼠标拖拽会根据当前位置换算角度；滚轮可做细粒度调节。
- 视觉上会同时绘制：
  - 外圈轨道
  - 当前角度 accent 弧
  - 可选刻度 / 主刻度
  - 可选指针
  - focus ring（聚焦时）
- `sizeHint()` / `minimumSizeHint()` 已为“紧凑旋钮”场景做过约束，通常直接放进水平布局即可。

适用场景：

- 颜色渐变角度编辑
- 旋转角度输入
- 朝向 / 方位选择

---

## FluentAngleSelector

```cpp
#include "Fluent/FluentAngleSelector.h"

auto *editor = new Fluent::FluentAngleSelector();
editor->setValue(128);
editor->setLabelText(QStringLiteral("角度："));
editor->setSuffix(QStringLiteral("°"));
```

用途：由“标签 + Dial + SpinBox”组成的一体化角度编辑器，对外只暴露单一的 `valueChanged(int)` 语义。

关键 API：

- `setValue(int)` / `value()`
- `setRange(int minimum, int maximum)`：范围默认是 `0..359`
- `setWrapping(bool)` / `wrapping()`：是否循环（默认 true）
- `setLabelText(const QString&)` / `labelText()`
- `setSuffix(const QString&)` / `suffix()`
- `setDialVisible(bool)` / `dialVisible()`
- `setLabelVisible(bool)` / `labelVisible()`
- `setSpinBoxVisible(bool)` / `spinBoxVisible()`
- `dial()` / `spinBox()`：取到底层子控件（便于做更细粒度定制）

实现语义：

- 内部使用 `FluentDial + FluentSpinBox + FluentLabel` 组合实现。
- Dial 与 SpinBox 双向同步，内部通过 `blockSignals(true)` 避免回环触发。
- wrapping 模式下，值会通过 `normalizeToRange()` 规整到给定范围内；例如默认范围下 `-1 -> 359`、`360 -> 0`。
- 非 wrapping 模式下会做 `qBound(minimum, value, maximum)` 钳制。
- 可见性开关适合做变体布局：
  - 隐藏 label：适合紧凑表单
  - 隐藏 dial：退化成“带统一语义的角度 SpinBox”
  - 隐藏 spinbox：只保留旋钮交互

典型场景：

- `FluentColorDialog` 的渐变角度编辑
- 图形 / 画布对象旋转
- 任何“角度值 + 可视化旋钮”组合输入

示例：隐藏标签，仅保留 Dial + SpinBox

```cpp
auto *editor = new Fluent::FluentAngleSelector();
editor->setLabelVisible(false);
editor->setValue(210);
```

