# Containers / Layout

## Widgets

- `FluentCard` (include: `Fluent/FluentCard.h`)
- `FluentGroupBox` (include: `Fluent/FluentGroupBox.h`)
- `FluentTabWidget` (include: `Fluent/FluentTabWidget.h`)
- `FluentScrollArea` (include: `Fluent/FluentScrollArea.h`)
- `FluentScrollBar` (include: `Fluent/FluentScrollBar.h`)
- `FluentFlowLayout` (include: `Fluent/FluentFlowLayout.h`)
- `FluentSplitter` (include: `Fluent/FluentSplitter.h`)
- `FluentWidget` (include: `Fluent/FluentWidget.h`)
- `FluentLabel` (include: `Fluent/FluentLabel.h`)

Demo pages: Containers (`demo/pages/PageContainers.cpp`) and Overview (`demo/pages/PageOverview.cpp`).

## FluentCard

```cpp
#include "Fluent/FluentCard.h"

auto *card = new Fluent::FluentCard();
card->setTitle(QStringLiteral("Title"));
card->setCollapsible(true);
```

Purpose: content container with optional collapse/expand behavior. Most demo sections are built using cards.

Structure & collapse mechanics (implementation semantics):

When `setCollapsible(true)` is enabled, `FluentCard` ensures this internal structure:

- Root layout: if you didn't set a layout, it auto-creates a `QVBoxLayout` with `ContentsMargins(16,14,16,14)` and `spacing=10`.
- Header: a `FluentCardHeader` widget containing:
	- Title `FluentLabel` (`objectName: FluentCardTitle`)
	- Right chevron `FluentToolButton` (`objectName: FluentCardChevron`, fixed ~28×28)
	- Header uses `PointingHandCursor` and toggles collapse/expand via an event filter on left-click.
- Content: a `FluentCardContent` widget with an internal `QVBoxLayout` (`spacing=8`).

Content relocation: if you add widgets/layouts to the card before enabling collapsible mode, those items are moved into `FluentCardContent` automatically.

Collapse animation:

- Uses `QPropertyAnimation` on `contentWidget()->maximumHeight` (~160ms, `OutCubic`).
- On collapse end: sets the content area invisible and forces `maximumHeight=0`.
- On expand: restores visibility and releases the max height back to `QWIDGETSIZE_MAX`.

Caveats:

- The whole header row is clickable (not only the chevron button).
- The animation is a "maximumHeight" animation. If your content uses its own `maximumHeight` constraints, be aware of combined effects.

Key APIs:

- `setTitle(const QString&)` / `title()`
- `setCollapsible(bool)` / `isCollapsible()`
- `setCollapsed(bool)` / `isCollapsed()` + `collapsedChanged(bool)`
- `setCollapseAnimationEnabled(bool)`
- `contentWidget()` / `contentLayout()`

Demo: almost all pages.

---

## FluentFlowLayout

Purpose: adaptive flow layout (optional uniform item width, hysteresis, and geometry animations).

Example:

```cpp
#include "Fluent/FluentFlowLayout.h"

auto *host = new QWidget();
auto *flow = new Fluent::FluentFlowLayout(host, 0, 12, 12);
flow->setUniformItemWidthEnabled(true);
flow->setMinimumItemWidth(320);
host->setLayout(flow);

flow->addWidget(new QWidget());
```

Key APIs:

- `setUniformItemWidthEnabled(bool)` / `uniformItemWidthEnabled()`
- `setMinimumItemWidth(int)` / `minimumItemWidth()`
- `setColumnHysteresis(int)`
- Animation: `setAnimationEnabled(bool)`, `setAnimationDuration(int)`, `setAnimationEasing(...)`, `setAnimationThrottle(int)`, `setAnimateWhileResizing(bool)`.

Demo: Containers / Overview.

Implementation notes (layout algorithm):

- `hasHeightForWidth() == true`: the layout height is derived from `heightForWidth(width)`.
- Wrap decision uses an exclusive right edge (`rightEdgeExclusive = x + availableW`) to avoid off-by-one wrapping caused by `QRect::right()` being inclusive.
- Item height selection order:
	1) if the `QLayoutItem` has HFW → `heightForWidth(itemW)`
	2) else if the widget contains a layout with HFW → `layout->totalHeightForWidth(itemW)`
	3) else → `sizeHint().height()`

Uniform item width / column hysteresis:

