# 杂项与工具

本页主要覆盖“公开头文件里更偏底层/工具类”的部分。

Demo 页面：多数工具类被窗口/菜单/弹窗内部使用；可在 Windows 与 Overview 页面观察效果。

## FluentAccentBorderTrace

include：`Fluent/FluentAccentBorderTrace.h`

用途：Win11 风格强调边框 trace 动画的共享控制器（不依赖 moc，内部用 `QVariantAnimation` 驱动）。

关键 API：

- `setCurrentEnabled(bool)` / `currentEnabled()`：同步当前开关状态。
- `onEnabledChanged(bool)`：在 enabled 状态变化时播放动画。
- `play(bool fromEnabled, bool toEnabled)` / `stop()`
- `isAnimating()` / `t()`：动画进度。
- `setRequestUpdate(std::function<void()>)`：自定义刷新回调。

Demo：被 `FluentBorderEffect` 以及多个窗口/弹窗类内部使用。

---

## FluentBorderEffect

include：`Fluent/FluentBorderEffect.h`

用途：更易用的“强调边框 + trace”封装，便于在任意 `QWidget` 上快速接入边框强调。

关键 API：

- `syncFromTheme()` / `onThemeChanged()`：跟随 `ThemeManager::accentBorderEnabled()`。
- `playInitialTraceOnce(int delayMs = 0)`：弹窗出现时播放一次 trace-in。
- `applyToFrameSpec(FluentFrameSpec&, const ThemeColors&, ...)`：把边框/trace 字段注入绘制 spec。

Demo：用于 Menu / Dialog / MessageBox / Toast / MainWindow 等。

---

## FluentFramePainter

include：`Fluent/FluentFramePainter.h`

用途：提供通用的圆角窗口/面板绘制（surface + 1px border + 可选 trace）。

关键 API：

- `FluentFrameSpec`：描述 radius、surface/border override、trace 参数等。
- `paintFluentFrame(QPainter&, QRect, ThemeColors, FluentFrameSpec)`：绘制顶层窗口 frame。
- `paintFluentPanel(QPainter&, QRectF, ThemeColors, FluentFrameSpec)`：绘制任意矩形面板。

Demo：被 Dialog / MessageBox / Menu 等使用。
