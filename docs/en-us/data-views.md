# Data Views

## Widgets

- `FluentListView` (include: `Fluent/FluentListView.h`)
- `FluentTableView` (include: `Fluent/FluentTableView.h`)
- `FluentTreeView` (include: `Fluent/FluentTreeView.h`)

These views customize hover/selection rendering and include Win11-like selection transition animations.

Demo pages: DataViews (`demo/pages/PageDataViews.cpp`) and Overview (`demo/pages/PageOverview.cpp`).

## List view

```cpp
#include "Fluent/FluentListView.h"

auto *view = new Fluent::FluentListView();
view->setSelectionMode(QAbstractItemView::SingleSelection);
```

Purpose: list view with Fluent hover/selection painting and selection transition animation.

Key APIs:

- `hoverIndex()`
- `hoverLevel()`
- Overridden `setModel()` hooks the selection model for animations.

Demo: DataViews / Overview.

---

## Table view

Purpose: table view with Fluent hover/selection painting and selection transition animation.

Key APIs:

- `hoverIndex()` / `hoverLevel()`
- Overridden `setModel()` hooks the selection model for animations.

Demo: DataViews / Overview.

---

## Tree view

Purpose: tree view with Fluent hover/selection painting and selection transition animation, plus custom branch painting.

Key APIs:

- `hoverIndex()` / `hoverLevel()`
- Overridden `setModel()` hooks the selection model for animations.
- Overridden `drawBranches()` for Fluent branch visuals.

Demo: DataViews / Overview.
