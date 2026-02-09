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

### 默认行为与结构

- **不使用 QMainWindow 的 ToolBar/StatusBar 区域**：`addToolBar()` / `insertToolBar()` / `setStatusBar()` 会直接隐藏并 `deleteLater()` 传入的对象（`FluentMainWindow` 期望你的布局都在中央区域里完成）。
- **CentralWidget 会被“包一层”**：第一次 `setCentralWidget()` 时会创建内部容器（central border host + clip host），并把你的 widget re-parent 到该容器内。
    - 该内部容器会为边框/圆角裁剪预留 1px（左右/底部）空间。
    - 圆角裁剪通过 `setMask()` 实现：标题栏可见时中央区域只裁剪底部两个角；标题栏折叠/隐藏时裁剪四个角，避免出现缝隙。
- **标题栏是 `menuWidget()`**：标题栏宿主（`FluentTitleBarHost`）通过 `setMenuWidget()` 放在主窗口顶部。

### 标题栏内容：覆盖/插槽/自动折叠

- `setFluentTitleBarTitle()` / `setFluentTitleBarIcon()`：开启覆盖后，标题/图标优先使用覆盖值；`clear...()` 后回退到 `windowTitle()` / `windowIcon()`（若窗口图标为空，会回退到 `qApp->windowIcon()`）。
- `setFluentTitleBarCenterWidget(QWidget*)`：
    - 传入非空时：隐藏默认标题 `QLabel`，把你的 widget 放到居中区域；
    - 传入空时：恢复默认标题 `QLabel`。
- `setFluentTitleBarLeftWidget(QWidget*)` / `setFluentTitleBarRightWidget(QWidget*)`：
    - 仅作为插槽容器（布局 spacing=8）；传入空则隐藏对应插槽。
    - 右侧区域 `m_rightHost` 会在“存在任意窗口按钮”或“右插槽非空”时显示。
- **标题栏自动折叠**：当且仅当下面条件都不满足时，标题栏会被折叠为高度 0，使中央内容占满顶部：
    - 任何窗口按钮可见（最小化/最大化/关闭任意一个）；
    - 任意自定义内容存在（左/中/右任一插槽非空）；
    - 有 MenuBar；
    - 有标题/图标覆盖。

### 标题文本省略与高度自适应

- 标题文本会根据左右区域的**预留宽度**（`minimumWidth` 与 `sizeHint` 的较大值）进行 `Qt::ElideRight`，避免短暂挤压导致 `QMenuBar` 被迫进入 Qt 原生 overflow（`qt_menubar_ext_button`）。
- 标题栏高度会根据内容动态调整：
    - 有窗口按钮时至少为 `Style::windowMetrics().titleBarHeight`；
    - 同时取布局 `sizeHint().height()` 的更大值，避免自定义控件（例如标题栏内放 `FluentLineEdit`）被裁剪。

### 拖拽/双击行为

- 标题栏可拖拽移动：在标题栏区域按下左键时，如果命中的是交互控件（按钮/可聚焦控件/MenuBar 区域），事件会交给控件；否则触发系统移动（`startSystemMove()`）。
- 双击标题栏会在最大化/还原间切换，并同步窗口按钮 glyph。
- 当标题栏折叠/隐藏时：会给新加入的子控件安装事件过滤器，允许在“非交互区域”按下左键时拖拽窗口（但不会抢夺按钮、可聚焦控件、可选中文本的 `QLabel` 等）。

### MenuBar 集成与迁移

- `menuBar()`：不是 `QMainWindow::menuBar()` 的虚函数覆写（Qt 本身不可覆写），而是一个“便利 shim”。第一次调用会自动创建并安装 `FluentMenuBar`。
- `setMenuBar(QMenuBar*)`：
    - 传入 `FluentMenuBar*`：直接安装；
    - 传入原生 `QMenuBar*`：会 best-effort 迁移其 actions/menus 到新的 `FluentMenuBar`（保留 `QAction` 实例），并在原 menuBar 属于本窗口时 `deleteLater()`。
