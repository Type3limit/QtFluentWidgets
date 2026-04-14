# NavigationView

## Widgets

- `FluentNavigationView` (include: `Fluent/FluentNavigationView.h`)
- `FluentNavigationItem`: the data model used by NavigationView

Demo entry points: the Containers page (`demo/pages/PageContainers.cpp`) and the main demo shell in `demo/DemoWindow.cpp`.

## Basic usage

```cpp
#include "Fluent/FluentNavigationView.h"

using NI = Fluent::FluentNavigationItem;

auto *nav = new Fluent::FluentNavigationView();
nav->setExpandedWidth(280);
nav->setCompactWidth(48);
nav->setPaneDisplayMode(Fluent::FluentNavigationView::Left);
nav->setPaneTitle(QStringLiteral("Pane Title"));
nav->setBackButtonVisible(true);
nav->setAutoCollapseWidth(800);

auto applyGlyph = [](NI &item, ushort codePoint) {
        item.iconGlyph = QString(QChar(codePoint));
        item.iconFontFamily = QStringLiteral("Segoe Fluent Icons");
};

NI home;
home.key = QStringLiteral("home");
home.text = QStringLiteral("Home");
applyGlyph(home, 0xE80F);
nav->addItem(home);

NI documents;
documents.key = QStringLiteral("documents");
documents.text = QStringLiteral("Document options");
documents.selectsOnInvoked = false;
applyGlyph(documents, 0xE8A5);

NI recent;
recent.key = QStringLiteral("recent_files");
recent.text = QStringLiteral("Recent files");
applyGlyph(recent, 0xE823);
documents.children.push_back(recent);

nav->addItem(documents);

NI help;
help.key = QStringLiteral("help_center");
help.text = QStringLiteral("Help center");
applyGlyph(help, 0xE897);
nav->addFooterItem(help);

nav->setSelectedKey(QStringLiteral("home"));

QObject::connect(nav, &Fluent::FluentNavigationView::itemInvoked,
                                 nav, [](const QString &key) {
        qDebug() << "invoked:" << key;
});
```

## Data model

`FluentNavigationItem` exposes these fields:

- `key`: unique identifier used by `setSelectedKey()`, `selectedKeyChanged`, and `itemInvoked`.
- `text`: display label.
- `icon`: regular `QIcon`.
- `iconGlyph`: optional font glyph icon; if present, it is preferred over `icon`.
- `iconFontFamily`: font family used for `iconGlyph`; defaults to `Segoe Fluent Icons`.
- `separator`: when `true`, the item is rendered as a horizontal separator.
- `selectsOnInvoked`: whether clicking the item should also update `selectedKey`; set it to `false` for WinUI-style submenu-only parents.
- `children`: child items; the current implementation supports one nesting level.

## Pane modes and chrome

Key APIs:

- `setPaneDisplayMode(PaneDisplayMode)`: switches between `Left`, `LeftCompact`, and `Top`.
- `setExpandedWidth(int)` / `setCompactWidth(int)`: control the left expanded and compact widths.
- `setPaneTitle(const QString &)`: sets the title shown in the left control row.
- `setBackButtonVisible(bool)` / `setBackButtonEnabled(bool)`: control the built-in back button.
- `setFooterVisible(bool)`: shows or hides the entire footer area.
- `setExpanded(bool)` / `toggleExpanded()`: switches between `Left` and `LeftCompact`; these do not affect `Top` mode.

Layout semantics:

- `Left`: expanded left pane with labels, paneTitle, and optional `headerWidget`.
- `LeftCompact`: compact left pane with icons only; parents with children open flyouts when invoked.
- `Top`: horizontal top navigation; footer items move to the right side of the bar.

## Item and footer APIs

Use the incremental APIs when the menu is composed dynamically:

- `addItem(const FluentNavigationItem &item)`
- `setItems(const std::vector<FluentNavigationItem> &items)`
- `clearItems()`
- `addFooterItem(const FluentNavigationItem &item)`
- `setFooterItems(const std::vector<FluentNavigationItem> &items)`
- `clearFooterItems()`

`FluentNavigationView` does not create a built-in Settings entry. Footer content is fully application-defined, so you can explicitly add Help, Account, Feedback, About, or Settings items as needed.

## Interaction semantics

- In `Left`, body clicks and chevron clicks are separated: the body selects or invokes, while the chevron only expands or collapses the parent.
- Only one group stays expanded at a time.
- Selecting a child via `setSelectedKey()` auto-expands the parent in `Left`; in `Top`, the parent item stays highlighted.
- For parents with children and `selectsOnInvoked = false`:
    - In `Left`, clicking the body only expands or collapses the group.
    - In `LeftCompact` and `Top`, clicking the body only opens the flyout and does not change `selectedKey`.
- For leaf items with `selectsOnInvoked = false`, clicking still emits `itemInvoked`, but the current selection stays unchanged.

## Header, auto-collapse, and signals

Optional header:

```cpp
auto *header = new QWidget();
// ... populate the header layout
nav->setHeaderWidget(header);
```

The header is only shown in `Left` when the pane is wide enough.

Auto-collapse:

- `setAutoCollapseWidth(int)` switches from `Left` to `LeftCompact` when the parent window becomes narrow.
- Returning to `Left` uses about `+100px` of hysteresis to reduce resize thrash.
- `Top` mode is not managed by the auto-collapse behavior.

Signals:

- `selectedKeyChanged(const QString &key)`
- `itemInvoked(const QString &key)`
- `backRequested()`
- `expandedChanged(bool expanded)`
- `paneDisplayModeChanged(PaneDisplayMode mode)`

## Demo notes

- The main demo shell uses `addFooterItem()` explicitly; the Settings page is just one regular footer item added by the demo itself.
- The Containers page includes an API-in-action sample for:
    - `Left`, `LeftCompact`, and `Top` switching
    - back button visibility and enable state
    - explicit footer composition and visibility
    - `selectsOnInvoked` and submenu-only parent behavior
