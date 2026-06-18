# 主题 / 样式

本库的大部分控件依赖统一的主题与绘制基元：

- `ThemeManager`：切换浅色/深色、更新全局 `ThemeColors`，并广播 `themeChanged()`。
- `Theme`：生成基础 QSS 片段（按钮、输入框、菜单等的样式字符串）。
- `Style`：提供统一 metrics（圆角、padding、标题栏高度等）与绘制方法（surface/border/trace）。
- `FluentMotion`：提供 hover/press/popup/collapse/selection 等语义动画 token。

对应公开头文件：

- `Fluent/FluentTheme.h`
- `Fluent/FluentStyle.h`
- `Fluent/FluentMotion.h`

Demo 页面：Overview（`demo/pages/PageOverview.cpp`）、Motion（`demo/pages/PageMotion.cpp`）与 Windows（`demo/pages/PageWindows.cpp`）。Motion 页顶部提供 Motion Role Matrix，可横向检查各语义动效角色的 configured/effective duration 与 reduced-motion 结果。

## 主题切换

```cpp
#include "Fluent/FluentTheme.h"

using namespace Fluent;

ThemeManager::instance().setLightMode();
ThemeManager::instance().setDarkMode();
```

关键 API：

- `ThemeManager::instance()`：全局单例。
- `setThemeMode(ThemeMode)` / `themeMode()`
- `setColors(const ThemeColors&)` / `colors()`
- `setAnimationsEnabled(bool)` / `animationsEnabled()`
- `themeChanged()`：主题变化信号。

实现语义补充：

- `themeChanged()` 会被“合并触发”：多次 `setColors()` / `setThemeMode()` / `setAccentBorderEnabled()` 会在同一轮事件循环中合并为一次信号（使用 `QTimer::singleShot(0, ...)`），避免频繁刷新卡顿。
- `setThemeMode()` 切换 Light/Dark 时会保留当前 `accent`（更符合 Fluent 习惯），并将 `focus` 设置为 `accent.lighter(135)`。
- `animationsEnabled()` 会影响使用 `FluentMotion` 的新增/已迁移动画：关闭时对应语义 duration 为 0，popup、折叠和按钮状态反馈会尽量即时完成。

## ThemeColors 色板字段

`ThemeColors` 是全库共享的“语义色”，控件通常只依赖这些字段而不是硬编码颜色：

- `accent`：强调色（按钮主色、焦点、选择等）
- `text` / `subText` / `disabledText`：主文本/次文本/禁用文本
- `background`：窗口背景（`QWidget:window` / `QMainWindow` / `QDialog`）
- `surface`：卡片/输入框等表面色
- `border`：描边
- `hover` / `pressed` / `focus`：交互态色（hover/pressed/focus）
- `error`：错误态

可选强调边框（accent border）：

```cpp
ThemeManager::instance().setAccentBorderEnabled(true);

// 描边样式：Solid（默认，Win11 风格）或 Flow（旋转的 accent 锥形渐变流光，
// 仅在 accentBorderEnabled 为真时生效）。
ThemeManager::instance().setAccentBorderStyle(ThemeManager::AccentBorderStyle::Flow);
```

流光渐变色板（Flow 描边与 `FluentCard` 流光背景共用）：

```cpp
// 自定义色标；传空列表则从当前 accent 自动推导。
ThemeManager::instance().setFlowGradientColors({QColor("#2563EB"), QColor("#8A46D8"), QColor("#0F9B8E")});
// resolvedFlowColors() 返回实际用于绘制的色标（自定义或 accent 推导）。
```

实现语义补充：

- `accentBorderEnabled()` 不只是一个布尔开关；窗口层组件（`FluentMainWindow` / `FluentDialog` / `FluentMessageBox` / `FluentToast`）会通过 `FluentBorderEffect` 把它映射到“普通描边 ↔ accent 描边”的状态切换，并在启用时播放 trace-in 动画。
- `FluentMainWindow` 当前会把描边画在一个独立的顶层 overlay 上，因此描边可以同时包裹标题栏与 central widget，且不会被不透明内容控件覆盖。
- **Flow** 样式用 `resolvedFlowColors()` 的 `QConicalGradient` 描边并旋转动画；窗口失焦/最小化/隐藏或 `animationsEnabled()` 为 false 时自动暂停（不在后台空转，也不影响离屏渲染）。

