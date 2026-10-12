# Data Views

## Widgets

- `FluentListView` (include: `Fluent/FluentListView.h`)
- `FluentTableView` (include: `Fluent/FluentTableView.h`)
- `FluentTableWidget` (include: `Fluent/FluentTableWidget.h`)
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
- `hoverLevel()` is driven by an internal `QVariantAnimation` configured with `FluentMotionRole::Hover`. While animating, it continuously updates the viewport; when animations are globally disabled or the hover duration is 0, hover jumps directly to the target state.
- `leaveEvent()` clears the hover index and animates hover back to 0.

Selection transition (Current Index):

`FluentListView` paints an animated "current item" background in `paintEvent()` before calling the base paint, keeping text/icons crisp.

- Trigger: `selectionModel()->currentChanged(current, previous)`.
- Animation: `QVariantAnimation` uses `FluentMotionRole::Selection` to interpolate the background rect and opacity. Disabling global animations or setting the Selection duration to 0 snaps directly to the target state.
- Accent hint: a light accent fill (alpha ~40) plus a left accent indicator bar (~3px width, ~16px height).
- Multi-selection / non-current selected items: painted by the delegate using a lighter selection color (but avoids the current row to prevent double painting).
- Disabled views keep the selected/current row position visible with a neutral disabled selection fill, but do not draw the accent indicator; labels continue to use `disabledText`.

Theme coupling:

- On `ThemeManager::themeChanged` and Enabled changes, it refreshes `styleSheet` via `Theme::listViewStyle(colors)`.
- Focus, disabled background/text/border, and the normal border are derived from Theme tokens; the fallback stylesheet also resolves through `neutral.card` / `strokeSubtle` / `accent.base`, so dark mode no longer falls back to native Qt light colors.

Key APIs:

- `hoverIndex()`
- `hoverLevel()`
- Overridden `setModel()` / `setSelectionModel()` hooks the current selection model for animations, so replacing the `QItemSelectionModel` still preserves the current-row transition.

Demo: DataViews / Overview.

---

## Table view

`FluentTableView` and `FluentTableWidget` keep the rounded focus border visible when headers and scroll bars are shown. Unfocused and disabled borders retain their neutral colors; painting follows the widget's current border style.

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

- Color: derived from `ThemeManager::tokens().neutral.strokeSubtle`, matching the view palette's `Mid` role.
- Insets: leaves a small top/bottom inset (doesn't draw to the very edges).
- Safety: skips drawing when the window isn't exposed yet (`isExposed()` guard) to avoid early paint-engine warnings on some platforms.

Hover & selection transition:

- Hover uses `hoverIndex()` + `hoverLevel()`, configured with `FluentMotionRole::Hover`, and jumps directly to the final state when animations are disabled.
- Selection transition listens to `selectionModel()->currentChanged` and animates the background rect and opacity using `FluentMotionRole::Selection`; disabling global animations or setting the Selection duration to 0 snaps directly to the target state.
- Under `SelectRows`, the animation target rect is computed by union-ing `visualRect()` across visible columns for the row, then applying a small inset (`adjusted(2,1,-2,-1)`).
- Disabled views do not keep the enabled accent fill or left indicator for selected/current rows; they use a neutral disabled selection fill instead. `FluentTableWidget` shares the same semantics.

Theme coupling:

- Refreshes `styleSheet` via `Theme::tableViewStyle(colors)` on theme/enabled changes.
- Focus, disabled state, view palette roles, fallback stylesheet, header background, and separator colors all come from the same neutral/accent tokens, avoiding native light remnants in dark mode.

Key APIs:

- `hoverIndex()` / `hoverLevel()`
- Overridden `setModel()` / `setSelectionModel()` hooks the selection model for animations.

Demo: DataViews / Overview.

---

## Table widget

```cpp
#include "Fluent/FluentTableWidget.h"
#include <QTableWidgetItem>

auto *table = new Fluent::FluentTableWidget(5, 4);
table->setHorizontalHeaderLabels({"Name", "Enabled", "Qty", "Level"});
table->setItem(0, 0, new QTableWidgetItem("Row 1"));
table->setCellWidget(0, 1, new Fluent::FluentCheckBox());
table->setCellWidget(0, 2, new Fluent::FluentSpinBox());
table->setCellWidget(0, 3, new Fluent::FluentComboBox());
```

Purpose: item-based table (`QTableWidget` style) sharing all visuals/interactions with `FluentTableView`, but carrying its own internal model so cells can be populated through `setItem()` / `setCellWidget()` — ideal when you need real widgets inside cells.

### Inheritance

- `QTableWidget` → `Fluent::FluentTableWidget`

### When to use

- **Small datasets with in-row controls** — property sheets, editor panes, form-style grids. Each `QTableWidgetItem` is a real object, so reading/writing per-cell data is straightforward. `setCellWidget(r, c, widget)` embeds any widget (`FluentCheckBox`, `FluentComboBox`, `FluentSpinBox`, `FluentButton`, `FluentToggleSwitch`, …).
- **Large datasets** — use `FluentTableView` + a custom `QStyledItemDelegate` instead. Because every `QTableWidget` cell is an object, memory and refresh costs grow quickly past a few thousand rows.

### Defaults

- Reuses the same `FluentHeaderView` + `FluentTableItemDelegate` as `FluentTableView`: identical row hover/selection rendering, column separators, Fluent scroll bars, and `FluentMotionRole::Selection` transition semantics.
- Two constructors: `FluentTableWidget(QWidget*)` and `FluentTableWidget(int rows, int cols, QWidget*)`.

Key APIs:

- Inherits `QTableWidget`: `setItem()` / `item()` / `setCellWidget()` / `cellWidget()` / `setHorizontalHeaderLabels()` / etc.
- `hoverIndex()` / `hoverLevel()`.
- Overridden `setSelectionModel()` keeps the selection transition connected after external selection-model replacement.

Demo: DataViews (the "FluentTableWidget" section).

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

- Hover uses `hoverIndex()` + `hoverLevel()`, configured with `FluentMotionRole::Hover`, and jumps directly to the final state when animations are disabled. Under `SelectRows`, it treats "same row + same parent" as a line.
- Selection transition matches `FluentTableView` (row rect union across visible columns); disabled selected/current rows use the neutral disabled selection fill and omit the accent indicator.

Theme coupling:

- Refreshes `styleSheet` via `Theme::treeViewStyle(colors)` on theme/enabled changes.

Key APIs:

- `hoverIndex()` / `hoverLevel()`
- Overridden `setModel()` / `setSelectionModel()` hooks the selection model for animations.
- Overridden `drawBranches()` for Fluent branch visuals.

Demo: DataViews / Overview.
