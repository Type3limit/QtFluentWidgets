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

### 结构与折叠机制（实现语义）

当你启用 `setCollapsible(true)` 后，`FluentCard` 会确保内部具有如下结构：

- 根布局：如果你没有给 card 设置布局，会自动创建 `QVBoxLayout`，并设置 `ContentsMargins(16,14,16,14)`、`spacing=10`。
- Header：`FluentCardHeader`（一个 `QWidget`），包含：
	- 标题 `FluentLabel`（objectName: `FluentCardTitle`）
	- 右侧 chevron `FluentToolButton`（objectName: `FluentCardChevron`，固定 28×28）
	- header 自带 `PointingHandCursor`，并通过 `eventFilter` 监听鼠标左键按下以触发折叠/展开。
- Content：`FluentCardContent`（一个 `QWidget`），内部 `QVBoxLayout` 采用 `spacing=8`。

如果你在调用 `setCollapsible(true)` 之前已经往 card 的布局里塞了控件/子布局，控件会被“搬家”到 `FluentCardContent` 里（因此你不需要为了折叠特性改写大量布局代码）。

折叠动画：

- 通过 `QPropertyAnimation` 动画 `contentWidget()->maximumHeight`（160ms，`OutCubic`）。
- 折叠完成后会把内容区 `setVisible(false)` 并强制 `maximumHeight=0`，展开则恢复可见并把最大高度放开到 `QWIDGETSIZE_MAX`。

关键 API：

- `setTitle(const QString&)` / `title()`
- `setCollapsible(bool)` / `isCollapsible()`
- `setCollapsed(bool)` / `isCollapsed()` + `collapsedChanged(bool)`
- `setCollapseAnimationEnabled(bool)`：折叠动画。
- `contentWidget()` / `contentLayout()`：添加内容。

注意事项：

- 折叠的触发区域包含 header 整行（不仅是 chevron 按钮），这是通过 header 上的事件过滤实现的。
- 折叠动画本质是“最大高度”动画：如果你的内容内部有依赖 `maximumHeight` 的复杂尺寸策略（或你手动改了 content 的 maximumHeight），需要注意两者叠加效果。

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

### 实现语义（布局算法）

- `hasHeightForWidth() == true`：整体高度由 `heightForWidth(width)` 推导。
- 换行判定使用“右边界 exclusive”的逻辑（`rightEdgeExclusive = x + availableW`），避免 `QRect::right()` 的 inclusive 语义导致“刚好能放下却被错误换行”的 off-by-one。
- 每个 item 的高度优先级：
	1) `item->hasHeightForWidth()` → `item->heightForWidth(itemW)`
	2) 否则若是 widget 且其内部 layout 支持 HFW → `layout->totalHeightForWidth(itemW)`
	3) 否则 → `sizeHint().height()`

### uniform item width / 列数迟滞

当 `setUniformItemWidthEnabled(true)` 时：

- 先根据可用宽度、`minimumItemWidth()` 与水平间距估算 `idealCols`。
- 然后通过 `columnHysteresis()` 做“列数迟滞”：只有当宽度跨过阈值并额外超过/低于 hysteresis 像素时，才允许列数 +1 或 -1，避免窗口缩放边缘抖动。
- 统一宽度：
	- `uniformW = (availableW - (cols-1)*spaceX) / cols`
	- full-row 项不参与 uniformW（见下）。

### 分组/换行属性（对 widget 设置 property）

该布局支持用属性快速实现“标题占一整行 + 下面是卡片网格”的结构：

- `fluentFlowFullRow=true`：该控件强制独占一行，宽度占满可用宽度。
- `fluentFlowBreakBefore=true`：在该控件前强制换行（另起一行）。
- `fluentFlowBreakAfter=true`：在该控件后强制换行。

示例：

```cpp
auto *title = new Fluent::FluentLabel(QStringLiteral("Group"));
title->setProperty(Fluent::FluentFlowLayout::kFullRowProperty, true);
flow->addWidget(title);

// 下面继续 add card/tile...
```

### 几何动画（实现语义）

启用 `setAnimationEnabled(true)` 后，布局会对每个子 widget 的 `geometry` 做 `QPropertyAnimation`：

- 动画参数：默认 140ms，`OutCubic`。
- 持久化动画对象：按 widget 缓存 `QPropertyAnimation`，减少 resize 时频繁 new/delete。
- 节流：当动画正在运行且目标频繁变化时，使用 `animationThrottle`（默认 50ms）限制“重启动画”的频率：
	- 若未到可重启时间，则只更新 `endValue`（平滑 steer 到新目标）。
	- 否则 stop → 用当前 geometry 作为 startValue 重启。

### resize 期间是否动画

- `animateWhileResizing=true`：每次 `setGeometry()` 都会直接动画到新布局。
- `animateWhileResizing=false`：为交互更顺滑，会“先立即应用布局（关闭父控件更新以减少闪烁）”，并把最后一次目标几何缓存起来；当 resize 停止一段时间（默认 debounce 90ms）后，才播放一次动画到最终位置。

