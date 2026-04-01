# Utilities

This page covers lower-level helpers that are public but mostly used internally or for advanced customization.

Demo: you can observe their effects on the Windows and Overview pages (menus/dialogs/toasts use them internally).

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

Purpose: shared rounded-panel constants and helper functions for calendar/time/color popups and similar transient surfaces.

Note: `FluentPopupSurface` is not a widget class; it is a set of inline helpers under `namespace Fluent::PopupSurface`.

Key items:

- `kRadius` / `kBorderWidth` / `kOpenDurationMs` / `kOpenSlideOffsetPx`
- `panelRect(const QRect&)`
- `contentClipPath(const QRect&)`
- `paintPanel(QPainter&, const QRect&, const ThemeColors&, FluentBorderEffect*)`

Implementation notes:

- `panelRect()` slightly insets the rect before painting to reduce border jitter on HiDPI and translucent popup windows.
- `contentClipPath()` returns a rounded path matching the popup panel border, useful for clipping popup contents so they do not visually square off the corners.
- `paintPanel()` builds a `FluentFrameSpec` internally and injects accent-border / trace parameters automatically when a `FluentBorderEffect` is provided.
- These helpers are mainly reused by popup widgets such as `FluentCalendarPopup` to keep radius, border width, and open-animation tuning consistent.

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

