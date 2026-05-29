# 杂项与工具

本页主要覆盖“公开头文件里更偏底层/工具类”的部分。

Demo 页面：多数工具类被窗口/菜单/弹窗内部使用；可在 Windows 与 Overview 页面观察效果。

## FluentDiagnostics

include：`Fluent/FluentDiagnostics.h`

用途：为 Debug 调试阶段提供一个可选的 Qt warning 输出开关，避免已知高频 warning 把调试启动和主题切换拖慢。

关键 API：

- `Diagnostics::setKnownQtWarningSuppressionEnabled(true)`：只过滤库内已知的高频 Qt warning，例如无效字体尺寸、`QWidget::paintEngine` / `QPainter::begin` 这类噪声日志。
- `Diagnostics::setQtWarningOutputEnabled(false)`：关闭所有 `QtWarningMsg` 输出。这个开关更激进，建议只在本地 Debug 性能验证时临时使用。
- `Diagnostics::installMessageHandler()`：手动安装库提供的 message handler。上面两个 setter 会自动安装，一般不需要单独调用。

最小示例：

```cpp
#include <Fluent/FluentDiagnostics.h>

int main(int argc, char *argv[])
{
#ifdef QT_DEBUG
    Fluent::Diagnostics::setKnownQtWarningSuppressionEnabled(true);
    // 或者临时关闭所有 Qt warning：
    // Fluent::Diagnostics::setQtWarningOutputEnabled(false);
#endif

    QApplication app(argc, argv);
    ...
    return app.exec();
}
```

也可以通过环境变量启用：

- `QTFLUENT_SUPPRESS_KNOWN_WARNINGS=1`：过滤已知高频 warning。
- `QTFLUENT_DISABLE_QT_WARNINGS=1`：关闭所有 Qt warning 输出。
- `QTFLUENT_SUPPRESS_KNOWN_PAINT_WARNINGS=1`：兼容旧名称，只过滤已知绘制 warning。

注意：这只是调试体验开关，不应该替代真实问题修复。Release 性能仍应通过减少启动期控件创建、避免大表格 `setCellWidget` 和减少全局 stylesheet 刷新来保证。

---

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

## FluentMotion

include：`Fluent/FluentMotion.h`

用途：统一控件动效的语义 token，让 hover、press、focus、popup open、selection、collapse 等动画不再散落硬编码时长和曲线。

关键 API：

- `FluentMotion::duration(FluentMotionRole)`：按语义读取时长；当 `ThemeManager::animationsEnabled()` 为 false 时返回 0。
- `FluentMotion::configuredDuration(FluentMotionRole)`：读取用户配置的原始时长，不受全局动效开关影响。
- `FluentMotion::easing(FluentMotionRole)`：按语义读取 easing。
- `FluentMotion::configure(QVariantAnimation*, role)` / `configure(QPropertyAnimation*, role)`：给已有动画对象写入统一时长与曲线。
- `FluentMotion::setDuration(role, ms)`：单独修改某个语义动画的时长。
- `FluentMotion::setTokens(...)` / `resetTokens()`：整体替换或重置 motion token。
- `FluentMotion::popupSlideOffset()` / `pressOffset()`：读取 popup 位移和 press 位移 token。
- `ThemeManager::setAnimationsEnabled(bool)` / `animationsEnabled()`：全局动效开关。
- `ThemeManager::setMotionTokens(...)` / `motionTokens()`：应用级 motion token 配置入口。

当前已接入：

- `FluentButton` / `FluentToolButton` / `FluentAnimatedButton` 的 hover 与 press；主题或全局动效开关变化后会重新同步 token。
- `FluentLineEdit` 的 hover 与 focus；关闭全局动画后，下一次 hover/focus 会直接落到最终状态。
- `FluentToggleSwitch` / `FluentCheckBox` / `FluentRadioButton` / `FluentSlider` 的 hover、focus 与选中/位置过渡。
- `FluentProgressBar` / `FluentProgressRing` 的确定进度过渡；关闭全局动画时进度立即跳到目标值，`FluentProgressRing` 的 indeterminate 旋转会暂停。
- `FluentComboBox` / `FluentAutoSuggestBox` / `FluentMenu` / `FluentMenuBar` / `FluentFlyout` / `FluentCalendarPopup` / `FluentDatePicker` / `FluentTimePicker` 的 popup 打开动画；ComboBox/AutoSuggestBox 的列表 hover/selection 也会随 token 更新。
- `FluentCard` 的折叠动画，以及 wheel picker 的滚动吸附动画。
- `FluentNavigationView` 的 pane 宽度、hover 和 selection indicator；`FluentTabWidget` 的 hover/selection；DataViews 的 hover/selection；`FluentScrollBar` 的 reveal/hover；`FluentDateRangePicker` 的 hover；`FluentTeachingTip` 的蒙版淡入淡出；`FluentToast` 的出现/消失和队列移动。
- `FluentFlowLayout` 的几何重排动画；显式调用 `setAnimationDuration()` / `setAnimationEasing()` 仍会覆盖默认 token，但全局关闭动效时会即时落位。
- Demo 主窗口页面切换使用 `FluentMotionRole::Page`，动态页可以直接观察 Page token 的效果。

