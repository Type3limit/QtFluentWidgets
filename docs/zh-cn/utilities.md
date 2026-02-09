# 杂项与工具

本页主要覆盖“公开头文件里更偏底层/工具类”的部分。

Demo 页面：多数工具类被窗口/菜单/弹窗内部使用；可在 Windows 与 Overview 页面观察效果。

## FluentAccentBorderTrace

include：`Fluent/FluentAccentBorderTrace.h`

用途：Win11 风格强调边框 trace 动画的共享控制器（不依赖 moc，内部用 `QVariantAnimation` 驱动）。

实现语义：

- “关闭描边”（toEnabled=false）时不会播放关闭动画：会直接 `stop()`，`t()` 变为 -1，并请求刷新。
- “开启描边”（toEnabled=true）时使用 `Style::windowMetrics()` 里的 trace 参数驱动动画：
	- `accentBorderTraceDurationMs`
	- `accentBorderTraceEnableEasing` / `accentBorderTraceDisableEasing`
	- 可选 overshoot：在 `accentBorderTraceEnableOvershootAt` 时刻把进度推到 `1 + accentBorderTraceEnableOvershoot`，用于更“Fluent”的收尾脉冲。
- `setRequestUpdate(...)` 可把刷新从 `updateTarget->update()` 替换成自定义逻辑（例如同时刷新多个 overlay）。

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

实现语义：

- `syncFromTheme()` / `onThemeChanged()` 只会读取 `ThemeManager::accentBorderEnabled()` 并驱动 `FluentAccentBorderTrace`。
- `playInitialTraceOnce()` 每个实例只会播放一次（适合弹窗 show 时的“trace-in”）；如需重复播放可 `resetInitial()`。
- `applyToFrameSpec(...)` 的注入策略：
	- 非动画态：`frame.borderColorOverride` 直接取“当前启用态”的边框色（accent 或 normal）。
	- 动画态：`frame.borderColorOverride` 取 from 状态边框色，`frame.traceEnabled=true`，`frame.traceColor` 取 to 状态边框色，`frame.traceT` 为当前 `t()`。

关键 API：

- `syncFromTheme()` / `onThemeChanged()`：跟随 `ThemeManager::accentBorderEnabled()`。
- `playInitialTraceOnce(int delayMs = 0)`：弹窗出现时播放一次 trace-in。
- `applyToFrameSpec(FluentFrameSpec&, const ThemeColors&, ...)`：把边框/trace 字段注入绘制 spec。

Demo：用于 Menu / Dialog / MessageBox / Toast / MainWindow 等。

---

## FluentFramePainter

include：`Fluent/FluentFramePainter.h`

用途：提供通用的圆角窗口/面板绘制（surface + 1px border + 可选 trace）。

实现语义：

- `paintFluentFrame(...)` 面向“顶层半透明窗口”：当 `FluentFrameSpec::clearToTransparent=true` 时，会先把整个窗口矩形清为透明，避免圆角边缘残影。
- `paintFluentPanel(...)` 面向“在更大透明窗口中的面板区域”：不会主动清理整窗，只按给定 `panelRect` 绘制。

## 最小接入示例（自绘面板 + 强调边框）

下面示例演示如何在自定义控件中接入强调边框与 trace（思路与 Dialog/Menu/MessageBox/Toast 类似）：

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
		spec.clearToTransparent = false; // 普通控件通常不需要整窗透明清理

		const auto colors = Fluent::ThemeManager::instance().colors();
		m_border.applyToFrameSpec(spec, colors);
		Fluent::paintFluentPanel(p, QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), colors, spec);
	}

private:
	Fluent::FluentBorderEffect m_border;
};
```

关键 API：

- `FluentFrameSpec`：描述 radius、surface/border override、trace 参数等。
- `paintFluentFrame(QPainter&, QRect, ThemeColors, FluentFrameSpec)`：绘制顶层窗口 frame。
- `paintFluentPanel(QPainter&, QRectF, ThemeColors, FluentFrameSpec)`：绘制任意矩形面板。

Demo：被 Dialog / MessageBox / Menu 等使用。
