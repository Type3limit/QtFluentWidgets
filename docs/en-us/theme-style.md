# Theme / Style

Most widgets rely on a unified theme and a small set of paint primitives:

- `ThemeManager`: toggles light/dark mode, updates global `ThemeColors`, and emits `themeChanged()`.
- `Theme`: generates QSS fragments (button/input/menu styles, etc.).
- `Style`: provides metrics (radius/padding/titlebar sizes) and paint primitives.

Public headers:

- `Fluent/FluentTheme.h`
- `Fluent/FluentStyle.h`

Demo pages: Overview (`demo/pages/PageOverview.cpp`) and Windows (`demo/pages/PageWindows.cpp`).

## Toggle theme

```cpp
#include "Fluent/FluentTheme.h"

using namespace Fluent;

ThemeManager::instance().setLightMode();
ThemeManager::instance().setDarkMode();
```

Key APIs:

- `ThemeManager::instance()`
- `setThemeMode(ThemeMode)` / `themeMode()`
- `setColors(const ThemeColors&)` / `colors()`
- `themeChanged()` signal

Implementation notes:

- `themeChanged()` is **coalesced**: multiple calls to `setColors()` / `setThemeMode()` / `setAccentBorderEnabled()` within the same event loop tick are merged into a single `themeChanged()` emission (via `QTimer::singleShot(0, ...)`).
- `setThemeMode()` preserves the current `accent` across Light/Dark switches (Fluent-like behavior), and derives `focus` as `accent.lighter(135)`.

## ThemeColors fields

`ThemeColors` is a shared semantic palette used across the whole library (widgets should depend on these instead of hard-coded colors):

- `accent`: accent color (primary buttons, focus/selection, etc.)
- `text` / `subText` / `disabledText`: text colors
- `background`: window background (applied to `QWidget:window` / `QMainWindow` / `QDialog`)
- `surface`: card/input surfaces
- `border`: 1px strokes
- `hover` / `pressed` / `focus`: interaction state colors
- `error`: error state

Optional accent border:

```cpp
ThemeManager::instance().setAccentBorderEnabled(true);
```

## Customize colors

```cpp
auto colors = ThemeManager::instance().colors();
colors.accent = QColor("#2D7DFF");
ThemeManager::instance().setColors(colors);
```

## Paint a Fluent-like input surface

```cpp
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

QPainter p(this);
Style::paintControlSurface(p, QRectF(rect()), ThemeManager::instance().colors(), hover, focus, isEnabled(), pressed);
```

More related APIs:

- `Style::metrics()`
- `Style::windowMetrics()` / `Style::setWindowMetrics(...)`
- `Style::roundedRectPath(...)`, `Style::paintTraceBorder(...)`, `Style::paintElevationShadow(...)`

Implementation notes:

- `Style::setWindowMetrics()` applies defensive clamps for trace-related fields (e.g. duration 1..60000ms, overshoot up to 0.25, overshootAt up to 0.99) to keep animations in a valid range.
- `Style::paintTraceBorder()` supports a small **overshoot pulse**: if `progress > 1` (rawProgress overshoots), it draws a full border with a slightly increased width and alpha (overshoot capped) to feel more Fluent.

## Theme::baseStyleSheet (global QSS)

`Theme::baseStyleSheet(colors)` is the global baseline stylesheet, typically applied at the application level (e.g. by `FluentMainWindow` via `qApp->setStyleSheet(...)`). It includes:

- Default font family and size
- Window background (`QWidget:window, QMainWindow, QDialog`)
- Win11-like overlay scrollbars (handle becomes visible on `QAbstractScrollArea:hover`)
- `QToolTip` styling
- `QLabel#FluentLink` link color

Related headers (used by windows/menus/dialogs for accent borders):

- `Fluent/FluentBorderEffect.h`
- `Fluent/FluentAccentBorderTrace.h`
- `Fluent/FluentFramePainter.h`
