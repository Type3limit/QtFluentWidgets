# 容器 / 布局

## 控件清单

- `FluentCard`（include: `Fluent/FluentCard.h`）
- `FluentGroupBox`（include: `Fluent/FluentGroupBox.h`）
- `FluentTabWidget`（include: `Fluent/FluentTabWidget.h`）
- `FluentScrollArea`（include: `Fluent/FluentScrollArea.h`）
- `FluentScrollBar`（include: `Fluent/FluentScrollBar.h`）
- `FluentFlowLayout`（include: `Fluent/FluentFlowLayout.h`）
- `FluentSplitter`（include: `Fluent/FluentSplitter.h`）
- `FluentWidget`（include: `Fluent/FluentWidget.h`）
- `FluentLabel`（include: `Fluent/FluentLabel.h`）

Demo 页面：Containers（`demo/pages/PageContainers.cpp`）与 Overview（`demo/pages/PageOverview.cpp`）。

## FluentCard（卡片容器）

支持可折叠卡片（Demo 中大量使用）：

```cpp
#include "Fluent/FluentCard.h"

auto *card = new Fluent::FluentCard();
card->setTitle(QStringLiteral("标题"));
card->setCollapsible(true);
card->setCollapsed(false);

auto *body = card->contentLayout();
// body->addWidget(...)
```

用途：内容分组容器（可选折叠），Demo 中多数示例卡片都基于此控件。

关键 API：

- `setTitle(const QString&)` / `title()`
- `setCollapsible(bool)` / `isCollapsible()`
- `setCollapsed(bool)` / `isCollapsed()` + `collapsedChanged(bool)`
- `setCollapseAnimationEnabled(bool)`：折叠动画。
- `contentWidget()` / `contentLayout()`：添加内容。

Demo：几乎所有页面（Buttons/Inputs/Pickers/DataViews/Containers/Windows/Overview）。

## FluentFlowLayout（自适应换行布局）

```cpp
#include "Fluent/FluentFlowLayout.h"

auto *host = new QWidget();
auto *flow = new Fluent::FluentFlowLayout(host, 0, 12, 12);
flow->setUniformItemWidthEnabled(true);
flow->setMinimumItemWidth(320);
host->setLayout(flow);

flow->addWidget(new QWidget());
```

用途：自适应换行布局（可选 uniform item width、列数迟滞、几何动画与节流）。

关键 API：

- `setUniformItemWidthEnabled(bool)` / `uniformItemWidthEnabled()`
- `setMinimumItemWidth(int)` / `minimumItemWidth()`
- `setColumnHysteresis(int)`：避免窗口缩放时列数抖动。
- 动画：`setAnimationEnabled(bool)`、`setAnimationDuration(int)`、`setAnimationEasing(...)`、`setAnimationThrottle(int)`、`setAnimateWhileResizing(bool)`。

Demo：Containers / Overview。

## FluentSplitter

```cpp
#include "Fluent/FluentSplitter.h"

auto *sp = new Fluent::FluentSplitter(Qt::Horizontal);
sp->addWidget(new QWidget());
sp->addWidget(new QWidget());
```

用途：Fluent 风格分割条（自绘 handle）。

关键 API：

- 继承自 `QSplitter`：使用 Qt 原生 API `addWidget()` / `setSizes()` / `setStretchFactor()`。

Demo：Containers / Overview。

---

## FluentScrollArea / FluentScrollBar

用途：滚动容器 + Fluent 风格滚动条，支持 overlay 模式（靠近 Win11）。

关键 API（FluentScrollArea）：

- `contentWidget()` / `contentLayout()` / `setContentLayout(QLayout*)`：便捷管理内容区。
- `setOverlayScrollBarsEnabled(bool)` / `overlayScrollBarsEnabled()`
- `setScrollBarsRevealed(bool)`：强制显示/隐藏 overlay 滚动条。

关键 API（FluentScrollBar）：

- `setOverlayMode(bool)` / `overlayMode()`
- `setForceVisible(bool)` / `forceVisible()`
- `revealLevel` / `hoverLevel`（Q_PROPERTY）：动效层。

Demo：Inputs（展示滚动条）/ Overview（也用于多个页面的滚动容器）。

---

## FluentTabWidget

用途：Win11 Settings 风格的 TabWidget（包含切换动效与 frame overlay）。

关键 API：

- 继承自 `QTabWidget`：使用 Qt 原生 API `addTab()` / `setTabPosition()` / `setCurrentIndex()`。

Demo：Containers / Overview。

---

## FluentGroupBox

用途：分组框（Fluent 风格标题与边框）。

关键 API：

- 继承自 `QGroupBox`：使用 Qt 原生 API `setTitle()` / `setCheckable()` 等。

Demo：Containers / Overview。

---

## FluentWidget

用途：基础容器控件（用于统一背景角色：WindowBackground / Surface / Transparent）。

关键 API：

- `setBackgroundRole(BackgroundRole)` / `backgroundRole()`

Demo：Containers / Windows / Overview（也常作为内部内容 host）。

---

## FluentLabel

用途：文本标签（自动跟随主题更新样式）。

关键 API：

- 继承自 `QLabel`：使用 Qt 原生 API `setText()` / `setWordWrap()`。

Demo：DataViews / Pickers / Containers / Windows / Overview。

## FluentTabWidget

```cpp
#include "Fluent/FluentTabWidget.h"

auto *tabs = new Fluent::FluentTabWidget();
tabs->setTabPosition(QTabWidget::West);
tabs->addTab(new QWidget(), QStringLiteral("页 1"));
```
