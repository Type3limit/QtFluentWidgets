#include "PageContainers.h"

#include "../DemoHelpers.h"
#include "Fluent/FluentCard.h"

#include "Fluent/FluentButton.h"
#include "Fluent/FluentAnnotatedScrollBar.h"
#include "Fluent/FluentFlowLayout.h"
#include "Fluent/FluentSplitter.h"

#include <QHBoxLayout>
#include <QMap>
#include <QPair>
#include <QScrollBar>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>

#include <QFrame>

#include "Fluent/FluentGroupBox.h"
#include "Fluent/FluentIcon.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentLineEdit.h"
#include "Fluent/FluentMainWindow.h"
#include "Fluent/FluentNavigationView.h"
#include "Fluent/FluentProgressBar.h"
#include "Fluent/FluentRadioButton.h"
#include "Fluent/FluentScrollArea.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTabWidget.h"
#include "Fluent/FluentTextEdit.h"
#include "Fluent/FluentToolButton.h"
#include "Fluent/FluentToggleSwitch.h"
#include "Fluent/FluentWidget.h"

#include <QGridLayout>
#include <QEvent>

#include <functional>
#include <memory>

namespace Demo::Pages {

using namespace Fluent;

namespace {

class DemoSizeLabelWatcher final : public QObject
{
public:
    DemoSizeLabelWatcher(QWidget *watched, FluentLabel *label, QObject *parent = nullptr)
        : QObject(parent)
        , m_watched(watched)
        , m_label(label)
    {
        updateLabel();
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (obj == m_watched && event->type() == QEvent::Resize) {
            updateLabel();
        }
        return QObject::eventFilter(obj, event);
    }

private:
    void updateLabel()
    {
        if (!m_watched || !m_label) {
            return;
        }
        m_label->setText(DEMO_TEXT("预览区：%1×%2px", "Preview: %1 x %2 px").arg(m_watched->width()).arg(m_watched->height()));
    }

    QWidget *m_watched = nullptr;
    FluentLabel *m_label = nullptr;
};

QFrame *makeNavigationPreviewBlock(int minHeight, qreal mixAmount, qreal alpha, bool accentTint = false)
{
    auto *frame = new QFrame();
    frame->setMinimumHeight(minHeight);
    frame->setAttribute(Qt::WA_StyledBackground, true);

    auto applyTheme = [frame, mixAmount, alpha, accentTint]() {
        const auto &tokens = ThemeManager::instance().tokens();
        QColor fill = accentTint
            ? Style::mix(tokens.neutral.card, tokens.accent.base, tokens.dark ? mixAmount + 0.04 : mixAmount)
            : Style::mix(tokens.neutral.card, tokens.neutral.cardHover, mixAmount);
        fill.setAlphaF(alpha);
        frame->setStyleSheet(QStringLiteral("background-color: %1; border-radius: %2px;")
                                 .arg(fill.name(QColor::HexArgb))
                                 .arg(tokens.radius.overlay));
    };

    applyTheme();
    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, frame, applyTheme);
    return frame;
}

FluentCard *makeAnnotatedSectionCard(const QString &group, const QString &title, const QString &summary)
{
    auto *card = new FluentCard();
    card->setFixedHeight(108);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    card->setProperty("_demoAnnotatedGroup", group);
    card->setProperty("_demoAnnotatedText", title);
    card->setProperty("sourceGroup", group);
    card->setProperty("sourceText", title);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(6);

    auto *titleLabel = new FluentLabel(title);
    titleLabel->setStyleSheet("font-size: 13px; font-weight: 650;");

    auto *groupLabel = new FluentLabel(DEMO_TEXT("分组：%1", "Group: %1").arg(group));
    groupLabel->setStyleSheet("font-size: 11px; opacity: 0.72;");

    auto *summaryLabel = new FluentLabel(summary);
    summaryLabel->setStyleSheet("font-size: 12px; opacity: 0.86;");
    summaryLabel->setWordWrap(true);

    layout->addWidget(titleLabel);
    layout->addWidget(groupLabel);
    layout->addWidget(summaryLabel);
    layout->addStretch(1);

    return card;
}

} // namespace

QWidget *createContainersPage(FluentMainWindow *window)
{
    return Demo::makePage([&](QVBoxLayout *page) {
        auto s = Demo::makeSection(DEMO_TEXT("容器 / 布局", "Containers / Layout"),
                                   QStringLiteral("Card / GroupBox / TabWidget / ScrollArea / ScrollBar / AnnotatedScrollBar / Splitter / FlowLayout"));

        page->addWidget(s.card);

        // Navigation and tab state matrix
        {
            const QString code = QStringLiteral(
                "auto *nav = new FluentNavigationView();\n"
                "nav->setPaneDisplayMode(FluentNavigationView::Left);\n"
                "nav->setSelectedKey(QStringLiteral(\"documents\"));\n"
                "auto *tabs = new FluentTabWidget();\n"
                "tabs->setTabDisplayMode(FluentTabWidget::TabDisplayMode::Document);\n");

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("Navigation / Tab State Matrix"),
                DEMO_TEXT("导航与页签的 Left / Compact / Top / Document 状态对比", "Compare Left / Compact / Top navigation and document-style tabs"),
                DEMO_TEXT("要点：\n"
                          "-NavigationView 的 Left、LeftCompact、Top 应共享同一 selection/hover 语言\n"
                          "-Footer 和 Settings 类入口在 compact/top 模式下也要保持清楚\n"
                          "-TabWidget 的 Underline 与 Document 模式应能明显区分，但仍来自同一 surface/token 体系",
                          "Highlights:\n"
                          "-NavigationView Left, LeftCompact, and Top modes should share one selection/hover language\n"
                          "-Footer and settings-style entries should remain clear in compact/top modes\n"
                          "-TabWidget underline and document modes should be distinct while sharing one surface/token system"),
                code,
                [=](QVBoxLayout *body) {
                    auto makeCaption = [](const QString &text, bool strong = false) {
                        auto *label = new FluentLabel(text);
                        label->setWordWrap(true);
                        label->setStyleSheet(strong
                                                 ? QStringLiteral("font-size: 12px; font-weight: 600; opacity: 0.9;")
                                                 : QStringLiteral("font-size: 12px; opacity: 0.78;"));
                        return label;
                    };

                    auto makeItems = []() {
                        std::vector<FluentNavigationItem> items;
                        FluentNavigationItem home;
                        home.key = QStringLiteral("home");
                        home.text = QStringLiteral("Home");
                        home.hasFluentIcon = true;
                        home.fluentIcon = FluentIconType::Home;
                        items.push_back(home);

                        FluentNavigationItem documents;
                        documents.key = QStringLiteral("documents");
                        documents.text = QStringLiteral("Documents");
                        documents.hasFluentIcon = true;
                        documents.fluentIcon = FluentIconType::Document;
                        items.push_back(documents);

                        FluentNavigationItem data;
                        data.key = QStringLiteral("data");
                        data.text = QStringLiteral("Data");
                        data.hasFluentIcon = true;
                        data.fluentIcon = FluentIconType::Data;
                        items.push_back(data);
                        return items;
                    };

                    auto makeFooter = []() {
                        FluentNavigationItem settings;
                        settings.key = QStringLiteral("settings");
                        settings.text = QStringLiteral("Settings");
                        settings.hasFluentIcon = true;
                        settings.fluentIcon = FluentIconType::Settings;
                        return std::vector<FluentNavigationItem>{settings};
                    };

                    auto makeNav = [&](FluentNavigationView::PaneDisplayMode mode, QSize size) {
                        auto *nav = new FluentNavigationView();
                        nav->setPaneTitle(QStringLiteral("QtFluent"));
                        nav->setExpandedWidth(212);
                        nav->setCompactWidth(52);
                        nav->setItems(makeItems());
                        nav->setFooterItems(makeFooter());
                        nav->setPaneDisplayMode(mode);
                        nav->setSelectedKey(mode == FluentNavigationView::Top
                                                ? QStringLiteral("home")
                                                : QStringLiteral("documents"));
                        nav->setFixedSize(size);
                        return nav;
                    };

                    auto makeTabs = [](FluentTabWidget::TabDisplayMode mode, bool enabled = true) {
                        auto *tabs = new FluentTabWidget();
                        tabs->setTabDisplayMode(mode);
                        tabs->setFixedSize(250, 118);
                        tabs->addTab(new FluentLabel(QStringLiteral("Overview")), QStringLiteral("Overview"));
                        tabs->addTab(new FluentLabel(QStringLiteral("Details")), QStringLiteral("Details"));
                        tabs->addTab(new FluentLabel(QStringLiteral("Settings")), QStringLiteral("Settings"));
                        tabs->setCurrentIndex(mode == FluentTabWidget::TabDisplayMode::Document ? 1 : 0);
                        tabs->setEnabled(enabled);
                        return tabs;
                    };

                    auto *navGrid = new QGridLayout();
                    navGrid->setContentsMargins(0, 0, 0, 0);
                    navGrid->setHorizontalSpacing(18);
                    navGrid->setVerticalSpacing(10);

                    navGrid->addWidget(makeCaption(QStringLiteral("FluentNavigationView"), true), 0, 0, 1, 3);
                    navGrid->addWidget(makeCaption(QStringLiteral("Left"), true), 1, 0);
                    navGrid->addWidget(makeCaption(QStringLiteral("Compact"), true), 1, 1);
                    navGrid->addWidget(makeCaption(QStringLiteral("Top"), true), 1, 2);
                    navGrid->addWidget(makeNav(FluentNavigationView::Left, QSize(212, 224)), 2, 0);
                    navGrid->addWidget(makeNav(FluentNavigationView::LeftCompact, QSize(72, 224)), 2, 1);
                    navGrid->addWidget(makeNav(FluentNavigationView::Top, QSize(292, 118)), 2, 2);
                    navGrid->setColumnStretch(0, 1);
                    navGrid->setColumnStretch(1, 0);
                    navGrid->setColumnStretch(2, 1);
                    body->addLayout(navGrid);

                    auto *tabGrid = new QGridLayout();
                    tabGrid->setContentsMargins(0, 10, 0, 0);
                    tabGrid->setHorizontalSpacing(18);
                    tabGrid->setVerticalSpacing(10);

                    tabGrid->addWidget(makeCaption(QStringLiteral("FluentTabWidget"), true), 0, 0, 1, 3);
                    tabGrid->addWidget(makeCaption(QStringLiteral("Underline"), true), 1, 0);
                    tabGrid->addWidget(makeCaption(QStringLiteral("Document"), true), 1, 1);
                    tabGrid->addWidget(makeCaption(QStringLiteral("Disabled"), true), 1, 2);
                    tabGrid->addWidget(makeTabs(FluentTabWidget::TabDisplayMode::Underline), 2, 0);
                    tabGrid->addWidget(makeTabs(FluentTabWidget::TabDisplayMode::Document), 2, 1);
                    tabGrid->addWidget(makeTabs(FluentTabWidget::TabDisplayMode::Underline, false), 2, 2);
                    tabGrid->setColumnStretch(0, 1);
                    tabGrid->setColumnStretch(1, 1);
                    tabGrid->setColumnStretch(2, 1);
                    body->addLayout(tabGrid);
                },
                false,
                130));
        }

        // FluentCard
        {
            QString code;
#define CONTAINERS_CARD(X) \
    X(auto *card = new FluentCard();) \
    X(card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);) \
    X(auto *l = new QVBoxLayout(card);) \
    X(l->setContentsMargins(16, 16, 16, 16);) \
    X(l->setSpacing(8);) \
    X(l->addWidget(new FluentLabel(DEMO_TEXT("这是一个 FluentCard 内容容器", "This is a FluentCard content container")));) \
    X(l->addWidget(new FluentLabel(DEMO_TEXT("你可以把任意控件放进来。", "You can place any controls inside it.")));) \
    X(body->addWidget(card);)

#define X(line) code += QStringLiteral(#line "\n");
            CONTAINERS_CARD(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentCard"),
                DEMO_TEXT("基础卡片容器（背景/边框/阴影随主题）", "Basic card container with theme-aware background, border, and shadow"),
                DEMO_TEXT("要点：\n"
                          "-作为内容容器时直接给 Card 设置布局\n"
                          "-内部子控件会随主题变化",
                          "Highlights:\n"
                          "-Set a layout on the Card directly when using it as a content container\n"
                          "-Child controls inside it still follow theme changes"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    CONTAINERS_CARD(X)
#undef X
                },
                false));