- 为避免 QMenuBar 自带 overflow：标题栏里的 menuBar 会被设置为 `QSizePolicy::Minimum`，并且设置 `minimumWidth(minimumSizeHint().width())`。

### Windows 平台：无边框 resize（WM_NCHITTEST）

- 启用标题栏（frameless）时，Windows 下会处理 `WM_NCHITTEST`：
    - 使用 `Style::windowMetrics().resizeBorder` 作为边缘 hit-test 宽度，返回 `HTLEFT/HTTOP/HTBOTTOMRIGHT...` 等以支持边缘拖拽 resize。
    - 标题栏区域：如果鼠标在交互控件上（按钮/可聚焦控件/MenuBar 区域），返回 `HTCLIENT`，保证点击能传递到控件；否则返回 `HTCAPTION`，允许拖拽。

### 主题联动与边框

- 主题变化会同步更新全局 `QApplication` stylesheet（`Theme::baseStyleSheet`），并做“相等判定”避免重复设置触发全量 repolish。
- 窗口外框/标题栏边框颜色与 `ThemeManager::accentBorderEnabled()` 联动；边框 trace 动画使用内部 overlay（marquee）绘制。

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

### FluentMenuBar：交互与绘制语义

- **click-to-popup**：左键按下即 `popup()` 打开菜单；`mouseReleaseEvent` 会吞掉左键 release，避免触发 QMenuBar 原生弹出逻辑。
- **自动转换菜单为 FluentMenu**：无论外部通过 `addMenu(QMenu*)` 还是修改 action，都会在 `ActionAdded/ActionChanged` 时扫描并把原生 `QMenu` 转换为 `FluentMenu`（保留原 `QAction`，迁移 actions 后删除原 menu）。
- **禁用原生 overflow 按钮**：检测到 `qt_menubar_ext_button` 后会隐藏/禁用，并清空其 menu（避免出现原生样式 overflow 菜单）。
- **高亮动画**：
    - hoverLevel：120ms 线性插值；
    - highlightRect：160ms `OutCubic` 滑动；
    - 自绘圆角高亮（radius=6）和底部分割线（alpha≈70）。
- **稳定宽度**：`sizeHint()/minimumSizeHint()` 会按 action 文本计算“足够的宽度”，减少被挤进 overflow 的概率。

### FluentMenu：弹出动画、子菜单、glyph

- `QProxyStyle` 覆写：子菜单延迟 120ms，关闭 sloppy submenu。
- `aboutToShow` 时确保所有子菜单也转换为 `FluentMenu`（递归）。
- **弹出动画**：show 时先 `opacity=0` 并下移 6px，然后 140ms `OutCubic` 同步 fade+slide 到最终位置。
- **绘制语义**：
    - panel 圆角半径 10，先清透明，再用 `paintFluentPanel()` 绘制面板；
    - hover 高亮圆角 6，颜色来自 `colors.hover` 并按 hoverLevel 调整 alpha；
    - 额外自绘：checked action 的 accent 对勾、以及有子菜单的 chevron。

---

## FluentToolBar

用途：Fluent 风格 ToolBar（把普通 `QAction` 自动包装成 `FluentToolButton`）。

关键 API / 行为：

- 继承自 `QToolBar`：仍然使用 Qt 原生 API `addAction()` / `addWidget()`。
- 当外部 `addAction(QAction*)`（且该 action 不是 separator / 不是 `QWidgetAction`）时：会移除该 action 并插入一个 `QWidgetAction`，其默认 widget 为 `FluentToolButton`，并 `setDefaultAction(action)`。
    - 这意味着你无需手动创建 ToolButton；但如果你本身就插入 `QWidgetAction` / `addWidget()`，不会被自动替换。

Demo：Windows / Overview。

---

## FluentStatusBar

用途：Fluent 风格 StatusBar。

关键 API / 行为：

- 继承自 `QStatusBar`：使用 Qt 原生 API `showMessage()` / `addWidget()`。
- 默认关闭 size grip（`setSizeGripEnabled(false)`），并启用 `WA_StyledBackground` 以配合 QSS。

