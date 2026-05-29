# Utilities

This page covers lower-level helpers that are public but mostly used internally or for advanced customization.

Demo: you can observe their effects on the Windows and Overview pages (menus/dialogs/toasts use them internally).

## FluentDiagnostics

include: `Fluent/FluentDiagnostics.h`

Purpose: optional Qt warning output controls for Debug builds, useful when known noisy warnings dominate startup or theme-switch profiling.

Key APIs:

- `Diagnostics::setKnownQtWarningSuppressionEnabled(true)`: filters known high-frequency Qt warnings, such as invalid font sizes and `QWidget::paintEngine` / `QPainter::begin` noise.
- `Diagnostics::setQtWarningOutputEnabled(false)`: suppresses every `QtWarningMsg`. This is intentionally broad; use it only for local Debug performance checks.
- `Diagnostics::installMessageHandler()`: installs the library message handler manually. The setters above install it automatically, so most apps do not need to call this directly.

Minimal example:

```cpp
#include <Fluent/FluentDiagnostics.h>

int main(int argc, char *argv[])
{
#ifdef QT_DEBUG
    Fluent::Diagnostics::setKnownQtWarningSuppressionEnabled(true);
    // Or temporarily suppress every Qt warning:
    // Fluent::Diagnostics::setQtWarningOutputEnabled(false);
#endif

    QApplication app(argc, argv);
    ...
    return app.exec();
}
```

Environment variables are also supported:

- `QTFLUENT_SUPPRESS_KNOWN_WARNINGS=1`: filters known noisy warnings.
- `QTFLUENT_DISABLE_QT_WARNINGS=1`: suppresses every Qt warning.
- `QTFLUENT_SUPPRESS_KNOWN_PAINT_WARNINGS=1`: compatibility alias for older paint-warning filtering.

Note: this is a diagnostics convenience, not a substitute for fixing real issues. Release performance should still come from avoiding eager page construction, large `setCellWidget` tables, and global stylesheet churn.

---

## FluentAccentBorderTrace

include: `Fluent/FluentAccentBorderTrace.h`

Purpose: shared controller for the Win11-like accent border trace animation (no moc required; uses `QVariantAnimation`).

Implementation notes:

- Disabling the accent border (`toEnabled=false`) does **not** play a “disable animation”: it stops immediately (`t() == -1`) and requests an update.
- Enabling uses `Style::windowMetrics()` for tuning:
	- `accentBorderTraceDurationMs`
	- `accentBorderTraceEnableEasing` / `accentBorderTraceDisableEasing`
	- Optional overshoot: at `accentBorderTraceEnableOvershootAt`, progress goes to `1 + accentBorderTraceEnableOvershoot` for a subtle pulse.
- `setRequestUpdate(...)` lets you replace `updateTarget->update()` with a custom refresh hook (e.g. update multiple overlays).

Key APIs:

- `setCurrentEnabled(bool)` / `currentEnabled()`
- `onEnabledChanged(bool)`
- `play(bool fromEnabled, bool toEnabled)` / `stop()`
- `isAnimating()` / `t()`
- `setRequestUpdate(std::function<void()>)`

---

## FluentBorderEffect

include: `Fluent/FluentBorderEffect.h`

Purpose: convenience wrapper around accent border enable + trace-in animation integration.

Implementation notes:

- `syncFromTheme()` / `onThemeChanged()` only read `ThemeManager::accentBorderEnabled()` and forward the state into the underlying trace.
- `playInitialTraceOnce()` plays at most once per instance (useful for popups on show). Call `resetInitial()` if you need to replay.
- `applyToFrameSpec(...)` injection strategy:
	- Not animating: `frame.borderColorOverride` is the current state border (accent or normal).
	- Animating: `frame.borderColorOverride` uses the **from** state color; `frame.traceEnabled=true`, `frame.traceColor` uses the **to** state color, `frame.traceT` is `t()`.

Key APIs:

- `syncFromTheme()` / `onThemeChanged()`
- `playInitialTraceOnce(int delayMs = 0)`
- `applyToFrameSpec(FluentFrameSpec&, const ThemeColors&, ...)`

---

## FluentMotion

include: `Fluent/FluentMotion.h`

Purpose: shared semantic motion tokens so hover, press, focus, popup open, selection, and collapse animations no longer hard-code separate durations and easing curves in every widget.

Key APIs:

