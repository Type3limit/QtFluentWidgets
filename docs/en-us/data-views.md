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

Inheritance:

- `QListView` → `Fluent::FluentListView`

Defaults & styling (differences from Qt defaults):

- Enables mouse tracking (`setMouseTracking(true)` and `viewport()->setMouseTracking(true)`) for hover.
- Removes the frame (`setFrameShape(QFrame::NoFrame)`).
- `setUniformItemSizes(true)` to match "settings list" style.
- Pixel scrolling (`setVerticalScrollMode(QAbstractItemView::ScrollPerPixel)`).
- Installs `FluentListItemDelegate` by default:
	- removes `State_HasFocus` to avoid the dotted focus rectangle
	- paints a light rounded hover/selection background (radius ~4) using theme colors
- Replaces scrollbars with `FluentScrollBar`.

Hover animation:

- `mouseMoveEvent()` updates `hoverIndex()` using `indexAt(pos)`.
- `hoverLevel()` is driven by an internal `QVariantAnimation` (~120ms). While animating, it continuously updates the viewport.
- `leaveEvent()` clears the hover index and animates hover back to 0.

Selection transition (Current Index):

`FluentListView` paints an animated "current item" background in `paintEvent()` before calling the base paint, keeping text/icons crisp.

- Trigger: `selectionModel()->currentChanged(current, previous)`.
- Animation: `QVariantAnimation` (~180ms, `InOutCubic`) interpolates the background rect and opacity.
- Accent hint: a light accent fill (alpha ~40) plus a left accent indicator bar (~3px width, ~16px height).
- Multi-selection / non-current selected items: painted by the delegate using a lighter selection color (but avoids the current row to prevent double painting).

Theme coupling:

- On `ThemeManager::themeChanged` and Enabled changes, it refreshes `styleSheet` via `Theme::listViewStyle(colors)`.

Key APIs:

- `hoverIndex()`
- `hoverLevel()`
- Overridden `setModel()` hooks the selection model for animations.

Demo: DataViews / Overview.

---

## Table view

Purpose: table view with Fluent hover/selection painting and a Win11-like current-row transition.

Inheritance:

- `QTableView` → `Fluent::FluentTableView`

Defaults & styling:

- Uses a custom horizontal header (`FluentHeaderView`) that draws column separators after Qt finishes header painting.
- `horizontalHeader()->setStretchLastSection(true)`, `verticalHeader()->setVisible(false)`.
- `setShowGrid(false)`, `setFrameShape(QFrame::NoFrame)`, disables alternating row colors.
- Default selection behavior is `SelectRows`.
- Enables pixel scrolling and mouse tracking.
- Installs `FluentTableItemDelegate`:
	- removes dotted focus
	- row selection becomes a single continuous rounded shape across columns (rounded at first/last visible column)
- Replaces scrollbars with `FluentScrollBar`.

Header separator painting:

The custom header draws separators in `viewportEvent(Paint)`:

- Color: derived from `colors.border` with an alpha tint.
- Insets: leaves a small top/bottom inset (doesn't draw to the very edges).
- Safety: skips drawing when the window isn't exposed yet (`isExposed()` guard) to avoid early paint-engine warnings on some platforms.

Hover & selection transition:

- Hover uses `hoverIndex()` + `hoverLevel()` (~120ms).
- Selection transition listens to `selectionModel()->currentChanged` and animates a background rect over ~180ms.
- Under `SelectRows`, the animation target rect is computed by union-ing `visualRect()` across visible columns for the row, then applying a small inset (`adjusted(2,1,-2,-1)`).

Theme coupling:

- Refreshes `styleSheet` via `Theme::tableViewStyle(colors)` on theme/enabled changes.

Key APIs:

- `hoverIndex()` / `hoverLevel()`
- Overridden `setModel()` hooks the selection model for animations.

Demo: DataViews / Overview.

---

## Tree view

Purpose: tree view with Fluent hover/selection painting and transition animations, plus custom branch chevrons.

Inheritance:

- `QTreeView` → `Fluent::FluentTreeView`

Defaults & styling:

- Uses the same custom header separator strategy as `FluentTableView`.
- `setFrameShape(QFrame::NoFrame)`, disables alternating row colors, default `SelectRows`.
- `setIndentation(18)` for a Win11-like density.
- Enables pixel scrolling and mouse tracking.
- Installs `FluentTreeItemDelegate`:
	- removes dotted focus
	- hover/selection backgrounds form a continuous row shape (rounded at first/last column)
- Replaces scrollbars with `FluentScrollBar`.

Branch chevrons:

Overrides `drawBranches()` and paints a chevron for items that have children and are in column 0:

- Collapsed: right-pointing chevron
- Expanded: down-pointing chevron
- Color: `colors.subText`, stroke width ~1.5 with round caps/joins

Hover & selection transition:

- Hover uses `hoverIndex()` + `hoverLevel()` (~120ms). Under `SelectRows`, it treats "same row + same parent" as a line.
- Selection transition matches `FluentTableView` (row rect union across visible columns).

Theme coupling:

- Refreshes `styleSheet` via `Theme::treeViewStyle(colors)` on theme/enabled changes.

Key APIs:

- `hoverIndex()` / `hoverLevel()`
- Overridden `setModel()` hooks the selection model for animations.
- Overridden `drawBranches()` for Fluent branch visuals.

Demo: DataViews / Overview.
