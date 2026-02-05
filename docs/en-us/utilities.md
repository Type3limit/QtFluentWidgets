# Utilities

This page covers lower-level helpers that are public but mostly used internally or for advanced customization.

Demo: you can observe their effects on the Windows and Overview pages (menus/dialogs/toasts use them internally).

## FluentAccentBorderTrace

include: `Fluent/FluentAccentBorderTrace.h`

Purpose: shared controller for the Win11-like accent border trace animation (no moc required; uses `QVariantAnimation`).

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

Key APIs:

- `syncFromTheme()` / `onThemeChanged()`
- `playInitialTraceOnce(int delayMs = 0)`
- `applyToFrameSpec(FluentFrameSpec&, const ThemeColors&, ...)`

---

## FluentFramePainter

include: `Fluent/FluentFramePainter.h`

Purpose: common rounded frame/panel painter for translucent top-level widgets.

Key APIs:

- `FluentFrameSpec`
- `paintFluentFrame(...)`
- `paintFluentPanel(...)`