- When `uniformItemWidthEnabled` is on, it computes `idealCols` from available width, `minimumItemWidth()`, and spacing.
- It then applies `columnHysteresis()` so the column count only changes after the width crosses the next/previous threshold by an extra hysteresis margin. This avoids column thrash while resizing.
- Uniform width is computed as `uniformW = (availableW - (cols-1)*spaceX) / cols`.

Grouping / line-break properties (set on child widgets):

- `fluentFlowFullRow=true`: starts on a new line and occupies the full available width.
- `fluentFlowBreakBefore=true`: forces a line break before the widget.
- `fluentFlowBreakAfter=true`: forces a line break after the widget.

Example (group title full-row):

```cpp
auto *title = new Fluent::FluentLabel(QStringLiteral("Group"));
title->setProperty(Fluent::FluentFlowLayout::kFullRowProperty, true);
flow->addWidget(title);
```

Geometry animations (implementation semantics):

- When `animationEnabled` is true, it animates each child widget's `geometry` using `QPropertyAnimation`.
- Default animation: ~140ms, `OutCubic`.
- Animations are cached per widget (persistent `QPropertyAnimation` instances) to reduce churn during resize.
- Throttling (`animationThrottle`, default ~50ms):
	- if an animation is already running and we are within the throttle window, it only updates `endValue` (smoothly steers toward the new target)
	- otherwise it restarts (stop → start from current geometry)

Animate while resizing:

- `animateWhileResizing=true`: every relayout animates to the new target geometries.
- `animateWhileResizing=false`: applies geometry immediately for responsiveness (batched with parent updates disabled), then plays a single animation to the final layout after a debounce delay (default ~90ms).

Avoid fighting child height animations:

- If any child widget sets `fluentFlowDisableAnimation=true`, the relayout pass skips geometry animations:
	- stops all in-flight animations
	- applies geometry in a batch (temporarily disables parent updates)
	- useful for collapsible cards or any widget that animates its own height.

---

## FluentSplitter

Purpose: Fluent-styled splitter with a custom-painted handle.

Implementation notes:

- Defaults: `setChildrenCollapsible(false)` and `setHandleWidth(8)` (wide enough to grab).
- `createHandle()` returns a custom `QSplitterHandle` implementation:
	- enables mouse tracking and sets a split cursor (`SplitHCursor` / `SplitVCursor`)
	- hover animation: `QVariantAnimation` (~120ms, `OutCubic`) drives `hoverLevel`
	- painting:
		- always-on separator line using `colors.border`, alpha increases with hover, with ~6px inset
		- on hover it draws a small pill (semi-transparent `colors.hover`) and three center grip dots (`colors.subText`) for a Win11-like affordance
- The splitter background is kept transparent (`QSplitter { background: transparent; }`); visuals come from the handle.

Key APIs:

- Inherits `QSplitter` (use `addWidget()` / `setSizes()` / `setStretchFactor()` etc.)

Demo: Containers / Overview.

---

## FluentScrollArea / FluentScrollBar

Purpose: scroll container + Fluent overlay scrollbars (Win11-like).

FluentScrollArea: transparent viewport + overlay scrollbars

- Prefers a transparent-looking viewport (uses `FluentWidget` as viewport and avoids platform styles filling a fixed gray background, common on Windows).
- Overlay scrollbars are implemented as "dual scrollbars":
	- internal scrollbars are the real drivers of scrolling (range/value/pageStep)
	- overlay scrollbars are children of the viewport and render on top of content
- The two are kept in sync for range/value/pageStep/singleStep. Dragging the overlay scrollbar writes value back to the internal one (signals are not blocked because `QAbstractScrollArea` relies on them to scroll).

Reveal / hide policy:

- A viewport event filter reveals on Enter/Wheel and starts a ~700ms timer on Leave to auto-hide.
- `setScrollBarsRevealed(bool)` forces show/hide and stops the hide timer.

Overlay geometry:

- Thickness is fixed (~10px), margin ~2px.
- Each direction shows only when scrolling is needed (`minimum() < maximum()`).
- If both directions are visible, it reserves space for the corner to avoid overlap.

Key APIs (FluentScrollArea):

- `contentWidget()` / `contentLayout()` / `setContentLayout(QLayout*)`
- `setOverlayScrollBarsEnabled(bool)` / `overlayScrollBarsEnabled()`
- `setScrollBarsRevealed(bool)`

Key APIs (FluentScrollBar):