#undef CONTAINERS_CARD
        }

        // Collapsible FluentCard
        {
            QString code;
#define CONTAINERS_COLLAPSIBLE_CARD(X) \
    X(auto *card = new FluentCard();) \
    X(card->setCollapsible(true);) \
    X(card->setTitle(DEMO_TEXT("高级选项", "Advanced options"));) \
    X(card->setCollapsed(true);) \
    X(card->contentLayout()->addWidget(new FluentLabel(DEMO_TEXT("-折叠时内容会隐藏", "-Content is hidden while collapsed")));) \
    X(card->contentLayout()->addWidget(new FluentLabel(DEMO_TEXT("-适合放可选配置/高级设置", "-Useful for optional configuration or advanced settings")));) \
    X(body->addWidget(card);)

#define X(line) code += QStringLiteral(#line "\n");
            CONTAINERS_COLLAPSIBLE_CARD(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("Collapsible FluentCard"),
                DEMO_TEXT("点击标题折叠/展开（隐藏内容区）", "Click the title to collapse or expand the content area"),
                DEMO_TEXT("要点：\n"
                          "-setCollapsible(true) 开启\n"
                          "-setTitle() 设置标题\n"
                          "-setCollapsed(true/false) 控制展开状态",
                          "Highlights:\n"
                          "-Use setCollapsible(true) to enable collapsing\n"
                          "-Use setTitle() to set the header text\n"
                          "-Use setCollapsed(true / false) to control the expanded state"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    CONTAINERS_COLLAPSIBLE_CARD(X)
#undef X
                },
                false));

#undef CONTAINERS_COLLAPSIBLE_CARD
        }

        // GroupBox
        {
            QString code;
#define CONTAINERS_GROUPBOX(X) \
    X(auto *gb = new FluentGroupBox(QStringLiteral("FluentGroupBox"));) \
    X(auto *gbl = new QVBoxLayout(gb);) \
    X(auto *gbLine = new FluentLineEdit();) \
    X(gbLine->setPlaceholderText(DEMO_TEXT("GroupBox 内部控件同样跟随 Theme", "Controls inside the GroupBox also follow Theme"));) \
    X(auto *gbToggle = new FluentToggleSwitch(DEMO_TEXT("开关", "Toggle"));) \
    X(gbl->addWidget(gbLine);) \
    X(gbl->addWidget(gbToggle);) \
    X(body->addWidget(gb);)

#define X(line) code += QStringLiteral(#line "\n");
            CONTAINERS_GROUPBOX(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentGroupBox"),
                DEMO_TEXT("带标题边框的分组容器", "Grouped container with a titled border"),
                DEMO_TEXT("要点：\n"
                          "-适合把相关控件归类\n"
                          "-内部控件同样跟随主题",
                          "Highlights:\n"
                          "-Useful for grouping related controls\n"
                          "-Child controls remain theme-aware"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    CONTAINERS_GROUPBOX(X)
#undef X
                }));

#undef CONTAINERS_GROUPBOX
        }

        // TabWidget
        {
            QString code;
#define CONTAINERS_TABS(X) \
    X(auto *tabs = new FluentTabWidget();) \
    X(auto *tab1 = new QWidget();) \
    X(auto *tab2 = new QWidget();) \
    X(auto *documentTabs = new FluentTabWidget();) \
    X(auto *addDocumentTab = new FluentToolButton();) \
    X(tabs->setContentMargin(5);) \
    X({ auto *l = new QVBoxLayout(tab1); l->setContentsMargins(0, 0, 0, 0); l->setSpacing(10); l->addWidget(new FluentLabel(DEMO_TEXT("Tab 1：演示控件复用", "Tab 1: control reuse demo"))); l->addWidget(new FluentProgressBar()); }) \
    X({ auto *l = new QVBoxLayout(tab2); l->setContentsMargins(0, 0, 0, 0); l->setSpacing(10); l->addWidget(new FluentLabel(DEMO_TEXT("Tab 2：滚动区域", "Tab 2: scrolling area"))); auto *big = new FluentTextEdit(); big->setText(DEMO_TEXT("这里是一个 TextEdit，用来制造滚动内容。\n\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n", "This TextEdit is here to create scrollable content.\n\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n")); big->setFixedHeight(160); l->addWidget(big); }) \
    X(tabs->addTab(tab1, QStringLiteral("Tab 1"));) \
    X(tabs->addTab(tab2, QStringLiteral("Tab 2"));) \
    X(documentTabs->setTabDisplayMode(FluentTabWidget::TabDisplayMode::Document);) \
    X(documentTabs->setContentMargin(5);) \
    X(documentTabs->setTabsClosable(true);) \
    X(documentTabs->setMovable(true);) \
    X(documentTabs->setFixedHeight(118);) \
    X(documentTabs->addTab(new FluentLabel(DEMO_TEXT("标签页 x1", "Document tab x1")), FluentIcon::icon(FluentIconType::Folder), DEMO_TEXT("桌面", "Desktop"));) \
    X(documentTabs->addTab(new FluentLabel(DEMO_TEXT("标签页 x2", "Document tab x2")), FluentIcon::icon(FluentIconType::Document), DEMO_TEXT("文档", "Documents"));) \
    X(addDocumentTab->setIcon(FluentIcon::icon(FluentIconType::Add));) \
    X(addDocumentTab->setFixedSize(34, 34);) \
    X(documentTabs->setCornerWidget(addDocumentTab, Qt::TopRightCorner);) \
    X(QObject::connect(documentTabs, &QTabWidget::tabCloseRequested, documentTabs, [=](int index) { if (documentTabs->count() <= 1) return; QWidget *page = documentTabs->widget(index); documentTabs->removeTab(index); if (page) page->deleteLater(); });) \
    X(QObject::connect(addDocumentTab, &QToolButton::clicked, documentTabs, [=]() { const int n = documentTabs->count() + 1; auto *page = new FluentLabel(DEMO_TEXT("新标签内容", "New tab content")); const int index = documentTabs->addTab(page, FluentIcon::icon(FluentIconType::Folder), DEMO_TEXT("新建标签%1", "New tab %1").arg(n)); documentTabs->setCurrentIndex(index); });) \
    X(body->addWidget(tabs);) \
    X(body->addWidget(documentTabs);)

#define X(line) code += QStringLiteral(#line "\n");
            CONTAINERS_TABS(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentTabWidget"),
                DEMO_TEXT("标签页容器（切换动画/主题联动）", "Tab container with switch animation and theme linkage"),
                DEMO_TEXT("要点：\n"
                          "-addTab(widget, title) 添加页面\n"
                          "-默认是底部线标识的轻量模式\n"
                          "-setContentMargin(5) 控制 pane 内边距，默认 5px\n"
                          "-setTabDisplayMode(Document) 可切换为标签式模式\n"
                          "-Document 模式支持 setTabsClosable(true)、setMovable(true) 与贴在最后一个标签右侧的 cornerWidget 加号入口",
                          "Highlights:\n"
                          "-Use addTab(widget, title) to add pages\n"
                          "-The default mode uses a lightweight underline indicator\n"
                          "-Use setContentMargin(5) to control pane padding; the default is 5px\n"
                          "-Use setTabDisplayMode(Document) for document-like tabs\n"
                          "-Document mode supports setTabsClosable(true), setMovable(true), and a cornerWidget add button positioned beside the last tab"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    CONTAINERS_TABS(X)
#undef X
                },
                true,
                275));

