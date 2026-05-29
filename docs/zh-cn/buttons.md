# 按钮与开关

本模块包含常见的可点击/可切换控件，均做了 Fluent 风格的圆角、hover/press/focus 动效。

## 控件清单（对应公开头文件）

- `FluentButton`（include: `Fluent/FluentButton.h`）
- `FluentAnimatedButton`（include: `Fluent/FluentAnimatedButton.h`）
- `FluentAnimatedIcon`（include: `Fluent/FluentAnimatedIcon.h`）
- `FluentLottieWidget`（include: `Fluent/FluentLottieWidget.h`）
- `FluentToolButton`（include: `Fluent/FluentToolButton.h`）
- `FluentDropDownButton`（include: `Fluent/FluentDropDownButton.h`）
- `FluentSplitButton`（include: `Fluent/FluentSplitButton.h`）
- `FluentCommandBar`（include: `Fluent/FluentCommandBar.h`）
- `FluentToggleSwitch`（include: `Fluent/FluentToggleSwitch.h`）
- `FluentCheckBox`（include: `Fluent/FluentCheckBox.h`）
- `FluentRadioButton`（include: `Fluent/FluentRadioButton.h`）
- `FluentProgressRing`（include: `Fluent/FluentProgressRing.h`）

Demo 页面：Buttons（`demo/pages/PageButtons.cpp`），以及 Overview（`demo/pages/PageOverview.cpp`）。

---

## FluentLottieWidget / FluentAnimatedIcon / FluentAnimatedButton

用途：`FluentLottieWidget` 是通用 Lottie JSON 播放控件；`FluentAnimatedIcon` 在它之上增加 WinUI 风格的状态语义；`FluentAnimatedButton` 则把 animated icon 接到 `QPushButton` 语义里，适合搜索、导航、按钮图标等需要 hover / pressed / selected 动效的场景。

完整用法、主题 tint、资源制作与性能说明见 [lottie.md](lottie.md)；本节保留按钮页相关的快速参考。

依赖：项目通过 `third_party/rlottie` Git 子模块源码级集成 rlottie。新 clone 仓库后需执行：

```bash
git submodule update --init --recursive
```

继承与构造：

- `class FluentLottieWidget : public QWidget`
- `class FluentAnimatedIcon : public FluentLottieWidget`
- `class FluentAnimatedButton : public QPushButton`
- 构造：`FluentLottieWidget(QWidget*)`、`FluentAnimatedIcon(QWidget*)`、`FluentAnimatedButton(QWidget*)`、`FluentAnimatedButton(const QString&, QWidget*)`

关键 API：

- `load(const QString&)`：从 `.json` 文件加载 Lottie。
- `loadData(const QByteArray&, const QString& cacheKey = {}, const QString& resourcePath = {})`：从内存加载 Lottie。
- `play()` / `pause()` / `stop()`：播放控制。
- `setCurrentFrame(int)` / `setProgress(qreal)`：定位帧。
- `playSegment(int, int)` / `playMarker(const QString&)`：播放帧区间或 marker 区间。
- `markerNames()` / `hasMarker()` / `markerFrame()` / `markerEndFrame()`：查询 Lottie 顶层 markers。
- `FluentLottieWidget::setTintColor(const QColor&)` / `resetTintColor()`：可选地把渲染帧按 alpha 重染为单色，适合图标型 Lottie。
- `FluentAnimatedIcon::setState(const QString&, bool animated = true)`：按状态切换 marker 动画。
- `FluentAnimatedIcon::setInteractive(true)`：控件自身 hover / press 时自动切换 `Normal`、`PointerOver`、`Pressed`。
- `FluentAnimatedButton::loadAnimation()` / `loadAnimationData()`：加载按钮图标动画。
- `FluentAnimatedButton::animatedIcon()`：访问内部图标控件，用于设置 fallback icon、查询 marker 等。
- `FluentAnimatedButton::setAnimatedIconSize()`：设置按钮内动画图标尺寸。
- `FluentAnimatedButton` 会自动把内部动画图标 tint 到当前前景色，因此 Primary 按钮会使用白色动画图标，禁用态会使用禁用文本色。