也可以直接修改色板：

```cpp
auto colors = ThemeManager::instance().colors();
colors.accent = QColor("#2D7DFF");
ThemeManager::instance().setColors(colors);
```

## Token ramps 与 Surface / Elevation

`FluentFramePainter.h` 提供 `FluentSurfaceSpec` 与 `paintFluentSurface()`，用于把 Background / Pane / Card / Raised / Popup / Modal 等层级映射到统一的填充、描边和轻量阴影。

Demo 的 Theme 面板包含 Token ramps 与 Surface / Elevation 预览：调整 Background、Surface、Accent 或切换深浅色时，可以直接观察 accent ramp、neutral ramp、radius token，以及各 surface/elevation 层级的分离度。

## 自绘输入面板（统一风格）

如需自定义控件并保持与输入控件一致，可使用：

```cpp
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

QPainter p(this);
Style::paintControlSurface(p, QRectF(rect()), ThemeManager::instance().colors(), hover, focus, isEnabled(), pressed);
```

关键 API：

- `Style::metrics()`：控件默认尺寸/圆角/padding。
- `Style::paintControlSurface()` 的 focus stroke 使用当前 `accent.base` token；若传入旧 `ThemeColors::focus` 自定义色，也不会绕过 token ramp。
- `Style::windowMetrics()` / `Style::setWindowMetrics(...)`：标题栏高度、窗口按钮尺寸、accent trace 动画参数等。
- `Style::roundedRectPath(...)` / `paintTraceBorder(...)` / `paintElevationShadow(...)`：用于窗口/弹窗/卡片等自绘。

实现语义补充：

- `Style::setWindowMetrics()` 会对 trace 相关参数做防御性 clamp（例如 duration 范围 1..60000ms、overshoot 上限 0.25、overshootAt 上限 0.99），避免 Demo/外部输入把动画参数设置到不可用区间。
- `Style::paintTraceBorder()` 支持轻微“越界脉冲”：当 `progress > 1`（rawProgress overshoot）时，会在绘制完整描边的同时略微增粗并提高 alpha（overshoot 最大按 0.10 截断），用来做更有“Fluent 味”的启用动画收尾。
- `Style::windowMetrics()` 里的窗口级参数现在也直接影响 `FluentMainWindow`：例如 `titleBarHeight` / `windowButtonWidth` / `resizeBorder` 会影响标题栏布局与 `WM_NCHITTEST`，而 trace 参数会影响外层 accent 描边动画节奏。

## Motion token

`FluentMotion` 是当前质感升级中新增的动效语义层。控件应优先使用 `FluentMotionRole`，而不是直接写 `setDuration(150)`：

```cpp
#include "Fluent/FluentMotion.h"

auto *anim = new QVariantAnimation(this);
Fluent::FluentMotion::configure(anim, Fluent::FluentMotionRole::Hover);

// 应用可按语义覆盖各类动画时长。
Fluent::FluentMotion::setDuration(Fluent::FluentMotionRole::Hover, 90);
Fluent::FluentMotion::setDuration(Fluent::FluentMotionRole::PopupOpen, 220);

// 也可以整体替换 token，例如做“更轻快”或“更稳重”的动效配置。
auto tokens = Fluent::ThemeManager::instance().motionTokens();
tokens.selectionDuration = 120;
tokens.collapseDuration = 240;
Fluent::ThemeManager::instance().setMotionTokens(tokens);
```

当前常用语义：

- `Hover` / `Press` / `Focus`
- `PopupOpen` / `PopupClose`
- `Collapse`
- `Selection`
- `Navigation` / `Page`
- `Layout`
- `Toast`
- `WheelSnap`

配置说明：

