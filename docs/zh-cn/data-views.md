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

关键 API：

- `hoverIndex()`：当前 hover 的 index。
- `hoverLevel()`：hover 动效强度（内部驱动）。
- 重载 `setModel()`：会自动 hook selection model（用于选中动画）。

Demo：DataViews / Overview。

## FluentTableView

```cpp
#include "Fluent/FluentTableView.h"

auto *table = new Fluent::FluentTableView();
table->setSelectionBehavior(QAbstractItemView::SelectRows);
```

用途：表格视图，提供 hover/selection 自绘与选中切换动效。

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

关键 API：

- `hoverIndex()` / `hoverLevel()`
- 重载 `setModel()`：自动接入选中动画。
- 重载 `drawBranches()`：用于 Fluent 风格分支绘制。

Demo：DataViews / Overview。