`FluentAnimatedIcon` 的 marker 查找规则：

- `[PreviousState]To[NewState]_Start` + `_End`：播放这一段。
- `[PreviousState]To[NewState]`：播放或跳转 transition marker。
- `[NewState]`：跳转到状态 marker。
- 任意 `To[NewState]_End`：作为兜底状态帧。
- 数字 state：作为目标帧。
- 找不到 marker 时回到 0 帧。

示例：

```cpp
#include "Fluent/FluentAnimatedIcon.h"

auto *icon = new Fluent::FluentAnimatedIcon();
icon->setFixedSize(48, 48);
icon->load(QStringLiteral(":/lottie/search.json"));
icon->setInteractive(true);
icon->setState(QStringLiteral("Normal"), false);
```

按钮示例：

```cpp
#include "Fluent/FluentAnimatedButton.h"

auto *button = new Fluent::FluentAnimatedButton(QStringLiteral("Search"));
button->loadAnimation(QStringLiteral(":/lottie/search.json"));
connect(button, &QPushButton::clicked, this, [] {
    qDebug() << "Search clicked";
});
```

注意事项：

- `FluentAnimatedButton` 适合动作入口，不建议把所有普通表单按钮都替换成动画按钮。
- 第一版不会把 animated icon 自动接入所有导航控件；目前可作为独立 QWidget 或按钮放进布局。
- 如果 Lottie 加载失败，会绘制 fallback icon 或默认静态 glyph。
- 公开头文件不暴露 rlottie 类型，后续替换/扩展后端不影响使用方 API。

完整接入计划见 [animated-icon-plan.md](animated-icon-plan.md)。

---

## FluentButton

用途：标准按钮（`QPushButton`），Fluent 风格圆角/边框 + hover/press 动效 + focus ring。支持 Primary（强调/填充）样式，并对“可勾选按钮”（`setCheckable(true)`）做了更接近 Win11 的选中表现。

继承与构造：

- `class FluentButton : public QPushButton`
- 构造：`FluentButton(QWidget*)`、`FluentButton(const QString&, QWidget*)`

外观/交互要点：

- 最小高度来自 `Style::metrics().height`（更贴近 Win11 控件高度）。
- Secondary（默认）：surface 填充 + border；若设置为 checkable 且 checked，会出现轻微 accent tint + accent 边框，并把文本颜色略偏向 accent。
- Primary：accent 填充；若 checkable 且 checked，会额外绘制一层内侧高光以强化“已选中”。
- 有图标时：图标绘制在左侧，文本居中对齐图标组（icon + gap + text）。

主题联动：控件会监听 `ThemeManager::themeChanged`，以及自身 `EnabledChange`，自动触发重绘。

关键 API：

- `setPrimary(bool)` / `isPrimary()`：切换 Primary 样式。
- `setCheckable(bool)` / `setChecked(bool)` / `toggled(bool)`：继承自 `QAbstractButton` 的原生能力；本控件对 checked 状态有额外绘制细节。
- `setIcon(const QIcon&)` / `setIconSize(const QSize&)`：原生 API；本控件会使用 `icon()` + `iconSize()` 参与布局。
- `hoverLevel` / `pressLevel`（Q_PROPERTY）：动效层（通常由控件内部动画驱动；仅在你要做自定义动画/测试时手动设置）。

Demo：Buttons / Containers / Pickers / Windows / Overview。

示例：

```cpp
#include "Fluent/FluentButton.h"

auto *btn = new Fluent::FluentButton(QStringLiteral("Primary"));
btn->setPrimary(true);
```

带图标与 checkable 的示例：

```cpp
auto *toggle = new Fluent::FluentButton(QStringLiteral("Pin"));
toggle->setCheckable(true);
toggle->setIcon(QIcon(":/icons/pin.svg"));
toggle->setIconSize(QSize(16, 16));
connect(toggle, &QAbstractButton::toggled, this, [](bool on) {
	qDebug() << "Pinned:" << on;
});
```

注意事项：