- `FluentMotion::configuredDuration(role)` 返回用户配置的原始时长。
- `FluentMotion::duration(role)` 返回实际生效时长；当全局动效关闭时为 0。
- `ThemeManager::setMotionTokens(...)` 会保留在浅色/深色/颜色切换之后，不会因主题刷新回到默认值。

Reduced motion：

- `ThemeManager::setAnimationsEnabled(false)` 会让 `FluentMotion::duration(...)` 返回 0。
- 已迁移控件会在下一次状态变化时即时完成动画；例如 Button/ToolButton/输入控件族/ComboBox 的 hover/focus 直接落到最终状态，popup 直接显示到最终几何，Toggle/Check/Radio/Slider/Progress 直接跳到目标状态。
- 容器、导航、弹层和 Lottie 控件也会跟随该开关：NavigationView 的 pane/selection、TabWidget 的切换指示器、DataViews hover、ScrollBar reveal/hover、picker hover/focus、TeachingTip mask、Toast opacity 与队列移动、Lottie 片段/状态过渡和 Demo Page transition 都会直接完成或静态展示。
- 对于 `FluentProgressRing` 的 indeterminate 状态，关闭全局动画会暂停旋转，只保留静态进度/弧段展示。

## Theme::baseStyleSheet（全局 QSS）

`Theme::baseStyleSheet(colors)` 生成的是“全局底座样式”，通常由窗口层（例如 `FluentMainWindow`）在应用级别设置到 `qApp->setStyleSheet(...)`。它包含：

- 全局字体（`Segoe UI`/`Microsoft YaHei UI` 等）与默认字号（14px）
- window 背景色（`QWidget:window, QMainWindow, QDialog`）
- Win11-like overlay 滚动条（仅在 `QAbstractScrollArea:hover` 时显示 handle 更明显），handle 颜色由 `neutral.strokeStrong` 与文本 token 派生，不再使用固定灰色 `rgba(...)`
- `QToolTip` 外观，背景与描边分别由 `neutral.layer` / `neutral.strokeSubtle` 和 `accent.base` 轻量混合得到
- `QLabel#FluentLink` 的链接色

补充说明：

- `Theme::baseStyleSheet(...)` 负责的是“全局底座外观”，并不直接承担 `FluentMainWindow` 外层描边的绘制；主窗口的 accent 描边与 trace 动画是自绘的。
- `FluentMainWindow` 同步 `QApplication` palette 时也会使用同一套 neutral/accent token（Base、AlternateBase、ToolTipBase、Mid、Highlight 等），避免 native/fallback 路径回到旧 `surface/border/accent` 直取。
- `FluentMainWindow` 的应用级 QSS / palette 缓存签名会跟踪实际派生出的 token 色；即使只调整 `border` / `pressed` / `error`，tooltip、scrollbar 和 native palette 角色也会刷新到新的 `strokeSubtle` / `fillTertiary` / semantic token。
- `Theme::*Style(...)` 的控件级 fallback QSS 也遵循同一套 token：disabled surface 向 `neutral.background` 收敛，pressed 使用 `neutral.fillTertiary`，CheckBox fallback checkmark 使用随 `onAccent` 切换的库内 SVG，RadioButton fallback checked 态使用 token 化径向渐变绘制 accent 内点与 neutral 圆面，ProgressBar disabled chunk 使用 muted `accent.base`，slider/dialog 的暗色提亮使用当前 text token，不再直接混旧 `hover` / `pressed`、平台原生图片或固定白色。
- 因为 `FluentMainWindow` 会在主题变化时做“相等判定”后再决定是否重新设置 `qApp->setStyleSheet(...)`，所以一般不会因为重复 themeChanged 而触发不必要的全量 repolish。

## 相关头文件

- `Fluent/FluentTheme.h`
- `Fluent/FluentStyle.h`

相关（用于窗口/弹窗强调边框效果）：

- `Fluent/FluentBorderEffect.h`
- `Fluent/FluentAccentBorderTrace.h`
- `Fluent/FluentFramePainter.h`