#undef CONTAINERS_TABS
        }

        // NavigationView
        {
            const QString code = QStringLiteral(R"CPP(#include "Fluent/FluentNavigationView.h"
#include "Fluent/FluentIcon.h"

#include <QMap>
#include <QStringList>
#include <functional>
#include <memory>

using NI = Fluent::FluentNavigationItem;

auto *nav = new Fluent::FluentNavigationView();
nav->setExpandedWidth(220);
nav->setCompactWidth(48);
nav->setPaneDisplayMode(Fluent::FluentNavigationView::Left);
nav->setPaneTitle(QStringLiteral("Pane Title"));
nav->setBackButtonVisible(false);

auto applyIcon = [](NI &item, Fluent::FluentIconType type) {
    item.hasFluentIcon = true;
    item.fluentIcon = type;
};

// ---------------------------------------------------------------------------
// 1) Base tree (static items). children 现在支持任意深度嵌套。
// ---------------------------------------------------------------------------
NI home;
home.key = QStringLiteral("home");
home.text = QStringLiteral("Home");
applyIcon(home, Fluent::FluentIconType::Home);

NI documents;
documents.key = QStringLiteral("documents");
documents.text = QStringLiteral("Document options");
documents.selectsOnInvoked = false;
applyIcon(documents, Fluent::FluentIconType::Folder);
{
    NI recent;
    recent.key = QStringLiteral("recent_files");
    recent.text = QStringLiteral("Recent Files");
    applyIcon(recent, Fluent::FluentIconType::More);
    documents.children.push_back(recent);
}

NI help;
help.key = QStringLiteral("help_center");
help.text = QStringLiteral("Help Center");
applyIcon(help, Fluent::FluentIconType::Info);

// ---------------------------------------------------------------------------
// 2) Dynamic-extras layer: per-parent extra children appended at runtime.
//    Keying by parent.key lets us merge anywhere in the tree, at any depth.
// ---------------------------------------------------------------------------
auto dynamicExtras = std::make_shared<QMap<QString, std::vector<NI>>>();
auto dynamicCounter = std::make_shared<int>(0);

auto rebuildNavigation = [=]() {
    std::vector<NI> items{ home, documents };

    // Recursively merge runtime additions into the tree.
    std::function<void(std::vector<NI> &)> mergeDynamic;
    mergeDynamic = [&](std::vector<NI> &nodes) {
        for (auto &node : nodes) {
            const auto it = dynamicExtras->constFind(node.key);
            if (it != dynamicExtras->constEnd()) {
                for (const auto &extra : it.value()) {
                    node.children.push_back(extra);
                }
            }
            if (!node.children.empty()) {
                mergeDynamic(node.children);
            }
        }
    };
    mergeDynamic(items);

    nav->setItems(items);
    nav->clearFooterItems();
    nav->addFooterItem(help);
};

rebuildNavigation();
nav->setSelectedKey(QStringLiteral("home"));

auto backStack = std::make_shared<QStringList>();
auto currentKey = std::make_shared<QString>(nav->selectedKey());
auto applyingBack = std::make_shared<bool>(false);
auto updateBackState = [=]() {
    nav->setBackButtonEnabled(!backStack->isEmpty());
};
updateBackState();

QObject::connect(nav, &Fluent::FluentNavigationView::selectedKeyChanged,
                 nav, [=](const QString &key) {
    if (*applyingBack) {
        *applyingBack = false;
    } else if (!currentKey->isEmpty() && *currentKey != key) {
        backStack->append(*currentKey);
    }
    *currentKey = key;
    updateBackState();
});

QObject::connect(nav, &Fluent::FluentNavigationView::backRequested, nav, [=]() {
    if (backStack->isEmpty()) {
        updateBackState();
        return;
    }

    *applyingBack = true;
    nav->setSelectedKey(backStack->takeLast());
    updateBackState();
});

// ---------------------------------------------------------------------------
// 3) UI button: append a new child under whichever item is selected. Works
//    at any depth because rebuildNavigation() walks the tree recursively.
//    After insertion, setSelectedKey() auto-expands every ancestor so the
//    new node is immediately visible.
// ---------------------------------------------------------------------------
auto *addChildButton = new Fluent::FluentButton(QStringLiteral("Add child to selected"));
QObject::connect(addChildButton, &QAbstractButton::clicked, nav, [=]() {
    QString parentKey = nav->selectedKey();
    if (parentKey.isEmpty()) {
        parentKey = QStringLiteral("home");
    }

    const int n = ++(*dynamicCounter);
    NI child;
    child.key = QStringLiteral("dyn_%1").arg(n);
    child.text = QStringLiteral("Dynamic child %1").arg(n);
    applyIcon(child, Fluent::FluentIconType::Add);

    (*dynamicExtras)[parentKey].push_back(child);
    rebuildNavigation();
    nav->setSelectedKey(child.key);
});

// Optional: reset all runtime additions back to the static tree.
auto *resetButton = new Fluent::FluentButton(QStringLiteral("Reset dynamic children"));
QObject::connect(resetButton, &QAbstractButton::clicked, nav, [=]() {
    dynamicExtras->clear();
    *dynamicCounter = 0;
    rebuildNavigation();
    nav->setSelectedKey(QStringLiteral("home"));
});

// ---------------------------------------------------------------------------
// 4) Overflow + scrolling: NavigationView paints its own vertical scrollbar
//    automatically when items don't fit. setSelectedKey() also scrolls the
//    selected row back into the viewport, so deeply nested or far-down items
//    never get lost.
// ---------------------------------------------------------------------------
auto *bulkAddButton = new Fluent::FluentButton(QStringLiteral("Bulk-add 12 children (triggers scrolling)"));
QObject::connect(bulkAddButton, &QAbstractButton::clicked, nav, [=]() {
    QString parentKey = nav->selectedKey();
    if (parentKey.isEmpty()) parentKey = QStringLiteral("home");

    for (int i = 0; i < 12; ++i) {
        const int n = ++(*dynamicCounter);
        NI child;
        child.key = QStringLiteral("dyn_%1").arg(n);
        child.text = QStringLiteral("Dynamic child %1").arg(n);
        applyIcon(child, Fluent::FluentIconType::Add);
        (*dynamicExtras)[parentKey].push_back(child);
    }
    rebuildNavigation();
    nav->setSelectedKey(parentKey); // keep parent selected; scrollbar appears
});

// Demonstrates auto-scroll-into-view: selecting any key (including a row that
// is currently below the fold or inside a collapsed ancestor) scrolls it back
// into the visible area after expanding all ancestors.
auto *scrollToFirstButton = new Fluent::FluentButton(QStringLiteral("Jump to first dynamic child"));
QObject::connect(scrollToFirstButton, &QAbstractButton::clicked, nav, [=]() {
    nav->setSelectedKey(QStringLiteral("dyn_1"));
});

QObject::connect(nav, &Fluent::FluentNavigationView::selectedKeyChanged,
                 nav, [](const QString &key) {
    qDebug() << "selected:" << key;
});
)CPP");

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentNavigationView"),
                DEMO_TEXT("WinUI3 风格导航栏：支持 Left / LeftCompact / Top 与显式 footer API", "WinUI3-style navigation bar with Left / LeftCompact / Top modes and an explicit footer API"),
                DEMO_TEXT("要点：\n"
                          "-setPaneDisplayMode() 在 Left / LeftCompact / Top 三种布局间切换\n"
                          "-父项可通过 selectsOnInvoked 控制“点击即选中”还是“只打开子菜单”\n"
                          "-backRequested / itemInvoked / selectedKeyChanged 可分别接回退、点击与选中逻辑；本示例维护一个简单返回栈\n"
                          "-Footer 通过 addFooterItem() 显式添加，不再暗示内置设置页\n"
                          "-children 支持任意深度嵌套，setSelectedKey() 会自动展开所有祖先；点击右侧“追加子项到选中项”可动态构建多层结构\n"
                          "-导航项超出可视区域时自动出现垂直滚动条（鼠标滚轮 / 拖动滑块 / 点击轨道翻页均可），调用 setSelectedKey() 还会自动把选中项滚动到可见区域",
                          "Highlights:\n"
                          "-Use setPaneDisplayMode() to switch between Left, LeftCompact, and Top layouts\n"
                          "-Parent items can use selectsOnInvoked to choose between select-on-click and submenu-only behavior\n"
                          "-backRequested, itemInvoked, and selectedKeyChanged separate back, invoke, and selection logic; this sample keeps a small back stack\n"
                          "-Footer entries are added explicitly through addFooterItem() instead of implying a built-in settings page\n"
                          "-children now supports arbitrarily deep nesting; setSelectedKey() auto-expands every ancestor. Use the 'Add child to selected' button on the right to build multi-level structures at runtime\n"
                          "-A vertical scrollbar appears automatically when items overflow the pane (mouse wheel, thumb drag, and track paging are all supported), and setSelectedKey() also auto-scrolls the row into view"),
                code,
                [=](QVBoxLayout *body) {
                    auto *shell = new QWidget();
                    shell->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
                    auto *shellLayout = new QHBoxLayout(shell);
                    shellLayout->setContentsMargins(0, 0, 0, 0);
                    shellLayout->setSpacing(12);
                    shellLayout->setAlignment(Qt::AlignTop);

                    auto *previewHost = new QWidget(shell);
                    previewHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
                    auto *previewGrid = new QGridLayout(previewHost);
                    previewGrid->setContentsMargins(0, 0, 0, 0);
                    previewGrid->setSpacing(12);

                    auto *nav = new FluentNavigationView(previewHost);
                    nav->setExpandedWidth(220);
                    nav->setCompactWidth(48);
                    nav->setPaneDisplayMode(FluentNavigationView::Left);
                    nav->setPaneTitle(QStringLiteral("Pane Title"));
                    nav->setBackButtonVisible(false);
                    nav->setBackButtonEnabled(true);
                    nav->loadPaneToggleAnimation(Demo::demoLottieResourcePath(QStringLiteral("menu.json")));
                    nav->loadBackButtonAnimation(Demo::demoLottieResourcePath(QStringLiteral("left-arrow.json")));

                    auto applyIcon = [](FluentNavigationItem &item, FluentIconType type) {
                        item.hasFluentIcon = true;
                        item.fluentIcon = type;
                    };

                    using NI = FluentNavigationItem;

                    // Dynamic-children state: any extras pushed in here are merged on
                    // top of the static tree every time rebuildNavigation() runs, so
                    // the user can keep stacking nested levels at runtime.
                    auto dynamicExtras = std::make_shared<QMap<QString, std::vector<NI>>>();
                    auto dynamicCounter = std::make_shared<int>(0);

                    auto *detailCard = new FluentCard();
                    detailCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
                    detailCard->setMinimumHeight(336);
                    auto *detailLayout = new QVBoxLayout(detailCard);
                    detailLayout->setContentsMargins(18, 18, 18, 18);
                    detailLayout->setSpacing(10);

                    auto *detailTitle = new FluentLabel();
                    detailTitle->setStyleSheet("font-size: 24px; font-weight: 700;");
                    auto *detailBody = new FluentLabel();
                    detailBody->setWordWrap(true);
                    detailBody->setStyleSheet("font-size: 12px; opacity: 0.9;");
                    auto *detailState = new FluentLabel();
                    detailState->setWordWrap(true);
                    detailState->setStyleSheet("font-size: 12px; opacity: 0.82;");
                    auto *detailEvent = new FluentLabel();
                    detailEvent->setWordWrap(true);
                    detailEvent->setStyleSheet("font-size: 12px; opacity: 0.82;");

                    auto *blocks = new QWidget(detailCard);
                    auto *blocksLayout = new QGridLayout(blocks);
                    blocksLayout->setContentsMargins(0, 0, 0, 0);
                    blocksLayout->setSpacing(10);
                    blocksLayout->addWidget(makeNavigationPreviewBlock(160, 0.16, 0.72, true), 0, 0, 2, 1);
                    blocksLayout->addWidget(makeNavigationPreviewBlock(74, 0.20, 0.74), 0, 1);
                    blocksLayout->addWidget(makeNavigationPreviewBlock(74, 0.36, 0.80), 0, 2);
                    blocksLayout->addWidget(makeNavigationPreviewBlock(74, 0.28, 0.78), 1, 1);
                    blocksLayout->addWidget(makeNavigationPreviewBlock(74, 0.48, 0.84), 1, 2);
                    blocksLayout->setColumnStretch(1, 1);
                    blocksLayout->setColumnStretch(2, 1);

                    auto *detailHint = new FluentLabel(DEMO_TEXT("切换右侧 Pane 模式观察 Left / LeftCompact / Top 三种布局；把“文档组选中父项”关闭后，点击 Document options 只会打开子菜单。", "Switch the pane mode on the right to compare Left / LeftCompact / Top layouts. When 'Select parent item for Documents' is off, clicking Document options only opens the submenu."));
                    detailHint->setWordWrap(true);
                    detailHint->setStyleSheet("font-size: 12px; opacity: 0.78;");
                    detailLayout->addWidget(detailTitle);
                    detailLayout->addWidget(blocks);
                    detailLayout->addWidget(detailBody);
                    detailLayout->addWidget(detailState);
                    detailLayout->addWidget(detailEvent);
                    detailLayout->addStretch(1);
                    detailLayout->addWidget(detailHint);

                    auto *optionsCard = new FluentCard(shell);
                    optionsCard->setFixedWidth(260);
                    auto *optionsLayout = new QVBoxLayout(optionsCard);
                    optionsLayout->setContentsMargins(16, 16, 16, 16);
                    optionsLayout->setSpacing(10);

                    auto *optionsTitle = new FluentLabel(QStringLiteral("API in action"));
                    optionsTitle->setStyleSheet("font-size: 14px; font-weight: 600;");
                    auto *paneModeLabel = new FluentLabel(QStringLiteral("Pane mode"));
                    paneModeLabel->setStyleSheet("font-size: 12px; opacity: 0.8;");
                    auto *leftRadio = new FluentRadioButton(QStringLiteral("Left"));
                    auto *compactRadio = new FluentRadioButton(QStringLiteral("LeftCompact"));
                    auto *topRadio = new FluentRadioButton(QStringLiteral("Top"));
                    leftRadio->setChecked(true);

                    auto *backVisibleToggle = new FluentToggleSwitch(QStringLiteral("Back button visible"));
                    backVisibleToggle->setChecked(false);
                    auto *backEnabledToggle = new FluentToggleSwitch(QStringLiteral("Back button enabled"));
                    backEnabledToggle->setChecked(true);
                    auto *footerVisibleToggle = new FluentToggleSwitch(QStringLiteral("Footer visible"));
                    footerVisibleToggle->setChecked(true);
                    auto *documentInvokesToggle = new FluentToggleSwitch(DEMO_TEXT("Document组选中父项", "Select parent item for Documents"));
                    documentInvokesToggle->setChecked(false);
                    auto *paneTitleLabel = new FluentLabel(QStringLiteral("Pane title"));
                    paneTitleLabel->setStyleSheet("font-size: 12px; opacity: 0.8;");
                    auto *paneTitleEdit = new FluentLineEdit();
                    paneTitleEdit->setText(QStringLiteral("Pane Title"));
                    paneTitleEdit->setPlaceholderText(QStringLiteral("Pane Title"));

                    auto *dynamicLabel = new FluentLabel(DEMO_TEXT("多级嵌套（点击向当前选中项追加子项）",
                                                                    "Multi-level nesting (append a child to the current selection)"));
                    dynamicLabel->setWordWrap(true);
                    dynamicLabel->setStyleSheet("font-size: 12px; opacity: 0.8;");
                    auto *addChildButton = new FluentButton(DEMO_TEXT("追加子项到选中项", "Add child to selected"));
                    auto *bulkAddButton = new FluentButton(DEMO_TEXT("批量追加 12 个子项（触发滚动）",
                                                                     "Bulk-add 12 children (triggers scrolling)"));
                    auto *scrollToFirstButton = new FluentButton(DEMO_TEXT("跳转到首个动态子项（自动滚动可见）",
                                                                           "Jump to first dynamic child (auto-scroll)"));
                    auto *resetDynamicButton = new FluentButton(DEMO_TEXT("清除追加的子项", "Reset dynamic children"));

                    optionsLayout->addWidget(optionsTitle);
                    optionsLayout->addWidget(paneModeLabel);
                    optionsLayout->addWidget(leftRadio);
                    optionsLayout->addWidget(compactRadio);
                    optionsLayout->addWidget(topRadio);
                    optionsLayout->addSpacing(6);
                    optionsLayout->addWidget(backVisibleToggle);
                    optionsLayout->addWidget(backEnabledToggle);
                    optionsLayout->addWidget(footerVisibleToggle);
                    optionsLayout->addWidget(documentInvokesToggle);
                    optionsLayout->addSpacing(6);
                    optionsLayout->addWidget(paneTitleLabel);
                    optionsLayout->addWidget(paneTitleEdit);
                    optionsLayout->addSpacing(6);
                    optionsLayout->addWidget(dynamicLabel);
                    optionsLayout->addWidget(addChildButton);
                    optionsLayout->addWidget(bulkAddButton);
                    optionsLayout->addWidget(scrollToFirstButton);
                    optionsLayout->addWidget(resetDynamicButton);
                    optionsLayout->addStretch(1);

                    const auto modeName = [](FluentNavigationView::PaneDisplayMode mode) {
                        switch (mode) {
                        case FluentNavigationView::Left:
                            return QStringLiteral("Left");
                        case FluentNavigationView::LeftCompact:
                            return QStringLiteral("LeftCompact");
                        case FluentNavigationView::Top:
                            return QStringLiteral("Top");
                        }
                        return QStringLiteral("Left");
                    };

                    auto navigationBackStack = std::make_shared<QStringList>();
                    auto navigationCurrentKey = std::make_shared<QString>();
                    auto applyingBackNavigation = std::make_shared<bool>(false);
                    const auto updateBackButtonState = [=]() {
                        nav->setBackButtonEnabled(backEnabledToggle->isChecked() && !navigationBackStack->isEmpty());
                    };

                    const auto rebuildNavigation = [=]() {
                        std::vector<NI> items;

                        NI home;
                        home.key = QStringLiteral("home");
                        home.text = QStringLiteral("Home");
                        applyIcon(home, FluentIconType::Home);
                        items.push_back(home);

                        NI account;
                        account.key = QStringLiteral("account");
                        account.text = QStringLiteral("Account");
                        applyIcon(account, FluentIconType::Person);
                        {
                            NI profile;
                            profile.key = QStringLiteral("profile");
                            profile.text = DEMO_TEXT("个人资料", "Profile");
                            applyIcon(profile, FluentIconType::Person);
                            account.children.push_back(profile);

                            NI security;
                            security.key = QStringLiteral("security");
                            security.text = DEMO_TEXT("安全中心", "Security Center");
                            applyIcon(security, FluentIconType::Lock);
                            account.children.push_back(security);
                        }
                        items.push_back(account);

                        NI documents;
                        documents.key = QStringLiteral("documents");
                        documents.text = QStringLiteral("Document options");
                        documents.selectsOnInvoked = documentInvokesToggle->isChecked();
                        applyIcon(documents, FluentIconType::Folder);
                        {
                            NI recent;
                            recent.key = QStringLiteral("recent_files");
                            recent.text = DEMO_TEXT("最近文件", "Recent Files");
                            applyIcon(recent, FluentIconType::More);
                            documents.children.push_back(recent);

                            NI templates;
                            templates.key = QStringLiteral("templates");
                            templates.text = DEMO_TEXT("模板库", "Template Library");
                            applyIcon(templates, FluentIconType::Layout);
                            documents.children.push_back(templates);
                        }
                        items.push_back(documents);

                        // Merge runtime-added children into the tree recursively so
                        // dynamic levels survive every rebuild.
                        std::function<void(std::vector<NI> &)> mergeDynamic;
                        mergeDynamic = [&](std::vector<NI> &nodes) {
                            for (auto &node : nodes) {
                                const auto it = dynamicExtras->constFind(node.key);
                                if (it != dynamicExtras->constEnd()) {
                                    for (const auto &extra : it.value()) {
                                        node.children.push_back(extra);
                                    }
                                }
                                if (!node.children.empty()) {
                                    mergeDynamic(node.children);
                                }
                            }
                        };
                        mergeDynamic(items);

                        nav->setItems(items);
                        nav->clearFooterItems();

                        NI help;
                        help.key = QStringLiteral("help_center");
                        help.text = DEMO_TEXT("帮助中心", "Help Center");
                        applyIcon(help, FluentIconType::Info);
                        nav->addFooterItem(help);

                        NI feedback;
                        feedback.key = QStringLiteral("feedback");
                        feedback.text = DEMO_TEXT("反馈", "Feedback");
                        applyIcon(feedback, FluentIconType::Mail);
                        nav->addFooterItem(feedback);

                        if (nav->selectedKey().isEmpty()) {
                            nav->setSelectedKey(QStringLiteral("home"));
                        }
                    };

                    const auto relayoutPreview = [=]() {
                        previewGrid->removeWidget(nav);
                        previewGrid->removeWidget(detailCard);
                        previewGrid->setColumnStretch(0, 0);
                        previewGrid->setColumnStretch(1, 0);
                        previewGrid->setRowStretch(0, 0);
                        previewGrid->setRowStretch(1, 0);

                        if (nav->paneDisplayMode() == FluentNavigationView::Top) {
                            previewGrid->addWidget(nav, 0, 0, 1, 2);
                            previewGrid->addWidget(detailCard, 1, 0, 1, 2);
                            previewGrid->setColumnStretch(0, 1);
                            previewGrid->setRowStretch(1, 1);
                        } else {
                            previewGrid->addWidget(nav, 0, 0);
                            previewGrid->addWidget(detailCard, 0, 1);
                            previewGrid->setColumnStretch(1, 1);
                            previewGrid->setRowStretch(0, 1);
                        }
                    };

                    const auto updateDetail = [=]() {
                        const QString key = nav->selectedKey();
                        if (key == QStringLiteral("home")) {
                            detailTitle->setText(QStringLiteral("Sample Page 1"));
                            detailBody->setText(DEMO_TEXT("首页适合映射欢迎页或仪表板。当前示例右侧这块内容不依赖具体布局，所以同一份页面可以随 NavigationView 在 Left / LeftCompact / Top 三种模式间切换。", "The home item is a good fit for a welcome page or dashboard. The content block on the right is layout-agnostic, so the same page can switch with NavigationView across Left, LeftCompact, and Top modes."));
                        } else if (key == QStringLiteral("account")) {
                            detailTitle->setText(QStringLiteral("Account"));
                            detailBody->setText(DEMO_TEXT("父项默认仍可点击选中；如果在 Top 或 LeftCompact 模式下点击它，也会同时弹出子菜单，接近 WinUI3 的层级导航体验。", "Parent items remain selectable by default. In Top or LeftCompact mode, clicking them can also open the submenu for a WinUI3-like hierarchical navigation experience."));
                        } else if (key == QStringLiteral("profile")) {
                            detailTitle->setText(DEMO_TEXT("个人资料", "Profile"));
                            detailBody->setText(DEMO_TEXT("选中子项时，Left 模式会自动展开父组；Top 模式则保持父项高亮，并通过 flyout 进入具体页面。", "Selecting a child item auto-expands the parent group in Left mode. In Top mode, the parent stays highlighted and the flyout routes into the specific page."));
                        } else if (key == QStringLiteral("security")) {
                            detailTitle->setText(DEMO_TEXT("安全中心", "Security Center"));
                            detailBody->setText(DEMO_TEXT("itemInvoked 和 selectedKeyChanged 被拆开后，可以分别处理“点击动作”和“选中状态”，便于接导航、分析埋点或只展开不选中的父项。", "Once itemInvoked and selectedKeyChanged are separated, you can handle invoke behavior and selection state independently. That helps with routing, analytics, or parent items that expand without selecting."));
                        } else if (key == QStringLiteral("documents")) {
                            detailTitle->setText(QStringLiteral("Document options"));
                            detailBody->setText(documentInvokesToggle->isChecked()
                                                    ? DEMO_TEXT("当前开启“选中父项”。点击正文会选中 Document options，点击箭头或 Top 模式下的下拉区域则打开子菜单。", "Parent selection is currently enabled. Clicking the body selects Document options, while the chevron or the Top-mode dropdown area opens the submenu.")
                                                    : DEMO_TEXT("当前关闭“选中父项”。点击 Document options 只会打开子菜单，不会修改 selectedKey，这和 WinUI3 文档里的 submenu-only 行为一致。", "Parent selection is currently disabled. Clicking Document options only opens the submenu and leaves selectedKey unchanged, which matches WinUI3's submenu-only behavior."));
                        } else if (key == QStringLiteral("recent_files")) {
                            detailTitle->setText(DEMO_TEXT("最近文件", "Recent Files"));
                            detailBody->setText(DEMO_TEXT("这类真正落到页面的叶子节点通常保持 selectsOnInvoked = true；如果你只想拿点击回调而不切换当前页，也可以单独把叶子节点的 selectsOnInvoked 设为 false。", "Leaf items that map to real pages usually keep selectsOnInvoked = true. If you only need the invoke callback without changing the current page, you can turn selectsOnInvoked off on the leaf item as well."));
                        } else if (key == QStringLiteral("templates")) {
                            detailTitle->setText(DEMO_TEXT("模板库", "Template Library"));
                            detailBody->setText(DEMO_TEXT("Top 模式下，当前高亮会挂在父级顶部 item 上；Left / LeftCompact 模式则保持与当前 key 一致的视觉反馈。", "In Top mode, the active highlight stays on the parent top item. In Left and LeftCompact modes, the feedback stays aligned with the current key."));
                        } else if (key == QStringLiteral("help_center")) {
                            detailTitle->setText(DEMO_TEXT("帮助中心", "Help Center"));
                            detailBody->setText(DEMO_TEXT("这里的 footer 是通过 addFooterItem() 显式加入的普通导航项，不再默认绑定成“设置页”。你可以自由放帮助、账户、反馈或关于入口。", "The footer here is made of ordinary navigation items added explicitly through addFooterItem(). It is no longer implicitly bound to a built-in settings page. You can place help, account, feedback, or about entries freely."));
                        } else if (key == QStringLiteral("feedback")) {
                            detailTitle->setText(DEMO_TEXT("反馈", "Feedback"));
                            detailBody->setText(DEMO_TEXT("Footer 在 Left / LeftCompact 模式下会固定到底部，在 Top 模式下会挪到右侧；也可以通过 setFooterVisible(false) 整体隐藏。", "The footer stays pinned to the bottom in Left and LeftCompact modes, and moves to the right side in Top mode. You can also hide it entirely with setFooterVisible(false)."));
                        } else if (key.startsWith(QStringLiteral("dyn_"))) {
                            detailTitle->setText(DEMO_TEXT("动态子项 %1", "Dynamic child %1").arg(key.mid(4)));
                            detailBody->setText(DEMO_TEXT(
                                "这是运行时通过 “追加子项到选中项” 按钮新增的节点。可以在任意层级继续追加，FluentNavigationView 现在支持任意深度的嵌套，"
                                "并会在 setSelectedKey() 时自动展开所有祖先。",
                                "This node was appended at runtime through the 'Add child to selected' button. You can keep appending at any depth — "
                                "FluentNavigationView now supports arbitrarily nested children and automatically expands all ancestors when setSelectedKey() is called."));
                        } else {
                            detailTitle->setText(QStringLiteral("NavigationView"));
                            detailBody->setText(DEMO_TEXT("使用 addItem / addFooterItem 追加导航项，使用 paneDisplayMode / backRequested / itemInvoked 组织不同布局和交互。", "Use addItem() and addFooterItem() to append navigation entries, then use paneDisplayMode, backRequested, and itemInvoked to organize layout and interaction."));
                        }

                        detailState->setText(DEMO_TEXT("当前 key：%1\nPane 模式：%2\n文档组选中父项：%3\nFooter：%4（通过 addFooterItem() 显式追加）\n返回栈：%5 项", "Current key: %1\nPane mode: %2\nSelect parent item for Documents: %3\nFooter: %4 (added explicitly via addFooterItem())\nBack stack: %5 item(s)")
                                                 .arg(key.isEmpty() ? QStringLiteral("<empty>") : key)
                                                 .arg(modeName(nav->paneDisplayMode()))
                                                 .arg(documentInvokesToggle->isChecked() ? DEMO_TEXT("开启", "On") : DEMO_TEXT("关闭", "Off"))
                                                 .arg(nav->isFooterVisible() ? DEMO_TEXT("可见", "Visible") : DEMO_TEXT("隐藏", "Hidden"))
                                                 .arg(navigationBackStack->size()));
                    };

                    rebuildNavigation();
                    nav->setSelectedKey(QStringLiteral("home"));
                    *navigationCurrentKey = nav->selectedKey();
                    updateBackButtonState();
                    relayoutPreview();

                    QObject::connect(nav, &FluentNavigationView::selectedKeyChanged, detailCard, [=](const QString &) {
                        detailEvent->setText(DEMO_TEXT("最近事件：selectedKeyChanged -> %1", "Latest event: selectedKeyChanged -> %1").arg(nav->selectedKey()));
                        const QString key = nav->selectedKey();
                        if (*applyingBackNavigation) {
                            *applyingBackNavigation = false;
                        } else if (!navigationCurrentKey->isEmpty() && *navigationCurrentKey != key) {
                            navigationBackStack->append(*navigationCurrentKey);
                        }
                        *navigationCurrentKey = key;
                        updateBackButtonState();
                        updateDetail();
                    });
                    QObject::connect(nav, &FluentNavigationView::itemInvoked, detailCard, [=](const QString &key) {
                        detailEvent->setText(DEMO_TEXT("最近事件：itemInvoked -> %1", "Latest event: itemInvoked -> %1").arg(key));
                    });
                    QObject::connect(nav, &FluentNavigationView::backRequested, detailCard, [=]() {
                        if (navigationBackStack->isEmpty()) {
                            detailEvent->setText(DEMO_TEXT("最近事件：backRequested（没有可返回的历史）", "Latest event: backRequested (no history)"));
                            updateBackButtonState();
                            updateDetail();
                            return;
                        }

                        const QString previousKey = navigationBackStack->takeLast();
                        *applyingBackNavigation = true;
                        nav->setSelectedKey(previousKey);
                        detailEvent->setText(DEMO_TEXT("最近事件：backRequested -> %1", "Latest event: backRequested -> %1").arg(previousKey));
                        updateBackButtonState();
                    });
                    QObject::connect(nav, &FluentNavigationView::paneDisplayModeChanged, detailCard, [=](FluentNavigationView::PaneDisplayMode) {
                        relayoutPreview();
                        updateDetail();
                    });
                    QObject::connect(leftRadio, &QRadioButton::toggled, nav, [=](bool checked) {
                        if (checked) {
                            nav->setPaneDisplayMode(FluentNavigationView::Left);
                        }
                    });
                    QObject::connect(compactRadio, &QRadioButton::toggled, nav, [=](bool checked) {
                        if (checked) {
                            nav->setPaneDisplayMode(FluentNavigationView::LeftCompact);
                        }
                    });
                    QObject::connect(topRadio, &QRadioButton::toggled, nav, [=](bool checked) {
                        if (checked) {
                            nav->setPaneDisplayMode(FluentNavigationView::Top);
                        }
                    });
                    QObject::connect(backVisibleToggle, &FluentToggleSwitch::toggled, nav, [=](bool checked) {
                        nav->setBackButtonVisible(checked);
                    });
                    QObject::connect(backEnabledToggle, &FluentToggleSwitch::toggled, nav, [=](bool checked) {
                        Q_UNUSED(checked)
                        updateBackButtonState();
                    });
                    QObject::connect(footerVisibleToggle, &FluentToggleSwitch::toggled, nav, [=](bool checked) {
                        nav->setFooterVisible(checked);
                        updateDetail();
                    });
                    QObject::connect(documentInvokesToggle, &FluentToggleSwitch::toggled, nav, [=](bool) {
                        const QString currentKey = nav->selectedKey();
                        rebuildNavigation();
                        nav->setSelectedKey(currentKey.isEmpty() ? QStringLiteral("home") : currentKey);
                        updateDetail();
                    });
                    QObject::connect(paneTitleEdit, &QLineEdit::textChanged, nav, [=](const QString &text) {
                        nav->setPaneTitle(text);
                    });

                    QObject::connect(addChildButton, &QAbstractButton::clicked, nav, [=]() {
                        QString parentKey = nav->selectedKey();
                        if (parentKey.isEmpty()) {
                            parentKey = QStringLiteral("home");
                        }

                        const int n = ++(*dynamicCounter);
                        NI child;
                        child.key = QStringLiteral("dyn_%1").arg(n);
                        child.text = DEMO_TEXT("动态子项 %1", "Dynamic child %1").arg(n);
                        applyIcon(child, FluentIconType::Add);

                        (*dynamicExtras)[parentKey].push_back(child);
                        rebuildNavigation();
                        // Select the new node so all ancestors get auto-expanded.
                        nav->setSelectedKey(child.key);
                        detailEvent->setText(DEMO_TEXT("最近事件：在 %1 下追加 %2",
                                                       "Latest event: appended %2 under %1")
                                                 .arg(parentKey, child.key));
                        updateDetail();
                    });

                    QObject::connect(resetDynamicButton, &QAbstractButton::clicked, nav, [=]() {
                        dynamicExtras->clear();
                        *dynamicCounter = 0;
                        rebuildNavigation();
                        navigationBackStack->clear();
                        *applyingBackNavigation = true;
                        nav->setSelectedKey(QStringLiteral("home"));
                        *applyingBackNavigation = false;
                        *navigationCurrentKey = nav->selectedKey();
                        updateBackButtonState();
                        detailEvent->setText(DEMO_TEXT("最近事件：已清除动态追加的子项",
                                                       "Latest event: dynamic children cleared"));
                        updateDetail();
                    });

                    // Bulk-add: stacks 12 children under the current selection in one click
                    // so the items overflow the pane and the vertical scrollbar appears.
                    QObject::connect(bulkAddButton, &QAbstractButton::clicked, nav, [=]() {
                        QString parentKey = nav->selectedKey();
                        if (parentKey.isEmpty()) {
                            parentKey = QStringLiteral("home");
                        }

                        QString lastAddedKey;
                        for (int i = 0; i < 12; ++i) {
                            const int n = ++(*dynamicCounter);
                            NI child;
                            child.key = QStringLiteral("dyn_%1").arg(n);
                            child.text = DEMO_TEXT("动态子项 %1", "Dynamic child %1").arg(n);
                            applyIcon(child, FluentIconType::Add);
                            (*dynamicExtras)[parentKey].push_back(child);
                            lastAddedKey = child.key;
                        }
                        rebuildNavigation();
                        // Select the last added child so the parent group is auto-expanded,
                        // the new items render immediately, and the row is scrolled into
                        // view — together this exercises both new behaviours at once.
                        if (!lastAddedKey.isEmpty()) {
                            nav->setSelectedKey(lastAddedKey);
                        }
                        detailEvent->setText(DEMO_TEXT(
                            "最近事件：批量追加 12 项到 %1，导航栏出现垂直滚动条并自动滚动到新增项",
                            "Latest event: bulk-added 12 children under %1 — scrollbar appears and the new item is scrolled into view")
                                                 .arg(parentKey));
                        updateDetail();
                    });

                    // Demonstrates auto-scroll-into-view: jumps to dyn_1 (which is usually
                    // near the top of an overflowing list, but expanding ancestors may push
                    // it out of the viewport). setSelectedKey() now scrolls it back into view.
                    QObject::connect(scrollToFirstButton, &QAbstractButton::clicked, nav, [=]() {
                        const QString target = QStringLiteral("dyn_1");
                        nav->setSelectedKey(target);
                        detailEvent->setText(DEMO_TEXT(
                            "最近事件：setSelectedKey(\"dyn_1\")，自动展开祖先并把行滚动到可见区域",
                            "Latest event: setSelectedKey(\"dyn_1\") — auto-expanded ancestors and scrolled the row into view"));
                        updateDetail();
                    });

                    updateDetail();
                    detailEvent->setText(DEMO_TEXT("最近事件：itemInvoked -> home", "Latest event: itemInvoked -> home"));

                    shellLayout->addWidget(previewHost, 1);
                    shellLayout->addWidget(optionsCard);
                    body->addWidget(shell);
                },
                false,
                370));
        }

        // ScrollArea
        {
            QString code;
#define CONTAINERS_SCROLLAREA(X) \
    X(auto *area = new FluentScrollArea();) \
    X(area->setOverlayScrollBarsEnabled(true);) \
    X(area->setWidgetResizable(true);) \
    X(area->setFixedHeight(160);) \
    X(auto *scrollContent = new FluentWidget();) \
    X(scrollContent->setBackgroundRole(FluentWidget::BackgroundRole::Transparent);) \
    X(area->setWidget(scrollContent);) \
    X(auto *sl = new QVBoxLayout(scrollContent);) \
    X(sl->setContentsMargins(12, 12, 12, 12);) \
    X(sl->setSpacing(6);) \
    X(for (int i = 1; i <= 18; ++i) { sl->addWidget(new FluentLabel(DEMO_TEXT("滚动内容 %1", "Scrollable content %1").arg(i))); }) \
    X(body->addWidget(area);)

#define X(line) code += QStringLiteral(#line "\n");
            CONTAINERS_SCROLLAREA(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentScrollArea"),
                DEMO_TEXT("滚动区域（可选 overlay 滚动条）", "Scrolling area with optional overlay scrollbars"),
                DEMO_TEXT("要点：\n"
                          "-setOverlayScrollBarsEnabled(true) 显示 overlay 滚动条\n"
                          "-setWidgetResizable(true) 常用配置",
                          "Highlights:\n"
                          "-Use setOverlayScrollBarsEnabled(true) for overlay scrollbars\n"
                          "-setWidgetResizable(true) is a common configuration"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    CONTAINERS_SCROLLAREA(X)
#undef X
                },
                true,
                240));