- 这是自绘按钮（重载 `paintEvent`），如果你依赖复杂的 QSS 对按钮背景/边框进行覆盖，可能不会生效；推荐通过 `ThemeManager`/`ThemeColors` 调整整体配色。

---

## FluentToolButton

用途：工具按钮（`QToolButton`），适合标题栏/工具栏/卡片折叠箭头等紧凑交互。默认启用 `autoRaise`（更接近 Win11 工具按钮观感），并提供 hover/press 动效与 focus ring。

继承与构造：

- `class FluentToolButton : public QToolButton`
- 构造：`FluentToolButton(QWidget*)`、`FluentToolButton(const QString&, QWidget*)`

外观/交互要点：

- 默认 `setAutoRaise(true)`，并使用 `Style::metrics().height` 作为最小高度。
- 若 `setCheckable(true)` 且 checked，会使用“轻微 accent tint + accent 边框”的方式表现选中状态，并把 label 颜色略偏向 accent。

标题栏窗口按钮（内部约定）：

当设置动态属性 `fluentWindowGlyph`（int）时，控件会按“窗口 caption 按钮”方式绘制背景和 glyph：

- `0`：最小化
- `1`：最大化
- `2`：还原
- `3`：关闭（hover 会用更明显的红色）

该属性主要由 `FluentMainWindow` 内部使用；一般业务代码无需设置。

关键 API：

- `setCheckable(bool)` / `setChecked(bool)`：使用 Qt 原生 API。
- `hoverLevel` / `pressLevel`（Q_PROPERTY）：动效层（内部动画驱动）。

Demo：Buttons / Overview（也用于若干容器控件内部）。

---

## FluentDropDownButton / FluentSplitButton / FluentCommandBar

用途：命令入口类控件，适合工具栏、页面命令区、卡片标题区等需要组织一组操作的场景。三者的侧重点不同：

- `FluentDropDownButton`：一个按钮只负责展开一组次级动作。
- `FluentSplitButton`：左侧是高频默认动作，右侧是同一动作的变体或更多选项。
- `FluentCommandBar`：以 `QAction` 为中心组织一组常用命令，统一承载按钮、菜单命令、分隔线和少量自定义控件；当 action 的 enabled/text/icon 改变时，命令栏中的按钮会同步更新。

继承与构造：

- `class FluentDropDownButton : public FluentButton`
- `class FluentSplitButton : public QWidget`
- `class FluentCommandBar : public QWidget`

关键 API：

- `FluentDropDownButton::setMenu(QMenu*)` / `addAction()`：按钮点击时展开菜单。
- `FluentSplitButton::setDefaultAction(QAction*)`：左侧执行主动作。
- `FluentSplitButton::setMenu(QMenu*)`：右侧箭头展开更多动作。
- `FluentCommandBar::addAction()` / `addCommand(QAction*)` / `addSeparator()` / `addWidget()`：组织命令按钮、菜单命令、分隔线和自定义控件；宽度不足时尾部命令会自动收进右侧 overflow FluentMenu。

示例：

```cpp
#include "Fluent/FluentDropDownButton.h"
#include "Fluent/FluentSplitButton.h"
#include "Fluent/FluentCommandBar.h"
#include "Fluent/FluentMenu.h"

#include <QAction>

auto *drop = new Fluent::FluentDropDownButton(QStringLiteral("More"));
drop->addAction(QStringLiteral("Copy"));
drop->addAction(QStringLiteral("Move"));

auto *split = new Fluent::FluentSplitButton(QStringLiteral("Save"));
split->setDefaultAction(new QAction(QStringLiteral("Save"), split));
auto *menu = new Fluent::FluentMenu(split);
menu->addAction(QStringLiteral("Save as"));
split->setMenu(menu);

auto *bar = new Fluent::FluentCommandBar();
auto *newAction = new QAction(QStringLiteral("New"), bar);
auto *editAction = new QAction(QStringLiteral("Edit"), bar);
bar->addCommand(newAction);
bar->addCommand(editAction);
bar->addSeparator();

auto *exportMenu = new Fluent::FluentMenu(bar);
exportMenu->setTitle(QStringLiteral("Export"));
exportMenu->addAction(QStringLiteral("PDF"));
bar->addCommand(exportMenu->menuAction());

editAction->setEnabled(false); // CommandBar 内对应按钮会同步禁用。
```

