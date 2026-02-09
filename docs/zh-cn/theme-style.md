# 主题 / 样式

本库的大部分控件依赖统一的主题与绘制基元：

- `ThemeManager`：切换浅色/深色、更新全局 `ThemeColors`，并广播 `themeChanged()`。
- `Theme`：生成基础 QSS 片段（按钮、输入框、菜单等的样式字符串）。
- `Style`：提供统一 metrics（圆角、padding、标题栏高度等）与绘制方法（surface/border/trace）。

对应公开头文件：

- `Fluent/FluentTheme.h`
- `Fluent/FluentStyle.h`

Demo 页面：Overview（`demo/pages/PageOverview.cpp`）与 Windows（`demo/pages/PageWindows.cpp`）。

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
- `themeChanged()`：主题变化信号。

实现语义补充：

- `themeChanged()` 会被“合并触发”：多次 `setColors()` / `setThemeMode()` / `setAccentBorderEnabled()` 会在同一轮事件循环中合并为一次信号（使用 `QTimer::singleShot(0, ...)`），避免频繁刷新卡顿。
- `setThemeMode()` 切换 Light/Dark 时会保留当前 `accent`（更符合 Fluent 习惯），并将 `focus` 设置为 `accent.lighter(135)`。

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
```

也可以直接修改色板：

```cpp
auto colors = ThemeManager::instance().colors();
colors.accent = QColor("#2D7DFF");
ThemeManager::instance().setColors(colors);
```

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
- `Style::windowMetrics()` / `Style::setWindowMetrics(...)`：标题栏高度、窗口按钮尺寸、accent trace 动画参数等。
- `Style::roundedRectPath(...)` / `paintTraceBorder(...)` / `paintElevationShadow(...)`：用于窗口/弹窗/卡片等自绘。

实现语义补充：

- `Style::setWindowMetrics()` 会对 trace 相关参数做防御性 clamp（例如 duration 范围 1..60000ms、overshoot 上限 0.25、overshootAt 上限 0.99），避免 Demo/外部输入把动画参数设置到不可用区间。
- `Style::paintTraceBorder()` 支持轻微“越界脉冲”：当 `progress > 1`（rawProgress overshoot）时，会在绘制完整描边的同时略微增粗并提高 alpha（overshoot 最大按 0.10 截断），用来做更有“Fluent 味”的启用动画收尾。

## Theme::baseStyleSheet（全局 QSS）

`Theme::baseStyleSheet(colors)` 生成的是“全局底座样式”，通常由窗口层（例如 `FluentMainWindow`）在应用级别设置到 `qApp->setStyleSheet(...)`。它包含：

- 全局字体（`Segoe UI`/`Microsoft YaHei UI` 等）与默认字号（14px）
- window 背景色（`QWidget:window, QMainWindow, QDialog`）
- Win11-like overlay 滚动条（仅在 `QAbstractScrollArea:hover` 时显示 handle 更明显）
- `QToolTip` 外观
- `QLabel#FluentLink` 的链接色

## 相关头文件

- `Fluent/FluentTheme.h`
- `Fluent/FluentStyle.h`

相关（用于窗口/弹窗强调边框效果）：

- `Fluent/FluentBorderEffect.h`
- `Fluent/FluentAccentBorderTrace.h`
- `Fluent/FluentFramePainter.h`
