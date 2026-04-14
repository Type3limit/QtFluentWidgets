#include "PageContainers.h"

#include "../DemoHelpers.h"
#include "Fluent/FluentCard.h"

#include "Fluent/FluentButton.h"
#include "Fluent/FluentAnnotatedScrollBar.h"
#include "Fluent/FluentFlowLayout.h"
#include "Fluent/FluentSplitter.h"

#include <QHBoxLayout>
#include <QPair>
#include <QStringList>
#include <QVBoxLayout>

#include <QFrame>

#include "Fluent/FluentGroupBox.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentLineEdit.h"
#include "Fluent/FluentMainWindow.h"
#include "Fluent/FluentNavigationView.h"
#include "Fluent/FluentProgressBar.h"
#include "Fluent/FluentRadioButton.h"
#include "Fluent/FluentScrollArea.h"
#include "Fluent/FluentTabWidget.h"
#include "Fluent/FluentTextEdit.h"
#include "Fluent/FluentToggleSwitch.h"
#include "Fluent/FluentWidget.h"

#include <QGridLayout>
#include <QEvent>

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
        m_label->setText(QStringLiteral("预览区：%1×%2px").arg(m_watched->width()).arg(m_watched->height()));
    }

    QWidget *m_watched = nullptr;
    FluentLabel *m_label = nullptr;
};

FluentCard *makeAnnotatedSectionCard(const QString &group, const QString &title, const QString &summary)
{
    auto *card = new FluentCard();
    card->setFixedHeight(108);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(6);

    auto *titleLabel = new FluentLabel(title);
    titleLabel->setStyleSheet("font-size: 13px; font-weight: 650;");

    auto *groupLabel = new FluentLabel(QStringLiteral("分组：%1").arg(group));
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

QVector<FluentAnnotatedScrollBarSource> makeAnnotatedSources(const QVector<QPair<QString, QString>> &items,
                                                             int sectionHeight,
                                                             int sectionSpacing,
                                                             int startOffset)
{
    QVector<FluentAnnotatedScrollBarSource> sources;
    sources.reserve(items.size());

    const int normalizedHeight = qMax(1, sectionHeight);
    const int normalizedSpacing = qMax(0, sectionSpacing);
    int cursor = qMax(0, startOffset);

    for (const auto &item : items) {
        FluentAnnotatedScrollBarSource source;
        source.group = item.first;
        source.text = item.second;
        source.start = cursor;
        source.end = cursor + normalizedHeight - 1;
        sources.push_back(source);
        cursor += normalizedHeight + normalizedSpacing;
    }

    return sources;
}

} // namespace

