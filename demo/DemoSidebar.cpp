#include "DemoSidebar.h"

#include "DemoHelpers.h"
#include "sidebar/DemoThemePanel.h"
#include "sidebar/DemoCodeEditorPanel.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QPointer>
#include <QResizeEvent>
#include <QScrollBar>
#include <QStringListModel>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

#include "Fluent/FluentAnnotatedScrollBar.h"
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
    if (showNavigation) {
        setFixedWidth(280);
    } else {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMinimumWidth(0);
    }

    auto *sidebarContent = new QWidget();
    setWidget(sidebarContent);
    sidebarContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    sidebarContent->setMinimumWidth(0);

    auto *sideLayout = new QVBoxLayout(sidebarContent);
    sideLayout->setContentsMargins(showNavigation ? 0 : 24,
                                   showNavigation ? 0 : 24,
                                   showNavigation ? 0 : 204,
                                   showNavigation ? 0 : 24);
    sideLayout->setSpacing(showNavigation ? 12 : 18);

    QWidget *headerCard = nullptr;
    if (showNavigation) {
        auto *header = new QWidget();
        auto *hl = new QVBoxLayout(header);
        hl->setContentsMargins(0, 0, 0, 0);
        hl->setSpacing(4);

        auto *title = new FluentLabel(QStringLiteral("QtFluentWidgets"));
        title->setStyleSheet("font-size: 18px; font-weight: 650;");
        auto *sub = new FluentLabel(DEMO_TEXT("全控件展示 + Theme/Style 联动", "Full control gallery + Theme/Style linkage"));
        sub->setStyleSheet("font-size: 12px; opacity: 0.85;");
        sub->setWordWrap(true);
        sub->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        sub->setMinimumWidth(0);
        hl->addWidget(title);
        hl->addWidget(sub);

        headerCard = Demo::makeSidebarCard(header);
    }

    auto *themePanel = new DemoThemePanel(m_hostWindow, sidebarContent, false, !showNavigation);
    m_toastPosition = themePanel->toastPosition();
    QObject::connect(themePanel,
                     &DemoThemePanel::toastPositionChanged,
                     this,
                     [this](Fluent::FluentToast::Position pos) {
                         m_toastPosition = pos;
                         emit toastPositionChanged(pos);
                     });
    QObject::connect(themePanel, &DemoThemePanel::languageChanged, this, &DemoSidebar::languageChanged);

    QWidget *themeSection = nullptr;
    QWidget *codeEditorSection = nullptr;

    if (showNavigation) {
        auto *themeCard = new FluentCard();
        themeCard->setCollapsible(true);
        themeCard->setTitle(DEMO_TEXT("主题 / 样式", "Theme / Style"));
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

        auto *codeEditorCard = new FluentCard();
        codeEditorCard->setCollapsible(true);
        codeEditorCard->setCollapsed(true);
        codeEditorCard->setTitle(QStringLiteral("CodeEditor"));
        codeEditorCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        codeEditorCard->contentLayout()->addWidget(new DemoCodeEditorPanel(codeEditorCard, false));

        themeSection = themeCard;
        codeEditorSection = codeEditorCard;
    } else {
        themeSection = themePanel;
        auto *codeEditorCard = new FluentCard();
        codeEditorCard->setProperty("_demoSectionTitle", QStringLiteral("CodeEditor"));
        auto *codeLayout = new QVBoxLayout(codeEditorCard);
        codeLayout->setContentsMargins(16, 16, 16, 16);
        codeLayout->setSpacing(10);
        auto *codeTitle = new FluentLabel(QStringLiteral("CodeEditor"));
        codeTitle->setStyleSheet("font-size: 14px; font-weight: 700;");
        codeLayout->addWidget(codeTitle);
        codeLayout->addWidget(new DemoCodeEditorPanel(codeEditorCard, true));
        codeEditorSection = codeEditorCard;
    }

    QWidget *navCard = nullptr;
    if (showNavigation) {
        auto *navCardInner = new QWidget();
        auto *navLayout = new QVBoxLayout(navCardInner);
        navLayout->setContentsMargins(0, 0, 0, 0);
        navLayout->setSpacing(10);
        navLayout->addWidget(new FluentLabel(DEMO_TEXT("页面", "Pages")));

        const QStringList navItems = {
            DEMO_TEXT("总览", "Overview"),
            DEMO_TEXT("输入", "Inputs"),
            DEMO_TEXT("按钮/开关", "Buttons / Toggles"),
            DEMO_TEXT("动态", "Motion"),
            DEMO_TEXT("选择器", "Pickers"),
            DEMO_TEXT("角度控件", "Angle Controls"),
            DEMO_TEXT("数据视图", "Data Views"),
            DEMO_TEXT("容器/布局", "Containers / Layout"),
            DEMO_TEXT("窗口/对话框", "Windows / Dialogs"),
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
    if (themeSection) {
        themeSection->setSizePolicy(QSizePolicy::Expanding, showNavigation ? QSizePolicy::Expanding : QSizePolicy::Preferred);
        sideLayout->addWidget(themeSection, showNavigation ? 1 : 0);
    }
    if (codeEditorSection) {
        sideLayout->addWidget(codeEditorSection);
    }
    if (navCard) {
        navCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sideLayout->addWidget(navCard);
    }
    if (!showNavigation) {
        sideLayout->addStretch(1);

        m_annotatedScrollBar = new FluentAnnotatedScrollBar(this);
        m_annotatedScrollBar->setObjectName(QStringLiteral("DemoSettingsAnnotatedScrollBar"));
        m_annotatedScrollBar->setFixedWidth(184);
        m_annotatedScrollBar->setToolTipDuration(1100);
        m_annotatedScrollBar->setScrollArea(this);
        m_annotatedScrollBar->setVisible(true);
        m_annotatedScrollBar->raise();

        QObject::connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, [this](int, int) {
            for (int delay : {0, 16, 48}) {
                QTimer::singleShot(delay, this, &DemoSidebar::refreshAnnotatedSources);
            }
        });
        for (int delay : {0, 16, 48}) {
            QTimer::singleShot(delay, this, &DemoSidebar::refreshAnnotatedSources);
        }
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

void DemoSidebar::resizeEvent(QResizeEvent *event)
{
    FluentScrollArea::resizeEvent(event);

    if (m_annotatedScrollBar) {
        const int w = m_annotatedScrollBar->width();
        m_annotatedScrollBar->setGeometry(width() - w - 10, 12, w, qMax(0, height() - 24));
        m_annotatedScrollBar->raise();
        for (int delay : {0, 16, 48}) {
            QTimer::singleShot(delay, this, &DemoSidebar::refreshAnnotatedSources);
        }
    }
}

void DemoSidebar::refreshAnnotatedSources()
{
    if (!m_annotatedScrollBar || !widget()) {
        return;
    }

    QVector<FluentAnnotatedScrollBarSource> sources;
    int fallbackTop = 0;
    const auto cards = widget()->findChildren<FluentCard *>();
    for (FluentCard *card : cards) {
        if (!card || card->isHidden()) {
            continue;
        }

        const QString title = annotatedRailTitleForWidget(card);
        if (title.isEmpty()) {
            continue;
        }

        const QPoint topLeft = card->mapTo(widget(), QPoint(0, 0));
        const QRect geometry = card->geometry();
        const int hintHeight = qMax(1, card->sizeHint().height());
        const bool hasGeometry = card->isVisible() && geometry.width() > 0 && geometry.height() > 0;
        const int sourceStart = hasGeometry ? qMax(0, topLeft.y()) : qMax(0, fallbackTop);
        const int sectionHeight = hasGeometry ? qMax(1, card->height()) : hintHeight;

        FluentAnnotatedScrollBarSource source;
        source.group = title;
        source.text = title;
        source.start = sourceStart;
        source.end = source.start + sectionHeight - 1;
        sources.append(source);
        fallbackTop = sourceStart + sectionHeight + 18;
    }

    std::sort(sources.begin(), sources.end(), [](const FluentAnnotatedScrollBarSource &a,
                                                 const FluentAnnotatedScrollBarSource &b) {
        return a.start < b.start;
    });

    if (sources.isEmpty()) {
        m_annotatedScrollBar->setVisible(true);
        return;
    }

    m_annotatedScrollBar->setSources(sources);
    m_annotatedScrollBar->setVisible(sources.size() > 1);
}

FluentToast::Position DemoSidebar::toastPosition() const
{
    return m_toastPosition;
}

} // namespace Demo