#undef CONTAINERS_SCROLLAREA
        }

        // AnnotatedScrollBar
        {
            const QString code = QStringLiteral(R"CPP(#include "Fluent/FluentAnnotatedScrollBar.h"
#include "Fluent/FluentScrollArea.h"

#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>

auto *area = new Fluent::FluentScrollArea();
area->setWidgetResizable(true);
area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

auto *content = new QWidget();
auto *contentLayout = new QVBoxLayout(content);
area->setWidget(content);

auto *annotated = new Fluent::FluentAnnotatedScrollBar();
annotated->setScrollArea(area);

auto syncSources = [=]() {
    contentLayout->activate();
    QVector<Fluent::FluentAnnotatedScrollBarSource> sources;
    for (int i = 0; i < contentLayout->count(); ++i) {
        QWidget *section = contentLayout->itemAt(i)->widget();
        if (!section || section->isHidden()) {
            continue;
        }

        const QRect rect = section->geometry();
        sources.append({section->property("sourceGroup").toString(),
                        section->property("sourceText").toString(),
                        rect.top(),
                        rect.bottom()});
    }
    annotated->setSources(sources);
};

QObject::connect(area->verticalScrollBar(), &QScrollBar::rangeChanged, annotated, [=](int, int) {
    QTimer::singleShot(0, annotated, syncSources);
});
QTimer::singleShot(0, annotated, syncSources);
)CPP");

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentAnnotatedScrollBar"),
                QStringLiteral("从内容段自动生成 source 的标注滚动条"),
                QStringLiteral("要点：\n"
                               "-遍历内容卡片 geometry 生成 sources，不手写 start/end\n"
                               "-内容增删后延迟同步，group 会自动合并\n"
                               "-滚动、点击标签、拖拽 thumb 都会触发 currentSourceChanged"),
                code,
                [=](QVBoxLayout *body) {
                    auto *shell = new FluentWidget();
                    shell->setBackgroundRole(FluentWidget::BackgroundRole::Transparent);

                    auto *row = new QHBoxLayout(shell);
                    row->setContentsMargins(0, 0, 0, 0);
                    row->setSpacing(8);

                    auto *area = new FluentScrollArea();
                    area->setWidgetResizable(true);
                    area->setOverlayScrollBarsEnabled(false);
                    area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                    area->setFixedHeight(276);
                    area->setFrameShape(QFrame::NoFrame);

                    auto *content = new FluentWidget();
                    content->setBackgroundRole(FluentWidget::BackgroundRole::Transparent);
                    area->setWidget(content);

                    auto *contentLayout = new QVBoxLayout(content);
                    contentLayout->setContentsMargins(12, 12, 12, 12);
                    contentLayout->setSpacing(10);

                    const QVector<QPair<QString, QString>> items = {
                        {DEMO_TEXT("概览", "Overview"), DEMO_TEXT("欢迎页", "Welcome page")},
                        {DEMO_TEXT("概览", "Overview"), DEMO_TEXT("快速操作", "Quick actions")},
                        {DEMO_TEXT("项目", "Projects"), DEMO_TEXT("项目列表", "Project list")},
                        {DEMO_TEXT("项目", "Projects"), DEMO_TEXT("里程碑", "Milestones")},
                        {DEMO_TEXT("同步", "Sync"), DEMO_TEXT("队列状态", "Queue status")},
                        {DEMO_TEXT("同步", "Sync"), DEMO_TEXT("同步历史", "Sync history")}
                    };
                    const QStringList summaries = {
                        DEMO_TEXT("首页入口、摘要卡片和欢迎提示。", "Home entry, summary cards, and welcome hints."),
                        DEMO_TEXT("常用按钮、最近访问和推荐操作。", "Common actions, recent visits, and recommended operations."),
                        DEMO_TEXT("当前项目、筛选器和统计摘要。", "Current projects, filters, and statistic summaries."),
                        DEMO_TEXT("任务进度、负责人和交付节奏。", "Task progress, owners, and delivery cadence."),
                        DEMO_TEXT("后台同步队列、重试数和实时状态。", "Background sync queue, retries, and live status."),
                        DEMO_TEXT("历史执行记录、完成时间和结果摘要。", "Historical runs, completion times, and result summaries.")
                    };

                    for (int i = 0; i < items.size() && i < summaries.size(); ++i) {
                        contentLayout->addWidget(makeAnnotatedSectionCard(items.at(i).first, items.at(i).second, summaries.at(i)));
                    }

                    auto *status = new FluentLabel(DEMO_TEXT("sources：等待内容同步", "Sources: waiting for content sync"));
                    status->setStyleSheet("font-size: 12px; opacity: 0.82;");

                    auto *annotated = new FluentAnnotatedScrollBar();
                    annotated->setFixedWidth(116);
                    annotated->setToolTipDuration(1100);
                    annotated->setScrollArea(area);

                    const auto updateStatus = [status, annotated]() {
                        const int count = annotated->sources().size();
                        const FluentAnnotatedScrollBarSource source = annotated->currentSource();
                        if (count <= 0) {
                            status->setText(DEMO_TEXT("sources：等待内容同步", "Sources: waiting for content sync"));
                            return;
                        }

                        if (source.group.isEmpty() && source.text.isEmpty()) {
                            status->setText(DEMO_TEXT("sources：%1 段（来自内容卡片）", "Sources: %1 sections (from content cards)").arg(count));
                            return;
                        }

                        status->setText(DEMO_TEXT("当前 source：%1 / %2（%3 段自动同步）",
                                                  "Current source: %1 / %2 (%3 auto-synced)")
                                            .arg(source.group, source.text)
                                            .arg(count));
                    };

                    const auto syncSources = [=]() {
                        if (!contentLayout || !annotated) {
                            return;
                        }

                        contentLayout->activate();

                        QVector<FluentAnnotatedScrollBarSource> sources;
                        const QMargins margins = contentLayout->contentsMargins();
                        int fallbackTop = margins.top();
                        for (int i = 0; i < contentLayout->count(); ++i) {
                            QWidget *section = contentLayout->itemAt(i) ? contentLayout->itemAt(i)->widget() : nullptr;
                            if (!section || section->isHidden()) {
                                continue;
                            }

                            const QRect geometry = section->geometry();
                            const int hintHeight = qMax(1, section->sizeHint().height());
                            const bool hasGeometry = geometry.width() > 0 && geometry.height() > 0;
                            const int sourceStart = hasGeometry ? qMax(0, section->mapTo(content, QPoint(0, 0)).y())
                                                                : qMax(0, fallbackTop);
                            const int sectionHeight = hasGeometry ? qMax(1, section->height()) : hintHeight;

                            FluentAnnotatedScrollBarSource source;
                            source.group = section->property("sourceGroup").toString();
                            source.text = section->property("sourceText").toString();
                            source.start = sourceStart;
                            source.end = source.start + sectionHeight - 1;
                            sources.append(source);

                            fallbackTop = source.start + sectionHeight + contentLayout->spacing();
                        }

                        annotated->setSources(sources);
                        annotated->setVisible(sources.size() > 1);
                        updateStatus();
                    };

                    const auto scheduleSync = [area, syncSources]() {
                        for (int delay : {0, 16, 48}) {
                            QTimer::singleShot(delay, area, syncSources);
                        }
                    };

                    QObject::connect(area->verticalScrollBar(), &QScrollBar::rangeChanged, area, [scheduleSync](int, int) {
                        scheduleSync();
                    });
                    QObject::connect(annotated, &FluentAnnotatedScrollBar::currentSourceChanged, status,
                                     [updateStatus](int, const QString &, const QString &) {
                        updateStatus();
                    });
                    QObject::connect(annotated, &FluentAnnotatedScrollBar::sourcesChanged, status, updateStatus);

                    auto *controls = new QWidget();
                    auto *controlsLayout = new QHBoxLayout(controls);
                    controlsLayout->setContentsMargins(0, 0, 0, 0);
                    controlsLayout->setSpacing(8);
                    auto *addSectionButton = new FluentButton(DEMO_TEXT("追加内容段", "Add section"));
                    auto *removeSectionButton = new FluentButton(DEMO_TEXT("移除新增段", "Remove added"));
                    controlsLayout->addWidget(addSectionButton);
                    controlsLayout->addWidget(removeSectionButton);
                    controlsLayout->addStretch(1);

                    auto dynamicSourceCount = std::make_shared<int>(0);
                    QObject::connect(addSectionButton, &QAbstractButton::clicked, area, [=]() {
                        const int n = ++(*dynamicSourceCount);
                        const QString group = (n % 2 == 0) ? DEMO_TEXT("动态", "Dynamic") : DEMO_TEXT("同步", "Sync");
                        const QString title = DEMO_TEXT("新增段 %1", "New section %1").arg(n);
                        contentLayout->addWidget(makeAnnotatedSectionCard(
                            group,
                            title,
                            DEMO_TEXT("运行时追加的内容，source 区间会由布局几何重新计算。",
                                      "Runtime content; source ranges are recalculated from layout geometry.")));
                        scheduleSync();
                    });
                    QObject::connect(removeSectionButton, &QAbstractButton::clicked, area, [=]() {
                        if (contentLayout->count() <= items.size()) {
                            return;
                        }

                        QLayoutItem *item = contentLayout->takeAt(contentLayout->count() - 1);
                        if (item) {
                            if (QWidget *widget = item->widget()) {
                                widget->deleteLater();
                            }
                            delete item;
                        }
                        if (*dynamicSourceCount > 0) {
                            --(*dynamicSourceCount);
                        }
                        scheduleSync();
                    });

                    scheduleSync();

                    row->addWidget(area, 1);
                    row->addWidget(annotated);
                    body->addWidget(shell);
                    body->addWidget(status);
                    body->addWidget(controls);
                },
                true,
                320));
        }

        // Splitter
        {
            QString code;
#define CONTAINERS_SPLITTER(X) \
    X(auto *root = new FluentWidget();) \
    X(root->setBackgroundRole(FluentWidget::BackgroundRole::Transparent);) \
    X(auto *l = new QVBoxLayout(root);) \
    X(l->setContentsMargins(0, 0, 0, 0);) \
    X(l->setSpacing(10);) \
    X(auto *h = new FluentSplitter(Qt::Horizontal);) \
    X(h->setFixedHeight(120);) \
    X(h->addWidget(Demo::makeSidebarCard(new FluentLabel(DEMO_TEXT("左侧", "Left"))));) \
    X(h->addWidget(Demo::makeSidebarCard(new FluentLabel(DEMO_TEXT("右侧", "Right"))));) \
    X(h->setSizes({320, 520});) \
    X(auto *v = new FluentSplitter(Qt::Vertical);) \
    X(v->setFixedHeight(160);) \
    X(v->addWidget(Demo::makeSidebarCard(new FluentLabel(DEMO_TEXT("上方", "Top"))));) \
    X(v->addWidget(Demo::makeSidebarCard(new FluentLabel(DEMO_TEXT("下方", "Bottom"))));) \
    X(v->setSizes({90, 70});) \
    X(l->addWidget(h);) \
    X(l->addWidget(v);) \
    X(body->addWidget(root);)

#define X(line) code += QStringLiteral(#line "\n");
            CONTAINERS_SPLITTER(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentSplitter"),
                DEMO_TEXT("可拖拽分隔的布局容器（横向/纵向）", "Draggable splitter layout container in horizontal or vertical orientation"),
                DEMO_TEXT("要点：\n"
                          "-横向：Qt::Horizontal，拖拽竖向分隔条调整左右宽度\n"
                          "-纵向：Qt::Vertical，拖拽横向分隔条调整上下高度\n"
                          "-适合可调整的面板布局（预览/设置/属性面板等）",
                          "Highlights:\n"
                          "-Horizontal: Qt::Horizontal lets you drag the vertical splitter to resize left and right panes\n"
                          "-Vertical: Qt::Vertical lets you drag the horizontal splitter to resize top and bottom panes\n"
                          "-A good fit for adjustable preview, settings, or properties layouts"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    CONTAINERS_SPLITTER(X)
#undef X
                },
                true,
                220));

