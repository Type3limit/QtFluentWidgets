# 数据视图

## 控件清单

- `FluentListView`（include: `Fluent/FluentListView.h`）
- `FluentTableView`（include: `Fluent/FluentTableView.h`）
- `FluentTreeView`（include: `Fluent/FluentTreeView.h`）

这些控件在 selection/hover 等交互上使用 Fluent 风格，并包含选中切换动效（更接近 Win11）。

Demo 页面：DataViews（`demo/pages/PageDataViews.cpp`）与 Overview（`demo/pages/PageOverview.cpp`）。

## FluentListView

```cpp
#include "Fluent/FluentListView.h"

auto *view = new Fluent::FluentListView();
view->setSelectionMode(QAbstractItemView::SingleSelection);
```

用途：列表视图，提供 hover/selection 自绘与选中切换动效。

### 继承关系

- `QListView` → `Fluent::FluentListView`

### 默认行为与样式

构造后控件会进行如下配置（与 Qt 默认行为不同的点）：

- 开启鼠标追踪：`setMouseTracking(true)` + `viewport()->setMouseTracking(true)`，用于 hover。
- 去掉外框：`setFrameShape(QFrame::NoFrame)`。
- `setUniformItemSizes(true)`：更偏向“设置页列表项”的使用场景。
- 像素级滚动：`setVerticalScrollMode(QAbstractItemView::ScrollPerPixel)`。
- 默认安装 `FluentListItemDelegate`：
	- 取消 `State_HasFocus`（去掉虚线焦点框）。
	- 使用主题色绘制轻量 hover/selection 背景（圆角 4）。
- 默认把水平/垂直滚动条替换为 `FluentScrollBar`。

### Hover 动效

- `mouseMoveEvent()` 中用 `indexAt(pos)` 更新 `hoverIndex()`。
- `hoverLevel()` 由内部 `QVariantAnimation` 驱动（120ms），动画过程中持续 `viewport()->update()`。
- `leaveEvent()` 会清空 `hoverIndex()` 并将 hover 动效退回到 0。

### 选中切换动效（Current Index）

`FluentListView` 会在 `paintEvent()` 里先绘制“当前项”的选中背景（再调用基类绘制文本/图标，保证文本清晰）。

- 动画触发：监听 `selectionModel()->currentChanged(current, previous)`。
- 动画形式：
	- `QVariantAnimation`（180ms，`InOutCubic`）插值 `QRectF` 与透明度。
	- 背景为 accent（alpha 40 的轻量填充）。
	- 左侧还有一条 3px 宽的 accent 指示条（高度约 16px）。
- 多选/非 current 的选中项：在 delegate 中用“轻量选中底色”绘制（但会避开 current 行，避免和动画层重复）。

### 主题联动

- 主题变化（`ThemeManager::themeChanged`）或 enabled 状态变化时，会用 `Theme::listViewStyle(colors)` 刷新 `styleSheet`。

关键 API：

- `hoverIndex()`：当前 hover 的 index。
- `hoverLevel()`：hover 动效强度（内部驱动）。
- 重载 `setModel()`：会自动 hook selection model（用于选中动画）。

注意事项：

- `hookSelectionModel()` 只在 `setModel()` / 构造后延迟一次时机调用；如果你在外部替换了 selection model（`setSelectionModel()`），需要自行确保 currentChanged 能被接入（最简单是重新 `setModel(model())` 或在控件侧补一个公共方法）。

Demo：DataViews / Overview。

## FluentTableView

```cpp
#include "Fluent/FluentTableView.h"

auto *table = new Fluent::FluentTableView();
table->setSelectionBehavior(QAbstractItemView::SelectRows);
```

用途：表格视图，提供 hover/selection 自绘与选中切换动效。

### 继承关系

- `QTableView` → `Fluent::FluentTableView`

### 默认行为与样式

- 自定义水平表头 `FluentHeaderView`：在表头绘制完成后补画列分隔线（更接近 Win11）。
- `horizontalHeader()->setStretchLastSection(true)`，`verticalHeader()->setVisible(false)`。
- `setShowGrid(false)`、`setFrameShape(QFrame::NoFrame)`、关闭交替行色。
- 默认 `setSelectionBehavior(QAbstractItemView::SelectRows)`。
- 像素级滚动 + 开启鼠标追踪。
- 默认安装 `FluentTableItemDelegate`：
	- 去掉虚线焦点框。
	- 行选中时把背景连成一个整体：首列/末列做圆角过渡，中间列保持直边。
- 默认滚动条替换为 `FluentScrollBar`。

### Header 分隔线绘制

水平表头会在 `viewportEvent(Paint)` 中额外绘制分隔线：

- 颜色来自 `colors.border`（alpha ≈ 140）。
- 上下各留 `inset=6` 的空白，不画到边缘。
- 为避免启动时的 Qt 警告（`Paint device returned engine == 0`），在窗口尚未 `isExposed()` 时跳过这次绘制。

### Hover/选中动效

与 `FluentListView` 一致的思路：

- hover：`hoverIndex()` + `hoverLevel()`（120ms）。
- 选中切换：监听 `selectionModel()->currentChanged`，用 180ms 动画插值选中背景矩形与透明度。
- `SelectRows` 下，动画矩形会把同一行多个 cell 的 `visualRect` 做 union（跳过隐藏列），再做轻微内缩（`adjusted(2,1,-2,-1)`）。

### 主题联动

- 主题变化/EnabledChange 时，会用 `Theme::tableViewStyle(colors)` 刷新 `styleSheet`。

关键 API：

- `hoverIndex()` / `hoverLevel()`
- 重载 `setModel()`：自动接入选中动画。

Demo：DataViews / Overview。

## FluentTreeView

```cpp
#include "Fluent/FluentTreeView.h"

auto *tree = new Fluent::FluentTreeView();
tree->setHeaderHidden(false);
```

用途：树视图，提供 hover/selection 自绘与选中切换动效，并自绘分支线/展开区域风格。

### 继承关系

- `QTreeView` → `Fluent::FluentTreeView`

### 默认行为与样式

- 自定义水平表头绘制分隔线（逻辑与 `FluentTableView` 相同）。
- `setFrameShape(QFrame::NoFrame)`、关闭交替行色、默认 `SelectRows`。
- `setIndentation(18)`：缩进偏向 Win11 树的视觉密度。
- 像素级滚动 + 开启鼠标追踪。
- 默认安装 `FluentTreeItemDelegate`：
	- 去掉虚线焦点框。
	- 行 hover/selection 背景连成整体（首列/末列圆角）。
- 默认滚动条替换为 `FluentScrollBar`。

### 分支（展开/折叠）chevron

重载 `drawBranches()`，对“有子节点且位于第 0 列”的项绘制一个 chevron：

- 折叠时绘制向右的 `>` 形。
- 展开时绘制向下的 `∨` 形。
- 颜色使用 `colors.subText`，线宽约 1.5，并采用圆角 cap/join。

### Hover/选中动效

- hover：同样通过 `hoverIndex()` + `hoverLevel()`（120ms），在 `SelectRows` 下以“同 row + 同 parent”为一行。
- 选中切换：与 TableView 相同，`SelectRows` 下会 union 一整行的可视矩形作为动画目标。

### 主题联动

- 主题变化/EnabledChange 时，会用 `Theme::treeViewStyle(colors)` 刷新 `styleSheet`。

关键 API：

- `hoverIndex()` / `hoverLevel()`
- 重载 `setModel()`：自动接入选中动画。
- 重载 `drawBranches()`：用于 Fluent 风格分支绘制。

Demo：DataViews / Overview。