- `setOverlayMode(bool)` / `overlayMode()`
- `setForceVisible(bool)` / `forceVisible()`
- `revealLevel` / `hoverLevel` (Q_PROPERTY)

Demo: Inputs (scrollbar) / Overview.

FluentScrollBar: painting & interaction (implementation semantics)

- Fixed thickness (~10px; fixed width for vertical / fixed height for horizontal).
- Paints a pill thumb and deliberately avoids `WA_TranslucentBackground` (on some backends, translucent child widgets can show black artifacts).
- Thumb colors are derived from background luminance (chooses a light or dark thumb with different alpha for normal/hover/pressed).
- Overlay mode:
	- `revealLevel` fades in/out; below a small threshold it simply doesn't paint.
	- Reveal is driven by viewport interactions (Enter/MouseMove/Wheel) and the same ~700ms hide timer.
- Non-overlay mode: if used as the real scrollbar of a `QAbstractScrollArea`, it fills the reserved track area to match the themed viewport background (avoids a mismatched strip).

---

## FluentTabWidget

Purpose: Win11 Settings-like tab widget (tab transition animations, navigation indicator animation, and a frame overlay).

Implementation notes (key behavior):

- Internally replaces the `QTabBar` with a custom `FluentTabBar`:
	- `setDocumentMode(true)`, `setDrawBase(false)`, `setExpanding(false)`, `ElideRight`, enables scroll buttons, and uses mouse tracking.
	- Handles hover/press states itself and custom-paints backgrounds.
	- The selection indicator is a `QVariantAnimation` (~240ms, `InOutCubic`) that interpolates an `indicatorRect`:
		- Horizontal tabs: a 3px accent underline at the bottom (with side inset).
		- Vertical navigation (West/East): a 3px accent indicator bar on the left/right that smoothly slides with the current tab, plus a subtle selected background block (Win11 Settings feel).

- Frame overlay: creates a transparent `FluentTabFrameOverlay` on top (`WA_TransparentForMouseEvents`) to:
	- paint a 1px rounded border
	- "clip" the corners visually by filling the outside-of-rounded-rect area with background (OddEvenFill cut path), so child widgets don't square-off the corners.

- Scroll buttons: when tabs overflow, it finds internal `QToolButton`s after show/layout and replaces arrow types with custom chevron icons (colored with `colors.text`) and normalizes the hit target (~26×26).

Caveat: `tabRect()` can be lazy during startup; if a tab rect isn't valid yet, the indicator sync/animation may retry on the next event loop tick.

Key APIs:

- Inherits `QTabWidget` (use `addTab()` / `setTabPosition()` / `setCurrentIndex()` etc.)

Demo: Containers / Overview.

---

## FluentGroupBox

Purpose: Fluent-styled group box.

Implementation notes:

- Sets `WA_StyledBackground` so the stylesheet can control background/border.
- Default `setContentsMargins(12, 20, 12, 12)` to reserve space for the title.
- Theme coupling:
	- Applies `Theme::groupBoxStyle(colors)` as a stylesheet.
	- Re-applies on `ThemeManager::themeChanged` and `EnabledChange` (with a string compare guard to avoid redundant sets).

Key APIs:

- Inherits `QGroupBox` (use `setTitle()` / `setCheckable()` etc.)

Demo: Containers / Overview.

---

## FluentWidget

Purpose: a basic container that standardizes background roles (WindowBackground / Surface / Transparent).

Implementation notes:

- Sets `WA_StyledBackground` but uses custom painting (`setAutoFillBackground(false)` + `paintEvent()`).
- `BackgroundRole` semantics:
	- `Transparent`: `paintEvent()` returns immediately (draws nothing).
	- `Surface`: fills with `colors.surface`.
	- `WindowBackground`: fills with `colors.background`.
- Theme changes / Enabled changes trigger `update()`; it does not use a stylesheet for the background (paint-only container).

Key APIs:

- `setBackgroundRole(BackgroundRole)` / `backgroundRole()`

Demo: Containers / Windows / Overview.

---

## FluentLabel

Purpose: label that follows theme changes.

Implementation notes:

- Does not clobber user styles (e.g. `font-weight`). Instead it appends a tiny color rule with a marker comment `/*FluentLabelTheme*/`.
- On each theme/enabled change, it strips any previous marker + rule and appends the updated `color: ...;`.
- Uses `colors.text` when enabled, and `colors.disabledText` when disabled.

Key APIs:

- Inherits `QLabel` (use `setText()` / `setWordWrap()` etc.)

Demo: used across pages.
