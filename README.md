# QtFluentWidgets

一套基于 **Qt Widgets** 的 Fluent Design 风格控件库（含 Demo 与 Qt Designer 插件），提供统一的主题/色板管理与一组常用控件。

- 语言 / Languages:
	- 中文：README.md（默认） / [README.zh-CN.md](README.zh-CN.md)
	- English: [README.en-US.md](README.en-US.md)
- Ask zread for easy use:
  
  - [![zread](https://img.shields.io/badge/Ask_Zread-_.svg?style=flat&color=00b0aa&labelColor=000000&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB3aWR0aD0iMTYiIGhlaWdodD0iMTYiIHZpZXdCb3g9IjAgMCAxNiAxNiIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj4KPHBhdGggZD0iTTQuOTYxNTYgMS42MDAxSDIuMjQxNTZDMS44ODgxIDEuNjAwMSAxLjYwMTU2IDEuODg2NjQgMS42MDE1NiAyLjI0MDFWNC45NjAxQzEuNjAxNTYgNS4zMTM1NiAxLjg4ODEgNS42MDAxIDIuMjQxNTYgNS42MDAxSDQuOTYxNTZDNS4zMTUwMiA1LjYwMDEgNS42MDE1NiA1LjMxMzU2IDUuNjAxNTYgNC45NjAxVjIuMjQwMUM1LjYwMTU2IDEuODg2NjQgNS4zMTUwMiAxLjYwMDEgNC45NjE1NiAxLjYwMDFaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik00Ljk2MTU2IDEwLjM5OTlIMi4yNDE1NkMxLjg4ODEgMTAuMzk5OSAxLjYwMTU2IDEwLjY4NjQgMS42MDE1NiAxMS4wMzk5VjEzLjc1OTlDMS42MDE1NiAxNC4xMTM0IDEuODg4MSAxNC4zOTk5IDIuMjQxNTYgMTQuMzk5OUg0Ljk2MTU2QzUuMzE1MDIgMTQuMzk5OSA1LjYwMTU2IDE0LjExMzQgNS42MDE1NiAxMy43NTk5VjExLjAzOTlDNS42MDE1NiAxMC42ODY0IDUuMzE1MDIgMTAuMzk5OSA0Ljk2MTU2IDEwLjM5OTlaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik0xMy43NTg0IDEuNjAwMUgxMS4wMzg0QzEwLjY4NSAxLjYwMDEgMTAuMzk4NCAxLjg4NjY0IDEwLjM5ODQgMi4yNDAxVjQuOTYwMUMxMC4zOTg0IDUuMzEzNTYgMTAuNjg1IDUuNjAwMSAxMS4wMzg0IDUuNjAwMUgxMy43NTg0QzE0LjExMTkgNS42MDAxIDE0LjM5ODQgNS4zMTM1NiAxNC4zOTg0IDQuOTYwMVYyLjI0MDFDMTQuMzk4NCAxLjg4NjY0IDE0LjExMTkgMS42MDAxIDEzLjc1ODQgMS42MDAxWiIgZmlsbD0iI2ZmZiIvPgo8cGF0aCBkPSJNNCAxMkwxMiA0TDQgMTJaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik00IDEyTDEyIDQiIHN0cm9rZT0iI2ZmZiIgc3Ryb2tlLXdpZHRoPSIxLjUiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIvPgo8L3N2Zz4K&logoColor=ffffff)](https://zread.ai/Type3limit/QtFluentWidgets)

## 预览

### 浅色模式

| 总览 | 按钮 / 开关 | 容器 / 布局 |
| --- | --- | --- |
| ![QtFluentWidgets 浅色总览](img/preview-light-overview.png) | ![QtFluentWidgets 浅色按钮](img/preview-light-buttons.png) | ![QtFluentWidgets 浅色容器](img/preview-light-containers.png) |

### 深色模式

| 总览 | 按钮 / 开关 | 容器 / 布局 |
| --- | --- | --- |
| ![QtFluentWidgets 深色总览](img/preview-dark-overview.png) | ![QtFluentWidgets 深色按钮](img/preview-dark-buttons.png) | ![QtFluentWidgets 深色容器](img/preview-dark-containers.png) |

## 特性

- Qt5 / Qt6 兼容（CMake 自动探测 Qt6，否则使用 Qt5）。
- 统一主题与色板：`ThemeManager` + `ThemeColors`。
- Fluent 风格输入控件面板：通过 `Style::paintControlSurface()` 统一圆角/边框/focus ring。
- 内置 `FluentLottieWidget` / `FluentAnimatedIcon` / `FluentAnimatedButton`，通过 `third_party/rlottie` 子模块提供 Lottie JSON 渲染与 marker 状态动画。
- 新增 `FluentProgressRing`、`FluentInfoBar`、`FluentAutoSuggestBox` / `FluentSearchBox`、`FluentFlyout` / `FluentTeachingTip`、`FluentCommandBar` / `FluentDropDownButton` / `FluentSplitButton` 等 WinUI Gallery 常见控件。
- 内置日期范围选择器 `FluentDateRangePicker`，支持双面板范围选择与可配置 prefix / suffix / separator。
- 窗口层支持 `FluentMainWindow` 自定义标题栏插槽、Windows 11 风格 accent 描边 / trace 动画，以及菜单栏嵌入标题栏。
- Demo 覆盖所有控件，并带侧边栏联动配置（含 CodeEditor 的 clang-format 路径配置）。
- 可选 Qt Designer 插件（默认开启构建）。

## 快速开始

### 依赖

- CMake >= 3.16
- Qt Widgets（Qt5 或 Qt6）
- Qt Svg、Qt UiPlugin（用于 Demo / Designer 插件）
- rlottie 子模块（见下面“拉取代码”）

### 拉取代码（包含子模块）

如果你还没有 clone 仓库，推荐直接用 `--recursive`，这样会把 `third_party/rlottie` 一起拉下来：

```bash
git clone --recursive https://github.com/Type3limit/QtFluentWidgets.git
cd QtFluentWidgets
```

如果你已经用普通 `git clone` 拉过仓库，也没关系，在仓库根目录补执行一次：

```bash
git submodule update --init --recursive
```

看到 `third_party/rlottie` 目录里有源码文件后，再继续执行 CMake。这个步骤通常只需要第一次配置项目时做一次；以后更新仓库后如果子模块有变化，再执行同一条命令即可。

### 构建（推荐 out-of-source）

```bash
cmake -S . -B build
cmake --build build --config Release
```

如果你的 Qt 没有在默认搜索路径中，可传入 `CMAKE_PREFIX_PATH`（示例：Windows）：

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.6.0/msvc2019_64"
cmake --build build --config Release
```

### 运行 Demo

- CMake target：`QtFluentDemo`
- 也可以使用仓库提供的自定义 target：`run-QtFluentDemo`（会设置少量 demo 用的 env）
- 视觉回归基线：`render-QtFluentDemoVisualBaselines` 使用 Qt `QWidget::render()` 离屏生成 `demo-visual-baselines/`，默认覆盖 100% / 150% 缩放、light / dark / accent 与 Demo 主页面状态矩阵。
- 离屏交互烟测：`smoke-QtFluentDemoInteractions` 使用合成点击 + `QWidget::render()` 生成 `demo-interaction-smoke/`，不打开可见窗口。

## 在你的项目中使用

### 方式 A：add_subdirectory（最简单）

```cmake
add_subdirectory(path/to/QtFluent)

target_link_libraries(your_app PRIVATE QtFluentWidgets)
```

然后在代码里包含头文件（均位于 `include/Fluent/`）：

```cpp
#include "Fluent/FluentButton.h"
#include "Fluent/FluentDateRangePicker.h"
#include "Fluent/FluentTheme.h"

using namespace Fluent;

auto *rangePicker = new FluentDateRangePicker();
rangePicker->setDateRange(QDate::currentDate(), QDate::currentDate().addDays(7));
rangePicker->setStartPrefix(QStringLiteral("开始："));
rangePicker->setSeparator(QStringLiteral("  至  "));
rangePicker->setEndPrefix(QStringLiteral("结束："));
```

### 方式 B：作为源码依赖

直接把 `include/`、`src/` 和 `CMakeLists.txt` 作为子模块/源码依赖引入即可。

## 文档（按模块拆分）

为了便于查阅，控件使用说明按模块拆分到 `docs/`，由本 README 跳转：

- 主题 / 样式： [docs/zh-cn/theme-style.md](docs/zh-cn/theme-style.md)
- 图标： [docs/zh-cn/icons.md](docs/zh-cn/icons.md)
- 按钮与开关： [docs/zh-cn/buttons.md](docs/zh-cn/buttons.md)
- Lottie 动效： [docs/zh-cn/lottie.md](docs/zh-cn/lottie.md)
- 输入与编辑： [docs/zh-cn/inputs.md](docs/zh-cn/inputs.md)
- 代码编辑器： [docs/zh-cn/code-editor.md](docs/zh-cn/code-editor.md)
- 选择器： [docs/zh-cn/pickers.md](docs/zh-cn/pickers.md)
- 数据视图： [docs/zh-cn/data-views.md](docs/zh-cn/data-views.md)
- 容器 / 布局： [docs/zh-cn/containers-layout.md](docs/zh-cn/containers-layout.md)
- NavigationView： [docs/zh-cn/navigation-view.md](docs/zh-cn/navigation-view.md)
- 窗口 / 菜单 / 对话框： [docs/zh-cn/windows-dialogs.md](docs/zh-cn/windows-dialogs.md)
- Qt Designer 兼容性测试： [docs/zh-cn/designer-compat-test.md](docs/zh-cn/designer-compat-test.md)
- 杂项与工具： [docs/zh-cn/utilities.md](docs/zh-cn/utilities.md)

## Qt Designer 插件

默认会构建 Designer 插件，输出到构建目录的 `designer/` 下。将生成的插件文件复制到 Qt Designer 的插件目录即可加载。

关闭插件构建：

```bash
cmake -S . -B build -DFLUENT_BUILD_DESIGNER_PLUGIN=OFF
cmake --build build
```

## Demo 导航（与模块一致）

Demo 的页面分组与文档模块基本一致：输入 / 按钮 / 动态 / 角度控件 / 选择器 / 数据视图 / 容器 / 窗口。

---

## Star History

<a href="https://www.star-history.com/?repos=Type3limit%2FQtFluentWidgets&type=date&legend=top-left">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/chart?repos=Type3limit/QtFluentWidgets&type=date&theme=dark&legend=top-left" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/chart?repos=Type3limit/QtFluentWidgets&type=date&legend=top-left" />
   <img alt="Star History Chart" src="https://api.star-history.com/chart?repos=Type3limit/QtFluentWidgets&type=date&legend=top-left" />
 </picture>
</a>
