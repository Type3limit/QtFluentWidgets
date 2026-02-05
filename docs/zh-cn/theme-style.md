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

## 相关头文件

- `Fluent/FluentTheme.h`
- `Fluent/FluentStyle.h`

相关（用于窗口/弹窗强调边框效果）：

- `Fluent/FluentBorderEffect.h`
- `Fluent/FluentAccentBorderTrace.h`
- `Fluent/FluentFramePainter.h`
