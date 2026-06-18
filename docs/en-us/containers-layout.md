# Containers / Layout

## Widgets

- `FluentCard` (include: `Fluent/FluentCard.h`)
- `FluentGroupBox` (include: `Fluent/FluentGroupBox.h`)
- `FluentTabWidget` (include: `Fluent/FluentTabWidget.h`)
- `FluentScrollArea` (include: `Fluent/FluentScrollArea.h`)
- `FluentScrollBar` (include: `Fluent/FluentScrollBar.h`)
- `FluentAnnotatedScrollBar` (include: `Fluent/FluentAnnotatedScrollBar.h`)
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

// Opt-in animated "flow" background: a subtle, readable linear gradient from
// the shared flow palette (see theme-style.md) that slowly rotates. Off by
// default; the animation pauses when the card is hidden or animations are off.
card->setFlowBackgroundEnabled(true);
```

Purpose: content container with optional collapse/expand behavior. Most demo sections are built using cards.

`setFlowBackgroundEnabled(true)` overlays the card surface with a low-contrast animated gradient (colours from `ThemeManager::resolvedFlowColors()`, shared with the Flow accent border). It keeps the normal border/hover and stays readable; it is the same effect family as the window's Flow border, but linear rather than conic.

Structure & collapse mechanics (implementation semantics):

When `setCollapsible(true)` is enabled, `FluentCard` ensures this internal structure:

- Root layout: if you didn't set a layout, it auto-creates a `QVBoxLayout` with `ContentsMargins(16,14,16,14)` and `spacing=10`.
- Header: a `FluentCardHeader` widget containing:
	- Title `FluentLabel` (`objectName: FluentCardTitle`)
	- Right chevron `FluentToolButton` (`objectName: FluentCardChevron`, fixed ~28×28)
	- Header uses `PointingHandCursor` and toggles collapse/expand via an event filter on left-click.
- Content: a `FluentCardContentClip` clipping wrapper that hosts `FluentCardContent`, which owns the internal `QVBoxLayout` (`spacing=8`).

Content relocation: if you add widgets/layouts to the card before enabling collapsible mode, those items are moved into `FluentCardContent` automatically.

Collapse animation:

- Uses `QPropertyAnimation` on the clipping wrapper's `maximumHeight`; duration and easing come from `FluentMotionRole::Collapse`.
- During the animation, `FluentCardContent` is temporarily locked to its natural height while only the wrapper height changes. This keeps title/body text from reflowing or jumping.
- On collapse end: sets the clipping wrapper invisible and forces `maximumHeight=0`.
- On expand: restores visibility and releases the max height back to `QWIDGETSIZE_MAX`.

Caveats:

- The whole header row is clickable (not only the chevron button).
- The animation changes the clipping wrapper's `maximumHeight`. If your content needs fixed-height constraints, prefer setting them on child widgets rather than directly changing `contentWidget()` constraints.

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
- Default animation: the current `FluentMotionRole::Layout` duration and easing.
- Explicit `setAnimationDuration()` / `setAnimationEasing()` overrides the token.
- Reduced motion: disabling global animations or setting the `Layout` duration to 0 makes geometry changes snap into place instead of creating/running `geometry` animations.
- Animations are cached per widget (persistent `QPropertyAnimation` instances) to reduce churn during resize.
- Throttling (`animationThrottle`, default ~50ms):
	- if an animation is already running and we are within the throttle window, it only updates `endValue` (smoothly steers toward the new target)
	- otherwise it restarts (stop → start from current geometry)

Animate while resizing:

- `animateWhileResizing=true`: every relayout animates to the new target geometries.
- `animateWhileResizing=false`: applies geometry immediately for responsiveness (batched with parent updates disabled) and debounces the final target geometry; because children are already at the settled layout, the debounce pass does not create redundant per-step `geometry` animations.

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
	- hover animation: `QVariantAnimation` uses `FluentMotionRole::Hover` to drive `hoverLevel`; disabling global animations or setting the Hover duration to 0 completes it immediately
	- painting:
		- always-on separator line using `tokens.neutral.strokeSubtle`, alpha increases with hover, with ~6px inset
		- on hover it draws a small pill (semi-transparent `tokens.neutral.cardHover`) and three center grip dots (`tokens.neutral.strokeStrong`) for a Win11-like affordance
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
- Thumb colors are derived from the current `FluentThemeTokens::neutral` ramp, then opacity is applied for normal/hover/pressed states so dark and custom themes do not fall back to fixed black/white thumbs.
- Overlay mode:
	- `revealLevel` fades in/out; below a small threshold it simply doesn't paint.
	- Reveal is driven by viewport interactions (Enter/MouseMove/Wheel) and the same ~700ms hide timer.
	- Hover/pressed slightly thickens the thumb without changing the reserved layout thickness, so the feedback is more tactile than a pure color fade.
- Non-overlay mode: if used as the real scrollbar of a `QAbstractScrollArea`, it fills the reserved track area to match the themed viewport background (avoids a mismatched strip).

---

## FluentAnnotatedScrollBar

Purpose: a segmented companion scrollbar for long vertical content, with quick-jump labels and a right-side tooltip that reports the current section while scrolling.

Key APIs:

- `setScrollArea(QAbstractScrollArea*)`: bind directly to a scroll area and track its vertical scrollbar.
- `setScrollBar(QScrollBar*)`: bind to a scrollbar directly when no scroll area wrapper is involved.
- `setSources(const QVector<FluentAnnotatedScrollBarSource>&)`: the recommended high-level API. Each source carries `group`, `text`, `start`, and `end`, and the control aggregates them into visible group labels automatically.
- `addSource(...)` / `addSources(...)`: append sources incrementally. If a matching group already exists, the new source is inserted after that group's block; otherwise a new group is created.
- `sources()` / `clearSources()`
- `groups()`
- `setCurrentGroup(const QString&)` / `setCurrentRangeIndex(int)` / `setCurrentSourceIndex(int)`: external control of the current group, visible segment, or concrete source.
- `currentGroup()` / `currentRangeText()` / `currentSource()`
- `setAnnotatedRanges(const QVector<FluentAnnotatedScrollBarRange>&)`: low-level fallback when ranges are already grouped outside the widget.
- `clearAnnotatedRanges()`
- `setShowToolTipOnScroll(bool)`
- `setToolTipDuration(int)`

Implementation notes:

- The widget does not own or lay out the scrollable content. It only observes the linked scrollbar's `value` / `range` changes and paints labels for the configured ranges.
- In sources mode, visible labels are grouped by `group`, while the tooltip and `currentSourceChanged(...)` prefer the current source's `text`.
- At the top/bottom edge, the active range is pinned to the first/last configured section. While scrolling through the middle, it is resolved by checking which configured range overlaps the current visible interval `[value, value + pageStep)` the most, so the last section can still become active near the bottom of long pages.
- Clicking a label jumps to the corresponding section. Clicking the rail jumps to the matching scroll position, and the thumb also supports direct dragging.
- The rail, thumb, connectors, hover label, and active label are derived from the current theme tokens: thumb/active use the accent ramp, rail/connectors use neutral stroke, and hover labels use neutral cardHover.
- While scrolling, it uses `FluentToolTip::showText(...)` to display the current range on the right side.
- Typical setup: hide the internal vertical scrollbar of `FluentScrollArea` and keep `FluentAnnotatedScrollBar` as the external navigation rail.

Signals:

- `currentRangeChanged(int, const QString&)`
- `currentGroupChanged(int, const QString&)`
- `currentSourceChanged(int, const QString&, const QString&)`
- `annotatedRangesChanged()` / `sourcesChanged()`

```cpp
#include "Fluent/FluentAnnotatedScrollBar.h"
#include "Fluent/FluentScrollArea.h"