Demo：Windows / Overview。

---

## FluentDialog

用途：带 Fluent 标题栏的对话框，支持可选 mask（父窗口变暗）与可选 resize。

关键 API：

- `setMaskEnabled(bool)` / `maskEnabled()`
- `setMaskOpacity(qreal)` / `maskOpacity()`
- `setFluentWindowButtons(WindowButtons)`
- `setFluentResizeEnabled(bool)`、`setFluentResizeBorderWidth(int)`

实现语义要点：

- Frameless + translucent：`WA_TranslucentBackground` + `Qt::FramelessWindowHint`，并使用 `paintFluentFrame()` 绘制圆角面板（radius=10）。
- **首帧 paint 防护**：第一次 `paintEvent` 会跳过并 `singleShot(0)` 触发 `update()`，避免某些平台 `QPainter::begin engine==0` 警告。
- **拖拽区域**：
    - 标题栏可见时，仅标题栏区域能拖拽；且不会抢夺窗口按钮的事件。
    - 标题栏隐藏（例如按钮全隐藏导致高度为 0）时：允许在“非交互区域”拖拽整个对话框（同样不会抢夺按钮/可聚焦控件/可选中文本）。
- **ResizeHelper 默认关闭**：`setFluentResizeEnabled(true)` 才会创建并启用 `FluentResizeHelper`。
- **Mask overlay**：启用后会在父窗口（top-level）上创建遮罩 widget，自动跟随父窗口 resize/show/state change，且会吞鼠标事件阻止背景交互。

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

实现语义要点：

- 同样支持 mask overlay（父窗口变暗并拦截输入）。
- **文本高度自适应 + 多行省略**：
    - 对话框最大高度约束为 `max(240, availableScreenHeight * 0.72)`；
    - 初次会先尝试用完整文本布局，只有确实超出最大高度才触发多行 elide；
    - 发生 elide 时：label 显示省略后的文本，完整文本保存在 tooltip（鼠标悬停可查看）。
- **拖拽**：允许在顶部区域（默认 48px）拖拽移动消息框。

Demo：Windows / Overview。

---

## FluentToast

用途：窗口内浮层 Toast（自动消失，支持动画与进度条；点击可立即关闭）。

关键 API：

- `FluentToast::showToast(window, title, message, durationMs)`
- `FluentToast::showToast(window, title, message, Position, durationMs)`

实现语义要点：

- Toast 通过内部 `ToastOverlay` 挂在目标窗口上（对象名 `FluentToastOverlay`），并用 **mask region** 只覆盖 toast 周围区域：
    - 有 toast 时 overlay 会拦截输入，但只在 toast 区域；其余窗口区域仍可交互。
    - 没有 toast 时 overlay 会 `WA_TransparentForMouseEvents=true`，完全不影响窗口。
- 布局：每个位置（TopLeft/TopCenter/...）各维护一个队列，新 toast 会插在队列头部；其余 toast 会用 `QPropertyAnimation(pos)` 做平滑移动（150~180ms，`OutCubic`）。
- 尺寸稳定：为避免 `QLabel(wordWrap)` 在快速创建时出现“多一行跳动”，overlay 会先固定 label wrap 宽度，再 `adjustSize()`，并在下一次事件循环再次 stabilize + relayout。
- 出现/消失：
    - 出现时 opacity 180ms `OutCubic` 从 0→1；
    - 自动消失计时使用线性进度动画，最短 `max(800, durationMs)`；
    - 关闭时 opacity 160ms `InCubic` 到 0（或不带动画直接销毁）。
- 进度条：从中间向两侧扩展/收缩（以 label 区域居中为基准）。

Demo：Windows / Pickers / Overview。

---

## FluentResizeHelper

用途：为无边框窗口/对话框提供边缘拖拽 resize（基于事件过滤 + hit-test）。

关键 API：

- `setEnabled(bool)` / `isEnabled()`
- `setBorderWidth(int)` / `borderWidth()`

Demo：Windows / Overview（主要被 `FluentMainWindow` / `FluentDialog` 内部使用）。