注意事项：

- `FluentCommandBar` 是轻量命令容器，不替代 `QToolBar`；需要主窗口工具栏语义时继续使用 `FluentToolBar`。
- `FluentSplitButton` 左右两块分别响应，适合“默认动作 + 更多选项”；如果只有菜单，使用 `FluentDropDownButton` 更清楚。
- `FluentCommandBar` 的价值在于集中管理一组命令，而不是简单替代 `QHBoxLayout + Button`；overflow 菜单仍复用原始 `QAction`，因此 enabled、text、icon、menu 状态会继续同步。

Demo：Buttons / Overview。

---

## FluentToggleSwitch

用途：Win11 风格开关（独立自绘 `QWidget`），包含 track + knob 动效、hover 行高亮与 focus ring。

继承与构造：

- `class FluentToggleSwitch : public QWidget`
- 构造：`FluentToggleSwitch(QWidget*)`、`FluentToggleSwitch(const QString&, QWidget*)`

外观/交互要点：

- Track 尺寸固定为约 `40x20`，knob 为 Islands 风格圆点（无阴影，偏性能优先）。
- `setText()` 可显示右侧标签（会自动省略）。
- `setChecked()` 会触发平滑的 progress 动画并发出 `toggled(bool)`。

关键 API：

- `setChecked(bool)` / `isChecked()`：开关状态。
- `toggled(bool)`：状态变化信号。
- `setText(const QString&)`：显示标签。
- `progress` / `hoverLevel` / `focusLevel`（Q_PROPERTY）：动画/交互层（内部 `QPropertyAnimation` 驱动）。

示例：

```cpp
#include "Fluent/FluentToggleSwitch.h"

auto *sw = new Fluent::FluentToggleSwitch(QStringLiteral("自动保存"));
sw->setChecked(true);
connect(sw, &Fluent::FluentToggleSwitch::toggled, this, [](bool on) {
	qDebug() << "AutoSave:" << on;
});
```

注意事项：

- 该控件是自绘 `QWidget`，不是 `QAbstractButton`；如果你需要 button group/autoExclusive 等能力，建议使用 `FluentRadioButton` 或自行封装。

Demo：Buttons / Containers / Windows / Overview。

---

## FluentCheckBox

用途：复选框（`QCheckBox`），包含 hover 行高亮、focus ring，以及勾选动画（checkmark 渐入）。

继承与构造：

- `class FluentCheckBox : public QCheckBox`
- 构造：`FluentCheckBox(QWidget*)`、`FluentCheckBox(const QString&, QWidget*)`

关键 API：

- `setChecked(bool)` / `isChecked()`：使用 Qt 原生 API。
- `stateChanged(int)`：原生信号（`Qt::Unchecked/Checked/PartiallyChecked`）；控件内部也用它驱动 check 动画。
- `hoverLevel` / `focusLevel`（Q_PROPERTY）：动效层（内部动画驱动）。

尺寸策略：重载了 `sizeHint()`/`minimumSizeHint()`，会基于文本宽度给出更贴近 Win11 的占位。

Demo：Buttons / Overview。

---

## FluentRadioButton

用途：单选按钮（`QRadioButton`），包含 hover 行高亮、focus ring，以及选中动画（dot 缩放 + accent border 渐入）。

继承与构造：

- `class FluentRadioButton : public QRadioButton`
- 构造：`FluentRadioButton(QWidget*)`、`FluentRadioButton(const QString&, QWidget*)`

关键 API：

- `setChecked(bool)` / `isChecked()`：使用 Qt 原生 API。
- `toggled(bool)`：原生信号；控件内部用它驱动选中动画。
- `hoverLevel` / `focusLevel`（Q_PROPERTY）：动效层（内部动画驱动）。

尺寸策略：重载了 `sizeHint()`/`minimumSizeHint()`，会基于文本宽度给出更贴近 Win11 的占位。

Demo：Buttons / Overview。
