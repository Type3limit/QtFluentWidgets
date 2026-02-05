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

Key APIs:

- `setUniformItemWidthEnabled(bool)` / `uniformItemWidthEnabled()`
- `setMinimumItemWidth(int)` / `minimumItemWidth()`
- `setColumnHysteresis(int)`
- Animation: `setAnimationEnabled(bool)`, `setAnimationDuration(int)`, `setAnimationEasing(...)`, `setAnimationThrottle(int)`, `setAnimateWhileResizing(bool)`.

Demo: Containers / Overview.

---

## FluentSplitter

Purpose: Fluent-styled splitter with a custom-painted handle.

Key APIs:

- Inherits `QSplitter` (use `addWidget()` / `setSizes()` / `setStretchFactor()` etc.)

Demo: Containers / Overview.

---

## FluentScrollArea / FluentScrollBar

Purpose: scroll container + Fluent overlay scrollbars (Win11-like).

Key APIs (FluentScrollArea):

- `contentWidget()` / `contentLayout()` / `setContentLayout(QLayout*)`
- `setOverlayScrollBarsEnabled(bool)` / `overlayScrollBarsEnabled()`
- `setScrollBarsRevealed(bool)`

Key APIs (FluentScrollBar):

- `setOverlayMode(bool)` / `overlayMode()`
- `setForceVisible(bool)` / `forceVisible()`
- `revealLevel` / `hoverLevel` (Q_PROPERTY)

Demo: Inputs (scrollbar) / Overview.

---

## FluentTabWidget

Purpose: Win11 Settings-like tab widget (includes an internal frame overlay).

Key APIs:

- Inherits `QTabWidget` (use `addTab()` / `setTabPosition()` / `setCurrentIndex()` etc.)

Demo: Containers / Overview.

---

## FluentGroupBox

Purpose: Fluent-styled group box.

Key APIs:

- Inherits `QGroupBox` (use `setTitle()` / `setCheckable()` etc.)

Demo: Containers / Overview.

---

## FluentWidget

Purpose: a basic container that standardizes background roles (WindowBackground / Surface / Transparent).

Key APIs:

- `setBackgroundRole(BackgroundRole)` / `backgroundRole()`

Demo: Containers / Windows / Overview.

---

## FluentLabel

Purpose: label that follows theme changes.

Key APIs:

- Inherits `QLabel` (use `setText()` / `setWordWrap()` etc.)

Demo: used across pages.