### 避免与子控件高度动画打架

- 若某个子 widget 设置 `fluentFlowDisableAnimation=true`，该次 relayout 会跳过几何动画：
	- 会先 stop 所有 in-flight 动画，
	- 然后批量应用新 geometry（临时禁用父控件 updates）
	- 常用于卡片折叠等“子控件自己在动高度”的场景，避免闪烁/撕裂。

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

实现语义：

- 默认：`setChildrenCollapsible(false)`，并把 handle 宽度设为 8px（更好抓取）。
- `createHandle()` 返回自定义 `QSplitterHandle`：
	- `setMouseTracking(true)` 并设置对应 cursor（水平分割为 `SplitHCursor`，垂直为 `SplitVCursor`）。
	- hover 动效：`QVariantAnimation`（120ms，`OutCubic`）驱动 `hoverLevel`。
	- 绘制：
		- 永久在线（separator line）：颜色为 `colors.border`，alpha 从约 110 → 165 随 hoverLevel 增强，并上下/左右留 6px inset。
		- hover 时额外绘制一个小 pill（使用 `colors.hover` 的半透明填充）+ 中心三点 grip（使用 `colors.subText`），偏 Win11 风格。
- splitter 本身背景保持透明（styleSheet: `QSplitter { background: transparent; }`），视觉由 handle 自己承担。

关键 API：

- 继承自 `QSplitter`：使用 Qt 原生 API `addWidget()` / `setSizes()` / `setStretchFactor()`。

Demo：Containers / Overview。

---

## FluentScrollArea / FluentScrollBar

用途：滚动容器 + Fluent 风格滚动条，支持 overlay 模式（靠近 Win11）。

### FluentScrollArea：透明 viewport 与 overlay 滚动条

`FluentScrollArea` 默认使用 `FluentWidget` 作为 viewport，并尽量把背景变为透明，以避免某些平台样式把 viewport 填成固定的浅灰色（Windows 下常见）。

Overlay 滚动条的思路是“双滚动条”：

- 内部滚动条（真正驱动滚动）：安装在 `QScrollArea` 上，负责滚动范围与滚动行为。
- Overlay 滚动条（只负责显示与交互）：作为 `viewport()` 的子控件绘制在内容上方。
- 两者会互相同步 range/value/pageStep/singleStep；overlay 拖拽时会把 value 回写给内部滚动条（注意：这里不会 block 内部滚动条信号，因为 `QAbstractScrollArea` 依赖这些信号来滚动）。

Reveal/隐藏策略：

- `viewport()` 上安装 `eventFilter`：Enter/Wheel 时立即 reveal；Leave 后启动 700ms 定时器，超时自动隐藏。
- `setScrollBarsRevealed(bool)` 可强制显示/隐藏（同时会停止 hide timer）。

Overlay 几何：

- thickness 固定为 10px，margin 为 2px。
- 垂直/水平滚动条会根据“是否需要滚动”（`minimum() < maximum()`）来决定 show/hide。
- 如果两个方向都需要滚动，会为角落留出空间避免重叠。

关键 API（FluentScrollArea）：

- `contentWidget()` / `contentLayout()` / `setContentLayout(QLayout*)`：便捷管理内容区。
- `setOverlayScrollBarsEnabled(bool)` / `overlayScrollBarsEnabled()`
- `setScrollBarsRevealed(bool)`：强制显示/隐藏 overlay 滚动条。

示例：快速创建一个带 overlay 滚动条的内容区

```cpp
#include "Fluent/FluentScrollArea.h"

auto *area = new Fluent::FluentScrollArea();
area->setOverlayScrollBarsEnabled(true);

auto *layout = new QVBoxLayout();
layout->setContentsMargins(0, 0, 0, 0);
layout->setSpacing(8);
area->setContentLayout(layout);
// layout->addWidget(...)
```

关键 API（FluentScrollBar）：

- `setOverlayMode(bool)` / `overlayMode()`
- `setForceVisible(bool)` / `forceVisible()`
- `revealLevel` / `hoverLevel`（Q_PROPERTY）：动效层。

### FluentScrollBar：绘制与交互（实现语义）

- 厚度固定为 10px（竖向固定宽度/横向固定高度）。
- 自绘 thumb（圆角 pill），并刻意避免 `WA_TranslucentBackground`（一些平台/后端下子控件半透明可能出现黑条）。
- 颜色：thumb 的 base/hover/pressed 颜色会根据主题背景亮度自动选择“白系”或“黑系”并带不同 alpha。
- Overlay 模式：
	- `revealLevel` 控制 thumb 淡入淡出（低于约 0.01 时直接不画）。
	- 通过 `eventFilter` 监听 viewport 的 Enter/MouseMove/Wheel 等交互来触发 reveal，并在 Leave 后用 700ms 定时器自动隐藏。