auto *area = new Fluent::FluentScrollArea();
area->setWidgetResizable(true);
area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

auto *annotated = new Fluent::FluentAnnotatedScrollBar();
annotated->setScrollArea(area);
annotated->setSources({
	{QStringLiteral("Overview"), QStringLiteral("Welcome"), 12, 119},
	{QStringLiteral("Overview"), QStringLiteral("Quick Actions"), 130, 237},
	{QStringLiteral("Projects"), QStringLiteral("Project List"), 248, 355},
	{QStringLiteral("Sync"), QStringLiteral("Queue State"), 366, 473}
});

annotated->addSource({QStringLiteral("Sync"), QStringLiteral("Sync History"), 484, 591});
annotated->setCurrentGroup(QStringLiteral("Projects"));

QObject::connect(annotated, &Fluent::FluentAnnotatedScrollBar::currentSourceChanged,
					 annotated, [](int, const QString &group, const QString &text) {
	qDebug() << group << text;
});
```

Demo: Containers / Overview.

---

## FluentTabWidget

Purpose: Win11 Settings-like tab widget (tab transition animations, navigation indicator animation, and a frame overlay).

Implementation notes (key behavior):

- Internally replaces the `QTabBar` with a custom `FluentTabBar`:
	- `setDocumentMode(true)`, `setDrawBase(false)`, `setExpanding(false)`, `ElideRight`, enables scroll buttons, and uses mouse tracking.
	- Handles hover/press states itself and custom-paints feedback.
	- The selection indicator is a `QVariantAnimation` using `FluentMotionRole::Selection` to interpolate an `indicatorRect`:
		- Horizontal tabs default to `TabDisplayMode::Underline`: a 3px accent underline at the bottom (with side inset), without a rounded selected background.
		- Horizontal tabs can switch to `TabDisplayMode::Document`: selected tabs are painted like Windows File Explorer tabs, useful for document/tabbed workflows. This mode does not use Qt's native close buttons; it paints a close glyph that matches the window close button so the native square close button does not leak into the Fluent surface.
		- Vertical navigation (West/East): a 3px accent indicator bar on the left/right that smoothly slides with the current tab, plus a subtle selected background block (Win11 Settings feel).
	- Tab hover/press, selected backgrounds, disabled fills, and indicators are derived from `FluentThemeTokens` (`neutral.card`, `cardHover`, `fillTertiary`, `strokeSubtle`, and `accent.base`), so document and vertical tabs avoid legacy hover/accent fallbacks; disabled indicators use `disabledText` instead of retaining the enabled accent.

- Frame overlay: creates a transparent `FluentTabFrameOverlay` on top (`WA_TransparentForMouseEvents`) to:
	- paint a 1px rounded border
	- paint only the border; it does not fill the outside corners, avoiding square patch edges on complex parent backgrounds.

- Scroll buttons: when tabs overflow, it finds internal `QToolButton`s after show/layout and replaces arrow types with custom chevron icons (colored with `colors.text`) and normalizes the hit target (~26×26).

Caveat: `tabRect()` can be lazy during startup; if a tab rect isn't valid yet, the indicator sync/animation may retry on the next event loop tick.

Key APIs:

- Inherits `QTabWidget` (use `addTab()` / `setTabPosition()` / `setCurrentIndex()` etc.)
- `setContentMargin(int)`: controls the pane content padding. The default is 5px, and the same padding is used by both Underline and Document modes.
- `setTabDisplayMode(FluentTabWidget::TabDisplayMode::Underline)`: default horizontal mode with an underline indicator.
- `setTabDisplayMode(FluentTabWidget::TabDisplayMode::Document)`: document-style tabs.
- `setTabsClosable(true)` / `tabCloseRequested(int)`: Document mode reserves room for close buttons so text and buttons do not overlap.
- `setMovable(true)` and `setCornerWidget()`: combine them for a File Explorer-like draggable tab strip with an add button. In Document mode, a `Qt::TopRightCorner` corner widget is automatically positioned beside the last tab instead of staying at the far right edge of the tab area.

Document mode paints the tab header and page pane as one surface: the top strip fills the available width with rounded top corners, the page pane uses square top corners and rounded bottom corners, and the selected tab aligns with the pane edge so the two parts read as one container.

Demo: Containers / Overview.

---

## FluentGroupBox

Purpose: Fluent-styled group box.

Implementation notes:

- Paint-only container: uses `FluentSurfaceSpec` for the Card surface, radius and border instead of a widget-level stylesheet.
- Default `setContentsMargins(14, 38, 14, 14)` to reserve room for the title and optional check indicator.
- The title is painted by the control and follows Theme colors; `setCheckable(true)` keeps the native `QGroupBox` checked semantics, enabled checked uses `accent.base` / `onAccent` for the custom checkbox indicator, and disabled checked keeps the neutral disabled surface with a `disabledText` checkmark, so platform-native indicator colors no longer leak into the control.
- Theme coupling:
	- `ThemeManager::themeChanged` and `EnabledChange` trigger repaint.
	- Surface, border and disabled colors come from Theme tokens, matching `FluentCard` / `FluentCommandBar` visual layers.

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
	- `Surface`: fills with `tokens.neutral.card`.
	- `WindowBackground`: fills with `tokens.neutral.background`.
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
