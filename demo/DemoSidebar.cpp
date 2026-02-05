#include "DemoSidebar.h"

#include "DemoHelpers.h"
#include "sidebar/DemoThemePanel.h"
#include "sidebar/DemoCodeEditorPanel.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QPointer>
#include <QStringListModel>
#include <QVBoxLayout>

#include "Fluent/FluentCard.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentListView.h"

namespace Demo {

using namespace Fluent;

DemoSidebar::DemoSidebar(QWidget *hostWindow, QWidget *parent, bool showNavigation)
    : FluentScrollArea(parent)
    , m_hostWindow(hostWindow)
{
    setOverlayScrollBarsEnabled(true);
    setFrameShape(QFrame::NoFrame);
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFixedWidth(280);

    auto *sidebarContent = new QWidget();
    setWidget(sidebarContent);
    sidebarContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    sidebarContent->setMinimumWidth(0);

    auto *sideLayout = new QVBoxLayout(sidebarContent);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(12);

    QWidget *headerCard = nullptr;
    {
        auto *header = new QWidget();
        auto *hl = new QVBoxLayout(header);
        hl->setContentsMargins(0, 0, 0, 0);
        hl->setSpacing(4);

        auto *title = new FluentLabel(QStringLiteral("QtFluentWidgets"));
        title->setStyleSheet("font-size: 18px; font-weight: 650;");
        auto *sub = new FluentLabel(QStringLiteral("全控件展示 + Theme/Style 联动"));
        sub->setStyleSheet("font-size: 12px; opacity: 0.85;");
        sub->setWordWrap(true);
        sub->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        sub->setMinimumWidth(0);
        hl->addWidget(title);
        hl->addWidget(sub);

        headerCard = Demo::makeSidebarCard(header);
    }

    auto *themePanel = new DemoThemePanel(m_hostWindow, nullptr, false);
    m_toastPosition = themePanel->toastPosition();
    QObject::connect(themePanel,
                     &DemoThemePanel::toastPositionChanged,
                     this,
                     [this](Fluent::FluentToast::Position pos) {
                         m_toastPosition = pos;
                         emit toastPositionChanged(pos);
                     });

    // Theme panel: wrap in a collapsible card and keep scrolling inside the card.
    auto *themeCard = new FluentCard();
    themeCard->setCollapsible(true);
    themeCard->setTitle(QStringLiteral("主题 / 样式"));
    themeCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *themeScroll = new FluentScrollArea(themeCard);
    themeScroll->setOverlayScrollBarsEnabled(true);
    themeScroll->setFrameShape(QFrame::NoFrame);
    themeScroll->setWidgetResizable(true);
    themeScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    themeScroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    themeScroll->setMinimumHeight(220);
    themeScroll->setWidget(themePanel);

    themeCard->contentLayout()->addWidget(themeScroll);

    // Code editor panel: allow configuring clang-format path and editor behaviors.
    auto *codeEditorCard = new FluentCard();
    codeEditorCard->setCollapsible(true);
    codeEditorCard->setCollapsed(true);
    codeEditorCard->setTitle(QStringLiteral("CodeEditor"));
    codeEditorCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    codeEditorCard->contentLayout()->addWidget(new DemoCodeEditorPanel(codeEditorCard));

    QWidget *navCard = nullptr;
    if (showNavigation) {
        auto *navCardInner = new QWidget();
        auto *navLayout = new QVBoxLayout(navCardInner);
        navLayout->setContentsMargins(0, 0, 0, 0);
        navLayout->setSpacing(10);
        navLayout->addWidget(new FluentLabel(QStringLiteral("页面")));

        const QStringList navItems = {
            QStringLiteral("总览"),
            QStringLiteral("输入"),
            QStringLiteral("按钮/开关"),
            QStringLiteral("选择器"),
            QStringLiteral("数据视图"),
            QStringLiteral("容器/布局"),
            QStringLiteral("窗口/对话框"),
        };

        m_navView = new FluentListView();
        m_navView->setFixedHeight(300);
        m_navView->setSelectionMode(QAbstractItemView::SingleSelection);

        m_navModel = new QStringListModel(this);
        m_navModel->setStringList(navItems);
        m_navView->setModel(m_navModel);
        navLayout->addWidget(m_navView);

        navCard = Demo::makeSidebarCard(navCardInner);
    }

    // Stack cards vertically so the theme card can stretch to fill the remaining height.
    if (headerCard) {
        headerCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sideLayout->addWidget(headerCard);
    }
    if (themeCard) {
        sideLayout->addWidget(themeCard, 1);
    }
    if (codeEditorCard) {
        sideLayout->addWidget(codeEditorCard);
    }
    if (navCard) {
        navCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sideLayout->addWidget(navCard);
    }

    if (m_navView && m_navView->selectionModel()) {
        QObject::connect(m_navView->selectionModel(),
                         &QItemSelectionModel::currentChanged,
                         this,
                         [this](const QModelIndex &current, const QModelIndex &) {
                             if (current.isValid()) {
                                 emit currentPageChanged(current.row());
                             }
                         });

        setCurrentPage(0);
    }
}

void DemoSidebar::setCurrentPage(int index)
{
    if (!m_navView || !m_navModel) {
        return;
    }
    if (index < 0 || index >= m_navModel->rowCount()) {
        return;
    }
    m_navView->setCurrentIndex(m_navModel->index(index));
}

int DemoSidebar::currentPage() const
{
    if (!m_navView || !m_navModel) {
        return -1;
    }
    const QModelIndex idx = m_navView->currentIndex();
    return idx.isValid() ? idx.row() : -1;
}

FluentToast::Position DemoSidebar::toastPosition() const
{
    return m_toastPosition;
}

} // namespace Demo