- 非 overlay 模式：如果它是 `QAbstractScrollArea` 的“真实滚动条”，会把自己占用的保留区域填充成 viewport 背景色，避免出现一条不跟随主题的底色。

Demo：Inputs（展示滚动条）/ Overview（也用于多个页面的滚动容器）。

---

## FluentTabWidget

用途：Win11 Settings 风格的 TabWidget（包含切换动效、导航指示器动画与 frame overlay）。

### 实现语义（关键行为）

- `FluentTabWidget` 内部会把 `QTabBar` 替换成自定义的 `FluentTabBar`：
	- `setDocumentMode(true)`、`setDrawBase(false)`、`setExpanding(false)`、右侧省略（`ElideRight`）、启用滚动按钮（`setUsesScrollButtons(true)`）。
	- hover/press 由 tabbar 自己处理（mouse tracking + 记录 `hoverIndex/pressedIndex`），并自绘 hover/press 背景。
	- 选中指示器是一个 `QVariantAnimation`（约 240ms，`InOutCubic`）驱动的 `indicatorRect` 插值：
		- 横向 Tab：底部 3px accent underline（两侧 inset）。
		- 纵向（West/East）导航：左/右侧 3px accent 指示条（跟随选中项平滑移动），并在选中项上绘制轻量背景块（更接近 Win11 Settings）。

- frame overlay：控件会创建一个透明的 `FluentTabFrameOverlay` 盖在最上层（`WA_TransparentForMouseEvents`），用来：
	- 画圆角边框（1px）
	- 通过“OddEvenFill cut path”把圆角外的区域填成 background，从视觉上把子控件的直角裁掉（避免内容区把圆角“顶成方形”）。

- 滚动按钮（当 tab 太多时出现）：tabbar 会在 `show/layout` 后查找内部 `QToolButton`，把箭头替换成自绘 chevron icon（颜色跟随 `colors.text`），并统一到约 26×26 的点击目标。

关键 API：

- 继承自 `QTabWidget`：使用 Qt 原生 API `addTab()` / `setTabPosition()` / `setCurrentIndex()`。

注意事项：

- 指示器矩形（`tabRect()`）在 Qt 内部可能是 lazy layout；实现里会在拿不到有效 rect 时 `singleShot(0, ...)` 重试一次，因此在 startup/首次 show 时指示器可能是“先 snap 再动画”。

Demo：Containers / Overview。

```cpp
#include "Fluent/FluentTabWidget.h"

auto *tabs = new Fluent::FluentTabWidget();
tabs->setTabPosition(QTabWidget::West);
tabs->addTab(new QWidget(), QStringLiteral("页 1"));
```

---

## FluentGroupBox

用途：分组框（Fluent 风格标题与边框）。

实现语义：

- 设置 `WA_StyledBackground` 让样式表能控制背景。
- 默认 `setContentsMargins(12, 20, 12, 12)`（为标题留出顶部空间）。
- 主题联动：
	- 构造时调用 `Theme::groupBoxStyle(colors)` 设置 styleSheet。
	- `ThemeManager::themeChanged` 与 `EnabledChange` 时都会重新应用（但会做字符串比较避免重复 set）。

关键 API：

- 继承自 `QGroupBox`：使用 Qt 原生 API `setTitle()` / `setCheckable()` 等。

Demo：Containers / Overview。

---

## FluentWidget

用途：基础容器控件（用于统一背景角色：WindowBackground / Surface / Transparent）。

实现语义：

- 设置 `WA_StyledBackground`，但 `setAutoFillBackground(false)`，背景完全由 `paintEvent()` 自绘。
- `BackgroundRole`：
	- `Transparent`：`paintEvent()` 直接 return，不画任何像素（用于作为透明 host）。
	- `Surface`：填充 `colors.surface`。
	- `WindowBackground`：填充 `colors.background`。
- 主题变化/EnabledChange 会触发 `update()`，但不直接改 styleSheet（这是一个“纯绘制型”容器）。

关键 API：

- `setBackgroundRole(BackgroundRole)` / `backgroundRole()`

Demo：Containers / Windows / Overview（也常作为内部内容 host）。

---

## FluentLabel

用途：文本标签（自动跟随主题更新样式）。

实现语义：

- `FluentLabel` 不会覆盖你的自定义 styleSheet（例如 font-weight、padding 等）。它采用“追加一条颜色规则”的方式实现主题联动：
	- 在 styleSheet 尾部追加一个 marker：`/*FluentLabelTheme*/`。
	- 每次主题/EnabledChange 时，会先把 marker 之后的内容裁掉，再追加当前颜色规则：`color: ...;`。
- 颜色：enabled 用 `colors.text`，disabled 用 `colors.disabledText`。

关键 API：

- 继承自 `QLabel`：使用 Qt 原生 API `setText()` / `setWordWrap()`。

Demo：DataViews / Pickers / Containers / Windows / Overview。
