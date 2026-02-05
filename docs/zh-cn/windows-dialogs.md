# 窗口 / 菜单 / 对话框

## 控件清单

- `FluentMainWindow`（include: `Fluent/FluentMainWindow.h`）
- `FluentMenuBar`（include: `Fluent/FluentMenuBar.h`）
- `FluentMenu`（include: `Fluent/FluentMenu.h`）
- `FluentToolBar`（include: `Fluent/FluentToolBar.h`）
- `FluentStatusBar`（include: `Fluent/FluentStatusBar.h`）
- `FluentDialog`（include: `Fluent/FluentDialog.h`）
- `FluentMessageBox`（include: `Fluent/FluentMessageBox.h`）
- `FluentToast`（include: `Fluent/FluentToast.h`）
- `FluentResizeHelper`（include: `Fluent/FluentResizeHelper.h`）

Demo 页面：Windows（`demo/pages/PageWindows.cpp`）与 Overview（`demo/pages/PageOverview.cpp`）。

## FluentMainWindow

```cpp
#include "Fluent/FluentMainWindow.h"

class MainWindow : public Fluent::FluentMainWindow {
public:
    using Fluent::FluentMainWindow::FluentMainWindow;
};
```

用途：带 Fluent 标题栏的主窗口（MenuBar 嵌入标题栏，支持可选无边框 resize）。

关键 API：

- `setFluentTitleBarEnabled(bool)`：启用/禁用 Fluent 标题栏。
- `setFluentWindowButtons(WindowButtons)`：配置最小化/最大化/关闭。
- 标题栏内容：
    - `setFluentTitleBarTitle()` / `clearFluentTitleBarTitle()`
    - `setFluentTitleBarIcon()` / `clearFluentTitleBarIcon()`
    - `setFluentTitleBarCenterWidget(QWidget*)`
    - `setFluentTitleBarLeftWidget(QWidget*)` / `setFluentTitleBarRightWidget(QWidget*)`
- MenuBar：`setFluentMenuBar(FluentMenuBar*)` / `fluentMenuBar()`
- Resize：`setFluentResizeEnabled(bool)`、`setFluentResizeBorderWidth(int)`

Demo：Windows / Overview（Demo 主窗口也基于此）。

## MenuBar / Menu

```cpp
#include "Fluent/FluentMenuBar.h"
#include "Fluent/FluentMenu.h"

auto *menuBar = new Fluent::FluentMenuBar();
auto *fileMenu = menuBar->addFluentMenu(QStringLiteral("文件"));
fileMenu->addAction(QStringLiteral("退出"));
window->setFluentMenuBar(menuBar);
```

用途：Fluent 风格菜单栏/菜单（含 hover 高亮与弹出动画，子菜单会保持 Fluent 风格）。

关键 API：

- `FluentMenuBar::addFluentMenu(const QString&) -> FluentMenu*`
- `FluentMenu::addFluentMenu(const QString&) -> FluentMenu*`（创建 Fluent 子菜单）

Demo：Windows / Overview。

---

## FluentToolBar

用途：Fluent 风格 ToolBar。

关键 API：

- 继承自 `QToolBar`：使用 Qt 原生 API `addAction()` / `addWidget()`。

Demo：Windows / Overview。

---

## FluentStatusBar

用途：Fluent 风格 StatusBar。

关键 API：

- 继承自 `QStatusBar`：使用 Qt 原生 API `showMessage()` / `addWidget()`。

Demo：Windows / Overview。

---

## FluentDialog

用途：带 Fluent 标题栏的对话框，支持可选 mask（父窗口变暗）与可选 resize。

关键 API：

- `setMaskEnabled(bool)` / `maskEnabled()`
- `setMaskOpacity(qreal)` / `maskOpacity()`
- `setFluentWindowButtons(WindowButtons)`
- `setFluentResizeEnabled(bool)`、`setFluentResizeBorderWidth(int)`

Demo：Windows / Overview。

## MessageBox

```cpp
#include "Fluent/FluentMessageBox.h"

Fluent::FluentMessageBox::information(parent,
                                     QStringLiteral("标题"),
                                     QStringLiteral("内容"),
                                     QStringLiteral("说明"));
```

用途：Fluent 风格消息框（支持 detail 文本、可选链接、可选 mask）。

关键 API：

- `FluentMessageBox(title, message, icon, parent)` / 带 `detail/link` 的构造。
- 静态便捷函数：`information()` / `warning()` / `error()` / `question()`。
- `setMaskEnabled(bool)` / `setMaskOpacity(qreal)`。

Demo：Windows / Overview。

---

## FluentToast

用途：右下角等位置的通知 Toast（自动消失，支持动画与进度条）。

关键 API：

- `FluentToast::showToast(window, title, message, durationMs)`
- `FluentToast::showToast(window, title, message, Position, durationMs)`

Demo：Windows / Pickers / Overview。

---

## FluentResizeHelper

用途：为无边框窗口/对话框提供边缘拖拽 resize（基于事件过滤 + hit-test）。

关键 API：

- `setEnabled(bool)` / `isEnabled()`
- `setBorderWidth(int)` / `borderWidth()`

Demo：Windows / Overview（主要被 `FluentMainWindow` / `FluentDialog` 内部使用）。

## Toast

```cpp
#include "Fluent/FluentToast.h"

Fluent::FluentToast::showToast(parent,
                              QStringLiteral("Toast"),
                              QStringLiteral("这是一条通知"),
                              Fluent::FluentToast::Position::BottomRight,
                              2600);
```