- `FluentMotion::duration(FluentMotionRole)`: returns the semantic duration; returns 0 when `ThemeManager::animationsEnabled()` is false.
- `FluentMotion::configuredDuration(FluentMotionRole)`: returns the configured duration and is not affected by the global animation switch.
- `FluentMotion::easing(FluentMotionRole)`: returns the semantic easing curve.
- `FluentMotion::configure(QVariantAnimation*, role)` / `configure(QPropertyAnimation*, role)`: applies duration and easing to an existing animation object.
- `FluentMotion::setDuration(role, ms)`: changes a single semantic animation duration.
- `FluentMotion::setTokens(...)` / `resetTokens()`: replace or reset all motion tokens.
- `FluentMotion::popupSlideOffset()` / `pressOffset()`: shared popup and press offset tokens.
- `ThemeManager::setAnimationsEnabled(bool)` / `animationsEnabled()`: global animation switch.
- `ThemeManager::setMotionTokens(...)` / `motionTokens()`: application-level motion token configuration.

Currently wired to:

- hover / press on `FluentButton`, `FluentToolButton`, and `FluentAnimatedButton`; these controls resync tokens when the theme or global animation switch changes.
- hover / focus on `FluentLineEdit`; when global animations are disabled, the next hover/focus transition jumps to its final state.
- hover, focus, checked, and position transitions on `FluentToggleSwitch`, `FluentCheckBox`, `FluentRadioButton`, and `FluentSlider`.
- determinate progress transitions on `FluentProgressBar` and `FluentProgressRing`; when global animations are disabled, progress jumps to the target value immediately and indeterminate ring rotation pauses.
- popup opening for `FluentComboBox`, `FluentAutoSuggestBox`, `FluentMenu`, `FluentMenuBar`, `FluentFlyout`, `FluentCalendarPopup`, `FluentDatePicker`, and `FluentTimePicker`; ComboBox/AutoSuggestBox list hover and selection feedback also follow token updates.
- `FluentCard` collapse motion and wheel-picker snap motion.
- `FluentNavigationView` pane width, hover, and selection indicator; `FluentTabWidget` hover/selection; DataViews hover/selection; `FluentScrollBar` reveal/hover; `FluentDateRangePicker` hover; `FluentTeachingTip` mask fade-in/fade-out; `FluentToast` appear/disappear and queue movement.
- `FluentFlowLayout` geometry reflow motion; explicit `setAnimationDuration()` / `setAnimationEasing()` still overrides the default token, while disabling global animations makes items snap into place.
- The demo main window page transition uses `FluentMotionRole::Page`, so the Motion page can show the Page token in a real navigation flow.

Example:

```cpp
auto *anim = new QVariantAnimation(this);
Fluent::FluentMotion::configure(anim, Fluent::FluentMotionRole::PopupOpen);

// Users can tune durations by semantic role; migrated widgets resync on themeChanged.
Fluent::FluentMotion::setDuration(Fluent::FluentMotionRole::PopupOpen, 220);
Fluent::FluentMotion::setDuration(Fluent::FluentMotionRole::Selection, 120);
```

---

## FluentFramePainter

include: `Fluent/FluentFramePainter.h`

Purpose: common rounded frame/panel painter for translucent top-level widgets.

Implementation notes:

- `paintFluentFrame(...)` is for translucent top-level windows: when `FluentFrameSpec::clearToTransparent=true`, it clears the whole widget rect to transparent first to avoid corner artifacts.
- `paintFluentPanel(...)` is for drawing a rounded panel inside a larger translucent widget: it does not clear the whole window; it only paints the given `panelRect`.

## Minimal integration example (custom painted panel + accent border)

This is the same pattern used by menus/dialogs/message boxes/toasts:

```cpp
class MyPanel : public QWidget {
	Q_OBJECT
public:
	explicit MyPanel(QWidget *parent = nullptr)
		: QWidget(parent)
		, m_border(this)
	{
		m_border.syncFromTheme();
		connect(&Fluent::ThemeManager::instance(), &Fluent::ThemeManager::themeChanged,
				this, [this] { m_border.onThemeChanged(); update(); });
	}

protected:
	void showEvent(QShowEvent *e) override {
		QWidget::showEvent(e);
		m_border.playInitialTraceOnce();
	}

	void paintEvent(QPaintEvent *) override {
		QPainter p(this);
		Fluent::FluentFrameSpec spec;
		spec.radius = 10;
		spec.clearToTransparent = false;

		const auto colors = Fluent::ThemeManager::instance().colors();
		m_border.applyToFrameSpec(spec, colors);
		Fluent::paintFluentPanel(p, QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), colors, spec);
	}

private:
	Fluent::FluentBorderEffect m_border;
};
```

