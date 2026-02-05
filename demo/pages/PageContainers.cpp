#include "PageContainers.h"

#include "../DemoHelpers.h"
#include "Fluent/FluentCard.h"

#include "Fluent/FluentButton.h"
#include "Fluent/FluentFlowLayout.h"
#include "Fluent/FluentSplitter.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

#include <QFrame>

#include "Fluent/FluentGroupBox.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentLineEdit.h"
#include "Fluent/FluentMainWindow.h"
#include "Fluent/FluentProgressBar.h"
#include "Fluent/FluentScrollArea.h"
#include "Fluent/FluentTabWidget.h"
#include "Fluent/FluentTextEdit.h"
#include "Fluent/FluentToggleSwitch.h"
#include "Fluent/FluentWidget.h"

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

} // namespace

QWidget *createContainersPage(FluentMainWindow *window)
{
    return Demo::makePage([&](QVBoxLayout *page) {
        auto s = Demo::makeSection(QStringLiteral("容器 / 布局"),
                                   QStringLiteral("Card / GroupBox / TabWidget / ScrollArea / Splitter / FlowLayout"));

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