示例：

```cpp
auto *anim = new QVariantAnimation(this);
Fluent::FluentMotion::configure(anim, Fluent::FluentMotionRole::PopupOpen);

// 用户可按语义单独调节时长；已经迁移的控件会在 themeChanged 后同步。
Fluent::FluentMotion::setDuration(Fluent::FluentMotionRole::PopupOpen, 220);
Fluent::FluentMotion::setDuration(Fluent::FluentMotionRole::Selection, 120);
```

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

---

## FluentToolTip

include：`Fluent/FluentToolTip.h`

用途：统一替换 Qt 原生 tooltip 的 Fluent 风格提示气泡。

关键 API：

- `FluentToolTip::ensureInstalled()`：安装全局事件过滤器
- `FluentToolTip::showText(const QPoint&, const QString&, QWidget* anchor = nullptr, int msecDisplayTime = -1)`
- `FluentToolTip::hideText()`

实现语义：

- 内部会创建一个 `Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint` 的顶层浮窗，并使用 `paintFluentPanel()` 自绘圆角面板。
- 默认文本宽度会上限到约 `420px`，并根据文本长度自动估算显示时长（大约 0.9s + 每字符 55ms，最终 clamp 到 `1600..8000ms`）。
- 位置会基于鼠标全局坐标加一个偏移量（约 `+14,+22`），并自动 clamp 到屏幕可用区域；如果底部放不下，会翻到鼠标上方显示。
- 安装后会统一接管 `QEvent::ToolTip`：读取 widget 的 `toolTip()` / `toolTipDuration()`，并在 `Leave` / `MouseButtonPress` / `Wheel` / `KeyPress` / `FocusOut` / 应用失活时隐藏。

适用场景：

- 想让整个应用的 tooltip 视觉统一为 Fluent 风格
- 想手动在某个全局位置弹出一条临时说明

---

## FluentPopupSurface

include：`Fluent/FluentPopupSurface.h`

用途：为菜单、组合框、日历、时间等弹出层提供一套共享的“圆角面板常量 + 阴影外边距 + clip path + 面板绘制”工具。

说明：`FluentPopupSurface` 不是一个 widget 类，而是 `namespace Fluent::PopupSurface` 下的一组内联 helper。

关键内容：

- `kRadius` / `kBorderWidth` / `kShadowMargin*Px`
- `panelRect(const QRect&)`
- `withShadowMargins(const QSize&)` / `shadowContentRect(const QRect&)`
- `contentClipPath(const QRect&)`
- `paintPanel(...)` / `paintPanelWithShadowMargins(...)`

实现语义：

- `panelRect()` 会先把输入矩形按半像素向内收缩，减少描边在 HiDPI / 半透明窗口上的边缘抖动。
- `withShadowMargins()` 会把弹层内容尺寸扩展为“透明窗口尺寸”，给软件阴影留出绘制空间；内部实际内容区域通过 `shadowContentRect()` 取得。
- `contentClipPath()` 返回与 popup 面板边框一致的圆角 path，适合在 `WA_TranslucentBackground` 下裁剪内部内容，避免内容把圆角“顶成方形”。
- `paintPanelWithShadowMargins()` 会先绘制统一软阴影，再绘制 surface / border / trace；适合顶层透明 popup。
- `paintPanel()` 仍可用于没有阴影外边距的嵌入式面板。两者都会构造 `FluentFrameSpec`，并在传入 `FluentBorderEffect` 时自动注入 accent 描边 / trace 参数。
- popup 出现动画的时长、曲线和位移由 `FluentMotionRole::PopupOpen` / `FluentMotion::popupSlideOffset()` 统一管理；`FluentPopupSurface` 只负责 surface 形态与绘制。
- 该组 helper 主要被 `FluentMenu`、`FluentComboBox`、`FluentCalendarPopup`、`FluentDatePicker` / `FluentTimePicker` 等弹出层复用，用来保证不同 popup 的半径、阴影和描边宽度保持一致。

---

## FluentQtCompat

include：`Fluent/FluentQtCompat.h`

用途：为 Qt5 / Qt6 提供一层很薄的事件 / 坐标兼容封装，减少组件实现里到处写版本分支。

说明：这是一个**偏内部实现的公开头文件**，普通业务代码通常不需要直接包含；它主要服务于库内部的自绘控件与事件处理逻辑。

关键内容：

- `using FluentEnterEvent = ...`：在 Qt6 下别名到 `QEnterEvent`，Qt5 下退化为 `QEvent`
- `mousePositionF(const QMouseEvent*)`
- `globalMousePosition(const QMouseEvent*)`
- `wheelPositionF(const QWheelEvent*)`

实现语义：

- Qt6 使用 `event->position()` / `event->globalPosition()`；Qt5 则回退到 `localPos()` / `globalPos()` / `posF()`。
- 这样上层控件（例如 `FluentDial` 等）可以统一写一套坐标读取逻辑，而不需要在每个 `.cpp` 中重复 `#if QT_VERSION` 分支。