QWidget *createContainersPage(FluentMainWindow *window)
{
    return Demo::makePage([&](QVBoxLayout *page) {
        auto s = Demo::makeSection(QStringLiteral("容器 / 布局"),
                                   QStringLiteral("Card / GroupBox / TabWidget / ScrollArea / ScrollBar / AnnotatedScrollBar / Splitter / FlowLayout"));

        page->addWidget(s.card);

        // FluentCard
        {
            QString code;
#define CONTAINERS_CARD(X) \
    X(auto *card = new FluentCard();) \
    X(card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);) \
    X(auto *l = new QVBoxLayout(card);) \
    X(l->setContentsMargins(16, 16, 16, 16);) \
    X(l->setSpacing(8);) \
    X(l->addWidget(new FluentLabel(QStringLiteral("这是一个 FluentCard 内容容器")));) \
    X(l->addWidget(new FluentLabel(QStringLiteral("你可以把任意控件放进来。")));) \
    X(body->addWidget(card);)

#define X(line) code += QStringLiteral(#line "\n");
            CONTAINERS_CARD(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentCard"),
                QStringLiteral("基础卡片容器（背景/边框/阴影随主题）"),
                QStringLiteral("要点：\n"
                               "-作为内容容器时直接给 Card 设置布局\n"
                               "-内部子控件会随主题变化"),
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
    X(card->setTitle(QStringLiteral("高级选项"));) \
    X(card->setCollapsed(true);) \
    X(card->contentLayout()->addWidget(new FluentLabel(QStringLiteral("-折叠时内容会隐藏")));) \
    X(card->contentLayout()->addWidget(new FluentLabel(QStringLiteral("-适合放可选配置/高级设置")));) \
    X(body->addWidget(card);)

#define X(line) code += QStringLiteral(#line "\n");
            CONTAINERS_COLLAPSIBLE_CARD(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("Collapsible FluentCard"),
                QStringLiteral("点击标题折叠/展开（隐藏内容区）"),
                QStringLiteral("要点：\n"
                               "-setCollapsible(true) 开启\n"
                               "-setTitle() 设置标题\n"
                               "-setCollapsed(true/false) 控制展开状态"),
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
    X(gbLine->setPlaceholderText(QStringLiteral("GroupBox 内部控件同样跟随 Theme"));) \
    X(auto *gbToggle = new FluentToggleSwitch(QStringLiteral("开关"));) \
    X(gbl->addWidget(gbLine);) \
    X(gbl->addWidget(gbToggle);) \
    X(body->addWidget(gb);)

#define X(line) code += QStringLiteral(#line "\n");
            CONTAINERS_GROUPBOX(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentGroupBox"),
                QStringLiteral("带标题边框的分组容器"),
                QStringLiteral("要点：\n"
                               "-适合把相关控件归类\n"
                               "-内部控件同样跟随主题"),
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
    X({ auto *l = new QVBoxLayout(tab1); l->setContentsMargins(0, 0, 0, 0); l->setSpacing(10); l->addWidget(new FluentLabel(QStringLiteral("Tab 1：演示控件复用"))); l->addWidget(new FluentProgressBar()); }) \
    X({ auto *l = new QVBoxLayout(tab2); l->setContentsMargins(0, 0, 0, 0); l->setSpacing(10); l->addWidget(new FluentLabel(QStringLiteral("Tab 2：滚动区域"))); auto *big = new FluentTextEdit(); big->setText(QStringLiteral("这里是一个 TextEdit，用来制造滚动内容。\n\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n")); big->setFixedHeight(160); l->addWidget(big); }) \
    X(tabs->addTab(tab1, QStringLiteral("Tab 1"));) \
    X(tabs->addTab(tab2, QStringLiteral("Tab 2"));) \
    X(body->addWidget(tabs);)

#define X(line) code += QStringLiteral(#line "\n");
            CONTAINERS_TABS(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentTabWidget"),
                QStringLiteral("标签页容器（切换动画/主题联动）"),
                QStringLiteral("要点：\n"
                               "-addTab(widget, title) 添加页面\n"
                               "-可用于设置页/多视图切换"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    CONTAINERS_TABS(X)
#undef X
                },
                true,
                250));

#undef CONTAINERS_TABS
        }

        // NavigationView
        {
            const QString code = QStringLiteral(R"CPP(#include "Fluent/FluentNavigationView.h"

using NI = Fluent::FluentNavigationItem;

auto *nav = new Fluent::FluentNavigationView();
nav->setExpandedWidth(220);
nav->setCompactWidth(48);
nav->setPaneDisplayMode(Fluent::FluentNavigationView::Left);
nav->setPaneTitle(QStringLiteral("Pane Title"));
nav->setBackButtonVisible(true);

auto applyGlyph = [](NI &item, ushort codePoint) {
    item.iconGlyph = QString(QChar(codePoint));
    item.iconFontFamily = QStringLiteral("Segoe Fluent Icons");
};

NI home;
home.key = QStringLiteral("home");
home.text = QStringLiteral("Home");
applyGlyph(home, 0xE80F);
nav->addItem(home);

NI documents;
documents.key = QStringLiteral("documents");
documents.text = QStringLiteral("Document options");
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

QObject::connect(nav, &Fluent::FluentNavigationView::selectedKeyChanged,
                 nav, [](const QString &key) {
    qDebug() << "selected:" << key;
});
)CPP");

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentNavigationView"),
                QStringLiteral("WinUI3 风格导航栏：支持 Left / LeftCompact / Top 与显式 footer API"),
                QStringLiteral("要点：\n"
                               "-setPaneDisplayMode() 在 Left / LeftCompact / Top 三种布局间切换\n"
                               "-父项可通过 selectsOnInvoked 控制“点击即选中”还是“只打开子菜单”\n"
                               "-backRequested / itemInvoked / selectedKeyChanged 可分别接回退、点击与选中逻辑\n"
                               "-Footer 通过 addFooterItem() 显式添加，不再暗示内置设置页"),
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
                    nav->setBackButtonVisible(true);
                    nav->setBackButtonEnabled(true);

                    auto applyGlyph = [](FluentNavigationItem &item, ushort codePoint) {
                        item.iconGlyph = QString(QChar(codePoint));
                        item.iconFontFamily = QStringLiteral("Segoe Fluent Icons");
                    };

                    using NI = FluentNavigationItem;

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

                    auto makeBlock = [](const QString &style, int minHeight) {
                        auto *frame = new QFrame();
                        frame->setMinimumHeight(minHeight);
                        frame->setStyleSheet(style);
                        return frame;
                    };

                    auto *blocks = new QWidget(detailCard);
                    auto *blocksLayout = new QGridLayout(blocks);
                    blocksLayout->setContentsMargins(0, 0, 0, 0);
                    blocksLayout->setSpacing(10);
                    blocksLayout->addWidget(makeBlock(QStringLiteral("background: rgba(77, 110, 128, 0.68); border-radius: 8px;"), 160), 0, 0, 2, 1);
                    blocksLayout->addWidget(makeBlock(QStringLiteral("background: rgba(240, 240, 240, 0.72); border-radius: 8px;"), 74), 0, 1);
                    blocksLayout->addWidget(makeBlock(QStringLiteral("background: rgba(248, 248, 248, 0.82); border-radius: 8px;"), 74), 0, 2);
                    blocksLayout->addWidget(makeBlock(QStringLiteral("background: rgba(246, 246, 246, 0.78); border-radius: 8px;"), 74), 1, 1);
                    blocksLayout->addWidget(makeBlock(QStringLiteral("background: rgba(234, 234, 234, 0.78); border-radius: 8px;"), 74), 1, 2);
                    blocksLayout->setColumnStretch(1, 1);
                    blocksLayout->setColumnStretch(2, 1);

                    auto *detailHint = new FluentLabel(QStringLiteral("切换右侧 Pane 模式观察 Left / LeftCompact / Top 三种布局；把“文档组选中父项”关闭后，点击 Document options 只会打开子菜单。"));
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
                    backVisibleToggle->setChecked(true);
                    auto *backEnabledToggle = new FluentToggleSwitch(QStringLiteral("Back button enabled"));
                    backEnabledToggle->setChecked(true);
                    auto *footerVisibleToggle = new FluentToggleSwitch(QStringLiteral("Footer visible"));
                    footerVisibleToggle->setChecked(true);
                    auto *documentInvokesToggle = new FluentToggleSwitch(QStringLiteral("Document组选中父项"));
                    documentInvokesToggle->setChecked(false);
                    auto *paneTitleLabel = new FluentLabel(QStringLiteral("Pane title"));
                    paneTitleLabel->setStyleSheet("font-size: 12px; opacity: 0.8;");
                    auto *paneTitleEdit = new FluentLineEdit();
                    paneTitleEdit->setText(QStringLiteral("Pane Title"));
                    paneTitleEdit->setPlaceholderText(QStringLiteral("Pane Title"));

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

                    const auto rebuildNavigation = [=]() {
                        std::vector<NI> items;

                        NI home;
                        home.key = QStringLiteral("home");
                        home.text = QStringLiteral("Home");
                        applyGlyph(home, 0xE80F);
                        items.push_back(home);

                        NI account;
                        account.key = QStringLiteral("account");
                        account.text = QStringLiteral("Account");
                        applyGlyph(account, 0xE77B);
                        {
                            NI profile;
                            profile.key = QStringLiteral("profile");
                            profile.text = QStringLiteral("个人资料");
                            applyGlyph(profile, 0xE77B);
                            account.children.push_back(profile);

                            NI security;
                            security.key = QStringLiteral("security");
                            security.text = QStringLiteral("安全中心");
                            applyGlyph(security, 0xE72E);
                            account.children.push_back(security);
                        }
                        items.push_back(account);

                        NI documents;
                        documents.key = QStringLiteral("documents");
                        documents.text = QStringLiteral("Document options");
                        documents.selectsOnInvoked = documentInvokesToggle->isChecked();
                        applyGlyph(documents, 0xE8A5);
                        {
                            NI recent;
                            recent.key = QStringLiteral("recent_files");
                            recent.text = QStringLiteral("最近文件");
                            applyGlyph(recent, 0xE823);
                            documents.children.push_back(recent);

                            NI templates;
                            templates.key = QStringLiteral("templates");
                            templates.text = QStringLiteral("模板库");
                            applyGlyph(templates, 0xE7C3);
                            documents.children.push_back(templates);
                        }
                        items.push_back(documents);

                        nav->setItems(items);
                        nav->clearFooterItems();

                        NI help;
                        help.key = QStringLiteral("help_center");
                        help.text = QStringLiteral("帮助中心");
                        applyGlyph(help, 0xE897);
                        nav->addFooterItem(help);

                        NI feedback;
                        feedback.key = QStringLiteral("feedback");
                        feedback.text = QStringLiteral("反馈");
                        applyGlyph(feedback, 0xE939);
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
                            detailBody->setText(QStringLiteral("首页适合映射欢迎页或仪表板。当前示例右侧这块内容不依赖具体布局，所以同一份页面可以随 NavigationView 在 Left / LeftCompact / Top 三种模式间切换。"));
                        } else if (key == QStringLiteral("account")) {
                            detailTitle->setText(QStringLiteral("Account"));
                            detailBody->setText(QStringLiteral("父项默认仍可点击选中；如果在 Top 或 LeftCompact 模式下点击它，也会同时弹出子菜单，接近 WinUI3 的层级导航体验。"));
                        } else if (key == QStringLiteral("profile")) {
                            detailTitle->setText(QStringLiteral("个人资料"));
                            detailBody->setText(QStringLiteral("选中子项时，Left 模式会自动展开父组；Top 模式则保持父项高亮，并通过 flyout 进入具体页面。"));
                        } else if (key == QStringLiteral("security")) {
                            detailTitle->setText(QStringLiteral("安全中心"));
                            detailBody->setText(QStringLiteral("itemInvoked 和 selectedKeyChanged 被拆开后，可以分别处理“点击动作”和“选中状态”，便于接导航、分析埋点或只展开不选中的父项。"));
                        } else if (key == QStringLiteral("documents")) {
                            detailTitle->setText(QStringLiteral("Document options"));
                            detailBody->setText(documentInvokesToggle->isChecked()
                                                    ? QStringLiteral("当前开启“选中父项”。点击正文会选中 Document options，点击箭头或 Top 模式下的下拉区域则打开子菜单。")
                                                    : QStringLiteral("当前关闭“选中父项”。点击 Document options 只会打开子菜单，不会修改 selectedKey，这和 WinUI3 文档里的 submenu-only 行为一致。"));
                        } else if (key == QStringLiteral("recent_files")) {
                            detailTitle->setText(QStringLiteral("最近文件"));
                            detailBody->setText(QStringLiteral("这类真正落到页面的叶子节点通常保持 selectsOnInvoked = true；如果你只想拿点击回调而不切换当前页，也可以单独把叶子节点的 selectsOnInvoked 设为 false。"));
                        } else if (key == QStringLiteral("templates")) {
                            detailTitle->setText(QStringLiteral("模板库"));
                            detailBody->setText(QStringLiteral("Top 模式下，当前高亮会挂在父级顶部 item 上；Left / LeftCompact 模式则保持与当前 key 一致的视觉反馈。"));
                        } else if (key == QStringLiteral("help_center")) {
                            detailTitle->setText(QStringLiteral("帮助中心"));
                            detailBody->setText(QStringLiteral("这里的 footer 是通过 addFooterItem() 显式加入的普通导航项，不再默认绑定成“设置页”。你可以自由放帮助、账户、反馈或关于入口。"));
                        } else if (key == QStringLiteral("feedback")) {
                            detailTitle->setText(QStringLiteral("反馈"));
                            detailBody->setText(QStringLiteral("Footer 在 Left / LeftCompact 模式下会固定到底部，在 Top 模式下会挪到右侧；也可以通过 setFooterVisible(false) 整体隐藏。"));
                        } else {
                            detailTitle->setText(QStringLiteral("NavigationView"));
                            detailBody->setText(QStringLiteral("使用 addItem / addFooterItem 追加导航项，使用 paneDisplayMode / backRequested / itemInvoked 组织不同布局和交互。"));
                        }

                        detailState->setText(QStringLiteral("当前 key：%1\nPane 模式：%2\n文档组选中父项：%3\nFooter：%4（通过 addFooterItem() 显式追加）")
                                                 .arg(key.isEmpty() ? QStringLiteral("<empty>") : key)
                                                 .arg(modeName(nav->paneDisplayMode()))
                                                 .arg(documentInvokesToggle->isChecked() ? QStringLiteral("开启") : QStringLiteral("关闭"))
                                                 .arg(nav->isFooterVisible() ? QStringLiteral("可见") : QStringLiteral("隐藏")));
                    };

                    rebuildNavigation();
                    nav->setSelectedKey(QStringLiteral("home"));
                    relayoutPreview();

                    QObject::connect(nav, &FluentNavigationView::selectedKeyChanged, detailCard, [=](const QString &) {
                        detailEvent->setText(QStringLiteral("最近事件：selectedKeyChanged -> %1").arg(nav->selectedKey()));
                        updateDetail();
                    });
                    QObject::connect(nav, &FluentNavigationView::itemInvoked, detailCard, [=](const QString &key) {
                        detailEvent->setText(QStringLiteral("最近事件：itemInvoked -> %1").arg(key));
                    });
                    QObject::connect(nav, &FluentNavigationView::backRequested, detailCard, [=]() {
                        detailEvent->setText(QStringLiteral("最近事件：backRequested"));
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
                        nav->setBackButtonEnabled(checked);
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

                    updateDetail();
                    detailEvent->setText(QStringLiteral("最近事件：itemInvoked -> home"));

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
    X(for (int i = 1; i <= 18; ++i) { sl->addWidget(new FluentLabel(QStringLiteral("滚动内容 %1").arg(i))); }) \
    X(body->addWidget(area);)

#define X(line) code += QStringLiteral(#line "\n");
            CONTAINERS_SCROLLAREA(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentScrollArea"),
                QStringLiteral("滚动区域（可选 overlay 滚动条）"),
                QStringLiteral("要点：\n"
                               "-setOverlayScrollBarsEnabled(true) 显示 overlay 滚动条\n"
                               "-setWidgetResizable(true) 常用配置"),
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

auto *area = new Fluent::FluentScrollArea();
area->setWidgetResizable(true);
area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

auto *annotated = new Fluent::FluentAnnotatedScrollBar();
annotated->setScrollArea(area);
annotated->setSources({
    {QStringLiteral("概览"), QStringLiteral("欢迎页"), 12, 119},
    {QStringLiteral("概览"), QStringLiteral("快速操作"), 130, 237},
    {QStringLiteral("项目"), QStringLiteral("项目列表"), 248, 355},
    {QStringLiteral("同步"), QStringLiteral("队列状态"), 366, 473}
});

annotated->addSource({QStringLiteral("同步"), QStringLiteral("同步历史"), 484, 591});
annotated->setCurrentGroup(QStringLiteral("项目"));

QObject::connect(annotated, &Fluent::FluentAnnotatedScrollBar::currentSourceChanged,
                 annotated, [](int, const QString &group, const QString &text) {
    qDebug() << group << text;
});
)CPP");

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentAnnotatedScrollBar"),
                QStringLiteral("带区间标注的滚动条，滚动时在右侧显示当前范围"),
                QStringLiteral("要点：\n"
                               "-setScrollArea(...) 直接绑定现有滚动区域\n"
                               "-setSources(...) / addSource(...) 自动按 group 合并成分组，不必手动整理 ranges\n"
                               "-可通过 setCurrentGroup(...) / setCurrentSourceIndex(...) 从外部定位当前段\n"
                               "-滚动、点击标签、拖拽 thumb 都会触发 currentSourceChanged 回调"),
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
                        {QStringLiteral("概览"), QStringLiteral("欢迎页")},
                        {QStringLiteral("概览"), QStringLiteral("快速操作")},
                        {QStringLiteral("项目"), QStringLiteral("项目列表")},
                        {QStringLiteral("项目"), QStringLiteral("里程碑")},
                        {QStringLiteral("同步"), QStringLiteral("队列状态")},
                        {QStringLiteral("同步"), QStringLiteral("同步历史")}
                    };
                    const QStringList summaries = {
                        QStringLiteral("首页入口、摘要卡片和欢迎提示。"),
                        QStringLiteral("常用按钮、最近访问和推荐操作。"),
                        QStringLiteral("当前项目、筛选器和统计摘要。"),
                        QStringLiteral("任务进度、负责人和交付节奏。"),
                        QStringLiteral("后台同步队列、重试数和实时状态。"),
                        QStringLiteral("历史执行记录、完成时间和结果摘要。")
                    };

                    for (int i = 0; i < items.size() && i < summaries.size(); ++i) {
                        contentLayout->addWidget(makeAnnotatedSectionCard(items.at(i).first, items.at(i).second, summaries.at(i)));
                    }

                    auto *status = new FluentLabel(QStringLiteral("当前 source：概览 / 欢迎页"));
                    status->setStyleSheet("font-size: 12px; opacity: 0.82;");

                    auto *annotated = new FluentAnnotatedScrollBar();
                    annotated->setFixedWidth(116);
                    annotated->setToolTipDuration(1100);
                    annotated->setScrollArea(area);
                    annotated->setSources(makeAnnotatedSources(items, 108, 10, 12));

                    QObject::connect(annotated, &FluentAnnotatedScrollBar::currentSourceChanged, status,
                                     [status](int, const QString &group, const QString &text) {
                        if (group.isEmpty() && text.isEmpty()) {
                            status->setText(QStringLiteral("当前 source：<none>"));
                            return;
                        }
                        status->setText(QStringLiteral("当前 source：%1 / %2").arg(group, text));
                    });
                    annotated->setCurrentGroup(QStringLiteral("项目"));

                    row->addWidget(area, 1);
                    row->addWidget(annotated);
                    body->addWidget(shell);
                    body->addWidget(status);
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
    X(h->addWidget(Demo::makeSidebarCard(new FluentLabel(QStringLiteral("左侧"))));) \
    X(h->addWidget(Demo::makeSidebarCard(new FluentLabel(QStringLiteral("右侧"))));) \
    X(h->setSizes({320, 520});) \
    X(auto *v = new FluentSplitter(Qt::Vertical);) \
    X(v->setFixedHeight(160);) \
    X(v->addWidget(Demo::makeSidebarCard(new FluentLabel(QStringLiteral("上方"))));) \
    X(v->addWidget(Demo::makeSidebarCard(new FluentLabel(QStringLiteral("下方"))));) \
    X(v->setSizes({90, 70});) \
    X(l->addWidget(h);) \
    X(l->addWidget(v);) \
    X(body->addWidget(root);)

#define X(line) code += QStringLiteral(#line "\n");
            CONTAINERS_SPLITTER(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentSplitter"),
                QStringLiteral("可拖拽分隔的布局容器（横向/纵向）"),
                QStringLiteral("要点：\n"
                               "-横向：Qt::Horizontal，拖拽竖向分隔条调整左右宽度\n"
                               "-纵向：Qt::Vertical，拖拽横向分隔条调整上下高度\n"
                               "-适合可调整的面板布局（预览/设置/属性面板等）"),
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
    X(auto *sizeLabel = new FluentLabel(QStringLiteral("预览区：--×--px"));) \
    X(sizeLabel->setStyleSheet("font-size: 12px; opacity: 0.9;");) \
    X(auto *tip = new FluentLabel(QStringLiteral("拖拽分隔条调整宽度/高度，观察 FlowLayout 自动换行与可视行数变化。"));) \
    X(tip->setWordWrap(true);) \
    X(tip->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);) \
    X(tip->setStyleSheet("font-size: 12px; opacity: 0.85;");) \
    X(ctrlL->addWidget(sizeLabel);) \
    X(ctrlL->addWidget(tip);) \
    X(auto *uniformSwitch = new FluentToggleSwitch(QStringLiteral("Uniform item width"));) \
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
    X(auto *previewTitle = new FluentLabel(QStringLiteral("FlowLayout 预览区"));) \
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
    X(auto *bottomLab = new FluentLabel(QStringLiteral("拖拽上方分隔条调整预览区高度"));) \
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
                QStringLiteral("自适应换行布局：拖拽 Splitter 调整宽高"),
                QStringLiteral("要点：\n"
                               "-用于 Tag/Tile/按钮组 等自适应换行场景\n"
                               "-只需 flow->addWidget(...)，无需手动算行列\n"
                               "-setUniformItemWidthEnabled(true) 开启自动对齐宽度（demo 内可随时开关对比）\n"
                               "-宽度变化会触发自动换行；高度变化会改变可视行数（可滚动）"),
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
