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

Related headers (used by windows/menus/dialogs for accent borders):

- `Fluent/FluentBorderEffect.h`
- `Fluent/FluentAccentBorderTrace.h`
- `Fluent/FluentFramePainter.h`