#undef CONTAINERS_SPLITTER
        }

        // FlowLayout
        {
            QString code;

#define CONTAINERS_FLOWLAYOUT(X) \
    X(auto *hs = new FluentSplitter(Qt::Horizontal);) \
    X(hs->setChildrenCollapsible(false);) \
    X(auto *ctrl = new FluentWidget();) \
    X(ctrl->setBackgroundRole(FluentWidget::BackgroundRole::Transparent);) \
    X(ctrl->setMinimumWidth(160);) \
    X(auto *ctrlL = new QVBoxLayout(ctrl);) \
    X(ctrlL->setContentsMargins(0, 0, 0, 0);) \
    X(ctrlL->setSpacing(8);) \
    X(auto *sizeLabel = new FluentLabel(DEMO_TEXT("预览区：--×--px", "Preview: -- x -- px"));) \
    X(sizeLabel->setStyleSheet("font-size: 12px; opacity: 0.9;");) \
    X(auto *tip = new FluentLabel(DEMO_TEXT("拖拽分隔条调整宽度/高度，观察 FlowLayout 自动换行与可视行数变化。", "Drag the splitter to adjust width and height, then observe FlowLayout's automatic wrapping and visible row count changes."));) \
    X(tip->setWordWrap(true);) \
    X(tip->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);) \
    X(tip->setStyleSheet("font-size: 12px; opacity: 0.85;");) \
    X(ctrlL->addWidget(sizeLabel);) \
    X(ctrlL->addWidget(tip);) \
    X(auto *uniformSwitch = new FluentToggleSwitch(DEMO_TEXT("统一项宽度", "Uniform item width"));) \
    X(uniformSwitch->setChecked(true);) \
    X(ctrlL->addWidget(uniformSwitch);) \
    X(ctrlL->addStretch(1);) \
    X(auto *vs = new FluentSplitter(Qt::Vertical);) \
    X(vs->setChildrenCollapsible(false);) \
    X(auto *preview = new FluentCard();) \
    X(preview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);) \
    X(preview->setMinimumWidth(120);) \
    X(auto *pl = new QVBoxLayout(preview);) \
    X(pl->setContentsMargins(12, 12, 12, 12);) \
    X(pl->setSpacing(10);) \
    X(auto *previewTitle = new FluentLabel(DEMO_TEXT("FlowLayout 预览区", "FlowLayout preview"));) \
    X(previewTitle->setStyleSheet("font-size: 12px; font-weight: 600; opacity: 0.95;");) \
    X(pl->addWidget(previewTitle);) \
    X(auto *area = new FluentScrollArea();) \
    X(area->setOverlayScrollBarsEnabled(true);) \
    X(area->setWidgetResizable(true);) \
    X(area->setFrameShape(QFrame::NoFrame);) \
    X(auto *tilesHost = new FluentWidget();) \
    X(tilesHost->setBackgroundRole(FluentWidget::BackgroundRole::Transparent);) \
    X(area->setWidget(tilesHost);) \
    X(pl->addWidget(area);) \
    X(auto *flow = new FluentFlowLayout(tilesHost, 0, 8, 8);) \
        X(flow->setUniformItemWidthEnabled(true);) \
        X(flow->setMinimumItemWidth(96);) \
        X(flow->setAnimationEnabled(true);) \
        X(flow->setAnimationDuration(160);) \
        X(flow->setAnimationEasing(QEasingCurve(QEasingCurve::OutCubic));) \
        X(flow->setAnimationThrottle(60);) \
        X(QObject::connect(uniformSwitch, &FluentToggleSwitch::toggled, tilesHost, [=](bool on) { flow->setUniformItemWidthEnabled(on); flow->invalidate(); flow->activate(); tilesHost->updateGeometry(); tilesHost->update(); });) \
    X(for (int i = 1; i <= 14; ++i) { \
          auto *b = new FluentButton(QStringLiteral("Tile %1").arg(i)); \
          b->setFixedHeight(32); \
            b->setMinimumWidth(80); \
            b->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed); \
          flow->addWidget(b); \
      }) \
    X(auto *bottomLab = new FluentLabel(DEMO_TEXT("拖拽上方分隔条调整预览区高度", "Drag the splitter above to adjust the preview height"));) \
    X(bottomLab->setWordWrap(true);) \
    X(bottomLab->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);) \
    X(auto *bottom = Demo::makeSidebarCard(bottomLab);) \
    X(vs->addWidget(preview);) \
    X(vs->addWidget(bottom);) \
    X(vs->setSizes({240, 120});) \
    X(hs->addWidget(ctrl);) \
    X(hs->addWidget(vs);) \
    X(hs->setSizes({240, 760});) \
    X(body->addWidget(hs);) \
    X(preview->installEventFilter(new DemoSizeLabelWatcher(preview, sizeLabel, preview));)

