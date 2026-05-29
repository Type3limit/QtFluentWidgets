# Theme / Style

Most widgets rely on a unified theme and a small set of paint primitives:

- `ThemeManager`: toggles light/dark mode, updates global `ThemeColors`, and emits `themeChanged()`.
- `Theme`: generates QSS fragments (button/input/menu styles, etc.).
- `Style`: provides metrics (radius/padding/titlebar sizes) and paint primitives.
- `FluentMotion`: provides semantic motion tokens for hover/press/popup/collapse/selection behavior.

Public headers:

- `Fluent/FluentTheme.h`
- `Fluent/FluentStyle.h`
- `Fluent/FluentMotion.h`

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
- `setAnimationsEnabled(bool)` / `animationsEnabled()`
- `themeChanged()` signal

Implementation notes:

- `themeChanged()` is **coalesced**: multiple calls to `setColors()` / `setThemeMode()` / `setAccentBorderEnabled()` within the same event loop tick are merged into a single `themeChanged()` emission (via `QTimer::singleShot(0, ...)`).
- `setThemeMode()` preserves the current `accent` across Light/Dark switches (Fluent-like behavior), and derives `focus` as `accent.lighter(135)`.
- `animationsEnabled()` affects widgets that have moved to `FluentMotion`: when disabled, semantic durations become 0 so popups, collapse motion, and button state feedback complete immediately where supported.

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

Implementation notes:

- `accentBorderEnabled()` is not just a plain on/off flag: window-layer widgets (`FluentMainWindow`, `FluentDialog`, `FluentMessageBox`, `FluentToast`) feed it through `FluentBorderEffect`, which switches between normal border and accent border states and can play a trace-in animation when enabled.
- `FluentMainWindow` currently paints that border in a dedicated top-level overlay, so the border can wrap both the title bar and the central widget without being covered by opaque child widgets.

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
- The values in `Style::windowMetrics()` now directly affect `FluentMainWindow` too: `titleBarHeight` / `windowButtonWidth` / `resizeBorder` shape title-bar layout and Windows hit-testing, while the trace fields tune the outer accent-border animation.

## Motion Tokens

`FluentMotion` is the new semantic motion layer introduced by the visual-quality work. Widgets should prefer `FluentMotionRole` instead of hard-coding values like `setDuration(150)`:

```cpp
#include "Fluent/FluentMotion.h"

auto *anim = new QVariantAnimation(this);
Fluent::FluentMotion::configure(anim, Fluent::FluentMotionRole::Hover);

// Applications can override durations by semantic role.
Fluent::FluentMotion::setDuration(Fluent::FluentMotionRole::Hover, 90);
Fluent::FluentMotion::setDuration(Fluent::FluentMotionRole::PopupOpen, 220);

// Or replace the whole token set for a snappier or calmer motion profile.
auto tokens = Fluent::ThemeManager::instance().motionTokens();
tokens.selectionDuration = 120;
tokens.collapseDuration = 240;
Fluent::ThemeManager::instance().setMotionTokens(tokens);
```

Common roles:

- `Hover` / `Press` / `Focus`
- `PopupOpen` / `PopupClose`
- `Collapse`
- `Selection`
- `Navigation` / `Page`
- `Layout`
- `Toast`
- `WheelSnap`

Configuration notes:

- `FluentMotion::configuredDuration(role)` returns the user-configured duration.
- `FluentMotion::duration(role)` returns the effective duration; it becomes 0 when global animations are disabled.
- `ThemeManager::setMotionTokens(...)` is preserved across light/dark and color changes, so theme refreshes do not reset custom motion.

Reduced motion:

- `ThemeManager::setAnimationsEnabled(false)` makes `FluentMotion::duration(...)` return 0.
- Migrated widgets complete their next state change immediately; for example, Button/ToolButton/LineEdit/ComboBox hover and focus feedback jumps to the final state, popups jump to their final geometry, and Toggle/Check/Radio/Slider/Progress controls jump to the target state.
- Container, navigation, and popup widgets follow the same switch: NavigationView pane/selection, TabWidget indicators, DataViews hover, ScrollBar reveal/hover, DateRangePicker hover, TeachingTip mask, Toast opacity, and the demo Page transition complete immediately or render as static states.
- For an indeterminate `FluentProgressRing`, disabling global animations pauses rotation and leaves a static arc.

## Theme::baseStyleSheet (global QSS)

`Theme::baseStyleSheet(colors)` is the global baseline stylesheet, typically applied at the application level (e.g. by `FluentMainWindow` via `qApp->setStyleSheet(...)`). It includes:

- Default font family and size
- Window background (`QWidget:window, QMainWindow, QDialog`)
- Win11-like overlay scrollbars (handle becomes visible on `QAbstractScrollArea:hover`)
- `QToolTip` styling
- `QLabel#FluentLink` link color

Additional notes:

- `Theme::baseStyleSheet(...)` provides the global baseline look, but it does **not** directly paint the outer `FluentMainWindow` border; the main window accent border and trace animation are custom-painted.
- `FluentMainWindow` compares the next stylesheet string before reapplying it, which avoids unnecessary full-application repolish on redundant theme changes.

Related headers (used by windows/menus/dialogs for accent borders):

- `Fluent/FluentBorderEffect.h`
- `Fluent/FluentAccentBorderTrace.h`
- `Fluent/FluentFramePainter.h`
