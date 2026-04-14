# NavigationView

## 控件清单

- `FluentNavigationView`（include: `Fluent/FluentNavigationView.h`）
- `FluentNavigationItem`：NavigationView 的数据模型

Demo 入口：Containers 页面（`demo/pages/PageContainers.cpp`）以及 Demo 主窗口左侧导航（`demo/DemoWindow.cpp`）。

## 基础用法

```cpp
#include "Fluent/FluentNavigationView.h"

using NI = Fluent::FluentNavigationItem;

auto *nav = new Fluent::FluentNavigationView();
nav->setExpandedWidth(280);
nav->setCompactWidth(48);
nav->setPaneDisplayMode(Fluent::FluentNavigationView::Left);
nav->setPaneTitle(QStringLiteral("Pane Title"));
nav->setBackButtonVisible(true);
nav->setAutoCollapseWidth(800);

auto applyGlyph = [](NI &item, ushort codePoint) {
        item.iconGlyph = QString(QChar(codePoint));
        item.iconFontFamily = QStringLiteral("Segoe Fluent Icons");
};

NI home;
home.key = QStringLiteral("home");
home.text = QStringLiteral("首页");
applyGlyph(home, 0xE80F);
nav->addItem(home);

NI documents;
documents.key = QStringLiteral("documents");
documents.text = QStringLiteral("文档选项");
documents.selectsOnInvoked = false;
applyGlyph(documents, 0xE8A5);

NI recent;
recent.key = QStringLiteral("recent_files");
recent.text = QStringLiteral("最近文件");
applyGlyph(recent, 0xE823);
documents.children.push_back(recent);

nav->addItem(documents);

NI help;
help.key = QStringLiteral("help_center");
help.text = QStringLiteral("帮助中心");
applyGlyph(help, 0xE897);
nav->addFooterItem(help);

nav->setSelectedKey(QStringLiteral("home"));

QObject::connect(nav, &Fluent::FluentNavigationView::itemInvoked,
                                 nav, [](const QString &key) {
        qDebug() << "invoked:" << key;
});
```

## 数据模型

`FluentNavigationItem` 当前包含这些字段：

- `key`：唯一标识，用于 `setSelectedKey()`、`selectedKeyChanged` 和 `itemInvoked`。
- `text`：显示文本。
- `icon`：普通 `QIcon` 图标。
- `iconGlyph`：字体 glyph 图标；如果设置，会优先于 `icon` 渲染。
- `iconFontFamily`：glyph 使用的字体族；未指定时默认使用 `Segoe Fluent Icons`。
- `separator`：若为 `true`，该项会被渲染为分隔线。
- `selectsOnInvoked`：是否在点击时同步修改 `selectedKey`；设为 `false` 时更接近 WinUI3 的 submenu-only 项。
- `children`：一级子项列表；当前实现只支持一层嵌套。

## Pane 模式与外观

关键 API：

- `setPaneDisplayMode(PaneDisplayMode)`：切换 `Left`、`LeftCompact`、`Top` 三种模式。
- `setExpandedWidth(int)` / `setCompactWidth(int)`：控制左侧展开/收起宽度。
- `setPaneTitle(const QString &)`：设置左侧控制行标题。
- `setBackButtonVisible(bool)` / `setBackButtonEnabled(bool)`：控制返回按钮显示与可用态。
- `setFooterVisible(bool)`：整体显示或隐藏 footer 区域。
- `setExpanded(bool)` / `toggleExpanded()`：在左侧模式间切换展开/收起；Top 模式不受这两个 API 影响。

布局语义：

- `Left`：左侧展开模式，显示文本、paneTitle 和可选 `headerWidget`。
- `LeftCompact`：左侧收起模式，仅保留图标；带子项的父项点击时会弹出 flyout。
- `Top`：顶部模式，顶层项水平排布，footer 项会靠右显示。

## 数据与 Footer API

推荐优先使用追加式 API，而不是预先手工组装 footer 容器：

- `addItem(const FluentNavigationItem &item)`：追加主导航项。
- `setItems(const std::vector<FluentNavigationItem> &items)`：整体替换主导航项。
- `clearItems()`：清空主导航项。
- `addFooterItem(const FluentNavigationItem &item)`：追加 footer 项。
- `setFooterItems(const std::vector<FluentNavigationItem> &items)`：整体替换 footer 项。
- `clearFooterItems()`：清空 footer 项。

`FluentNavigationView` 本身不会内置“设置页”。如果你要放设置、帮助、账户、反馈等入口，直接通过 `addFooterItem()` 或 `setFooterItems()` 显式配置即可。

## 交互语义

- Left 模式下，正文点击和箭头点击是分开的：正文负责选中/进入，箭头只负责展开或收起父项。
- 同一时刻只保留一个展开分组，减少多组同时展开时的视觉干扰。
- `setSelectedKey()` 选中子项时，Left 模式会自动展开其父分组；Top 模式则保持父项高亮。
- 对于带子项的父项，如果 `selectsOnInvoked = false`：
    - Left 模式正文点击只展开或收起子项。
    - LeftCompact / Top 模式正文点击只打开 flyout，不会修改 `selectedKey`。
- 对于叶子项，如果 `selectsOnInvoked = false`，点击仍会发出 `itemInvoked`，但不会改变当前选中态。

## Header、自适应与信号

可选 header：

```cpp
auto *header = new QWidget();
// ... 给 header 设置布局和内容
nav->setHeaderWidget(header);
```

Header 只会在 `Left` 模式且宽度足够时显示。

自适应收起：

- `setAutoCollapseWidth(int)` 会在父窗口宽度低于阈值时自动从 `Left` 切到 `LeftCompact`。
- 从 `LeftCompact` 恢复到 `Left` 时会带约 `+100px` 回滞，减少来回抖动。
- Top 模式不会被自动收起逻辑接管。

信号：

- `selectedKeyChanged(const QString &key)`：选中项变化。
- `itemInvoked(const QString &key)`：条目被点击或 flyout 子项被触发。
- `backRequested()`：返回按钮被点击。
- `expandedChanged(bool expanded)`：Left / LeftCompact 切换。
- `paneDisplayModeChanged(PaneDisplayMode mode)`：Pane 模式变化。

## Demo 说明

- Demo 主窗口左侧展示了 `addFooterItem()` 的显式 footer 用法，设置页只是 demo 自己挂进去的一个普通 footer 项。
- Containers 页面提供了一个 API in action 示例，便于单独验证：
    - Left / LeftCompact / Top 三种布局切换
    - back button 可见/可用态切换
    - footer 显隐与显式追加
    - `selectsOnInvoked` 对父项 submenu-only 行为的影响