#define X(line) code += QStringLiteral(#line "\n");
            CONTAINERS_FLOWLAYOUT(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentFlowLayout"),
                DEMO_TEXT("自适应换行布局：拖拽 Splitter 调整宽高", "Adaptive wrapping layout: drag the splitter to change width and height"),
                DEMO_TEXT("要点：\n"
                          "-用于 Tag/Tile/按钮组 等自适应换行场景\n"
                          "-只需 flow->addWidget(...)，无需手动算行列\n"
                          "-setUniformItemWidthEnabled(true) 开启自动对齐宽度（demo 内可随时开关对比）\n"
                          "-宽度变化会触发自动换行；高度变化会改变可视行数（可滚动）",
                          "Highlights:\n"
                          "-Ideal for adaptive wrapping scenarios such as tags, tiles, or button groups\n"
                          "-Just call flow->addWidget(...) without calculating rows or columns manually\n"
                          "-setUniformItemWidthEnabled(true) aligns item widths automatically and can be toggled in the demo for comparison\n"
                          "-Width changes trigger automatic wrapping, while height changes affect the visible row count and scrolling"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    CONTAINERS_FLOWLAYOUT(X)
#undef X
                },
                true,
                290));

#undef CONTAINERS_FLOWLAYOUT
        }

        Q_UNUSED(window);
    });
}

} // namespace Demo::Pages