Key APIs:

- `FluentFrameSpec`
- `paintFluentFrame(...)`
- `paintFluentPanel(...)`

---

## FluentToolTip

include: `Fluent/FluentToolTip.h`

Purpose: Fluent-styled replacement for Qt's native tooltip popup.

Key APIs:

- `FluentToolTip::ensureInstalled()`
- `FluentToolTip::showText(const QPoint&, const QString&, QWidget* anchor = nullptr, int msecDisplayTime = -1)`
- `FluentToolTip::hideText()`

Implementation notes:

- Internally it creates a `Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint` top-level popup and paints it with `paintFluentPanel()`.
- Text width is capped at about `420px`, and default display time is estimated from the text length (~0.9s + 55ms per character, clamped to `1600..8000ms`).
- Position is based on the global cursor point plus an offset (~`+14,+22`), then clamped to the available screen geometry; if it does not fit below, it flips above the cursor.
- Once installed, it intercepts `QEvent::ToolTip`, reads `widget->toolTip()` / `widget->toolTipDuration()`, and hides on `Leave`, mouse press, wheel, key press, focus out, or app deactivation.

Typical use cases:

- make the whole app use a consistent Fluent tooltip visual
- show a temporary help bubble manually at a global position

---

## FluentPopupSurface

include: `Fluent/FluentPopupSurface.h`

Purpose: shared rounded-panel constants and helper functions for menus, combo boxes, calendar/time popups, and similar transient surfaces.

Note: `FluentPopupSurface` is not a widget class; it is a set of inline helpers under `namespace Fluent::PopupSurface`.

Key items:

- `kRadius` / `kBorderWidth` / `kShadowMargin*Px`
- `panelRect(const QRect&)`
- `withShadowMargins(const QSize&)` / `shadowContentRect(const QRect&)`
- `contentClipPath(const QRect&)`
- `paintPanel(...)` / `paintPanelWithShadowMargins(...)`

Implementation notes:

- `panelRect()` slightly insets the rect before painting to reduce border jitter on HiDPI and translucent popup windows.
- `withShadowMargins()` expands the logical content size into the top-level transparent-window size, leaving room for the software shadow. Use `shadowContentRect()` to recover the actual panel/content area.
- `contentClipPath()` returns a rounded path matching the popup panel border, useful for clipping popup contents so they do not visually square off the corners.
- `paintPanelWithShadowMargins()` paints the shared soft shadow first, then the surface / border / trace. It is the preferred helper for top-level translucent popups.
- `paintPanel()` remains useful for embedded panels that do not need the extra shadow margins. Both helpers build a `FluentFrameSpec` internally and inject accent-border / trace parameters automatically when a `FluentBorderEffect` is provided.
- Popup-open duration, easing, and displacement are now owned by `FluentMotionRole::PopupOpen` / `FluentMotion::popupSlideOffset()`; `FluentPopupSurface` only owns surface geometry and painting.
- These helpers are reused by `FluentMenu`, `FluentComboBox`, `FluentCalendarPopup`, and the `FluentDatePicker` / `FluentTimePicker` wheel popup to keep radius, shadow, and border width consistent.

---

## FluentQtCompat

include: `Fluent/FluentQtCompat.h`

Purpose: very small Qt5 / Qt6 compatibility layer for event and pointer-position handling.

Note: this is a **mostly internal public header**; normal app code usually does not need to include it directly. It primarily exists so widget implementations can avoid repeating `#if QT_VERSION` branches everywhere.

Key items:

- `using FluentEnterEvent = ...`
- `mousePositionF(const QMouseEvent*)`
- `globalMousePosition(const QMouseEvent*)`
- `wheelPositionF(const QWheelEvent*)`

Implementation notes:

- On Qt6 it maps to `QEnterEvent`, `event->position()`, and `event->globalPosition()`.
- On Qt5 it falls back to `QEvent`, `localPos()`, `globalPos()`, and `posF()`.
- This lets higher-level widgets (for example `FluentDial`) use one unified coordinate-reading path across Qt versions.

