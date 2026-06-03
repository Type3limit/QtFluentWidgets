#include "DemoWindow.h"

#include "DemoHelpers.h"
#include "DemoSidebar.h"

#include "pages/PageButtons.h"
#include "pages/PageBasicInput.h"
#include "pages/PageContainers.h"
#include "pages/PageDataViews.h"
#include "pages/PageAngleControls.h"
#include "pages/PageIcons.h"
#include "pages/PageInputs.h"
#include "pages/PageMotion.h"
#include "pages/PageOverview.h"
#include "pages/PagePickers.h"
#include "pages/PageWindows.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QCursor>
#include <QDebug>
#include <QElapsedTimer>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QParallelAnimationGroup>
#include <QPointer>
#include <QPropertyAnimation>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QVector>
#include <array>
#include <functional>
#include <memory>
#include <utility>

#include "Fluent/FluentAnimatedButton.h"
#include "Fluent/FluentButton.h"
#include "Fluent/FluentCalendarPicker.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentCheckBox.h"
#include "Fluent/FluentColorDialog.h"
#include "Fluent/FluentColorPicker.h"
#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentDialog.h"
#include "Fluent/FluentFlowLayout.h"
#include "Fluent/FluentGroupBox.h"
#include "Fluent/FluentIcon.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentLineEdit.h"
#include "Fluent/FluentMenu.h"
#include "Fluent/FluentMenuBar.h"
#include "Fluent/FluentMessageBox.h"
#include "Fluent/FluentMotion.h"
#include "Fluent/FluentNavigationView.h"
#include "Fluent/FluentProgressBar.h"
#include "Fluent/FluentRadioButton.h"
#include "Fluent/FluentScrollArea.h"
#include "Fluent/FluentScrollBar.h"
#include "Fluent/FluentSlider.h"
#include "Fluent/FluentSpinBox.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTabWidget.h"
#include "Fluent/FluentTableView.h"
#include "Fluent/FluentTextEdit.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentTimePicker.h"
#include "Fluent/FluentToggleSwitch.h"
#include "Fluent/FluentToolButton.h"
#include "Fluent/FluentTreeView.h"
#include "Fluent/FluentToast.h"
#include "Fluent/FluentWidget.h"

namespace Demo {

using namespace Fluent;

namespace {

class DemoStartupTrace
{
public:
    explicit DemoStartupTrace(QString context)
        : m_context(std::move(context))
    {
        m_timer.start();
        qInfo().noquote() << QStringLiteral("[DemoStartup] %1 begin").arg(m_context);
    }

    ~DemoStartupTrace()
    {
        qInfo().noquote() << QStringLiteral("[DemoStartup] %1 done at %2 ms").arg(m_context).arg(m_timer.elapsed());
    }

    void mark(const QString &label)
    {
        const qint64 now = m_timer.elapsed();
        qInfo().noquote() << QStringLiteral("[DemoStartup] %1 %2 +%3 ms, total %4 ms")
                                  .arg(m_context, label)
                                  .arg(now - m_lastMark)
                                  .arg(now);
        m_lastMark = now;
    }

    template <typename Factory>
    QWidget *addPage(QStackedWidget *stack, const QString &name, Factory factory)
    {
        const qint64 beforeCreate = m_timer.elapsed();
        QWidget *page = factory();
        const qint64 afterCreate = m_timer.elapsed();
        qInfo().noquote() << QStringLiteral("[DemoStartup] %1 page %2 created +%3 ms, total %4 ms")
                                  .arg(m_context, name)
                                  .arg(afterCreate - beforeCreate)
                                  .arg(afterCreate);
        if (stack && page) {
            const qint64 beforeAdd = m_timer.elapsed();
            stack->addWidget(page);
            const qint64 afterAdd = m_timer.elapsed();
            qInfo().noquote() << QStringLiteral("[DemoStartup] %1 page %2 added +%3 ms, total %4 ms")
                                      .arg(m_context, name)
                                      .arg(afterAdd - beforeAdd)
                                      .arg(afterAdd);
        }
        return page;
    }

private:
    QString m_context;
    QElapsedTimer m_timer;
    qint64 m_lastMark = 0;
};

void animateStackPageTransition(QStackedWidget *stack, QWidget *page, int previousIndex, int nextIndex)
{
    if (!stack || !page || previousIndex == nextIndex || !stack->isVisible()) {
        return;
    }

    const int duration = FluentMotion::duration(FluentMotionRole::Page);
    if (duration <= 0) {
        page->move(QPoint(0, 0));
        return;
    }

    if (QGraphicsEffect *oldEffect = page->graphicsEffect()) {
        if (!oldEffect->property("_demoPageTransitionEffect").toBool()) {
            return;
        }
        // A transition is already animating this page. The running animation owns
        // its cleanup; clearing the effect here would delete its animation target.
        return;
    }

    const QPoint finalPos = page->pos();
    const int slide = qMax(12, FluentMotion::popupSlideOffset() * 2);
    const int direction = nextIndex > previousIndex ? 1 : -1;
    const QPoint startPos = finalPos + QPoint(0, direction * slide);

    auto *effect = new QGraphicsOpacityEffect(page);
    effect->setProperty("_demoPageTransitionEffect", true);
    effect->setOpacity(0.0);
    page->setGraphicsEffect(effect);
    page->move(startPos);

    auto *group = new QParallelAnimationGroup(page);
    auto *opacity = new QPropertyAnimation(effect, "opacity", group);
    opacity->setDuration(duration);
    opacity->setEasingCurve(FluentMotion::easing(FluentMotionRole::Page));
    opacity->setStartValue(0.0);
    opacity->setEndValue(1.0);

    auto *position = new QPropertyAnimation(page, "pos", group);
    position->setDuration(duration);
    position->setEasingCurve(FluentMotion::easing(FluentMotionRole::Page));
    position->setStartValue(startPos);
    position->setEndValue(finalPos);

    group->addAnimation(opacity);
    group->addAnimation(position);
    QObject::connect(group, &QParallelAnimationGroup::finished, page, [page, effect, finalPos, group]() {
        page->move(finalPos);
        if (page->graphicsEffect() == effect) {
            // QWidget owns graphicsEffect(); setGraphicsEffect(nullptr) deletes it.
            page->setGraphicsEffect(nullptr);
        }
        group->deleteLater();
    });
    group->start();
}

void fitCommandButtonText(FluentButton *button, int minimumWidth = 72)
{
    if (!button) {
        return;
    }

    const int horizontalPadding = 28;
    const int textWidth = button->fontMetrics().horizontalAdvance(button->text());
    button->setFixedSize(qMax(minimumWidth, textWidth + horizontalPadding), 28);
}

} // namespace

DemoWindow::DemoWindow(QWidget *parent,
                       const QString &selectedNavigationKey,
                       FluentToast::Position toastPosition)
    : FluentMainWindow(parent)
    , m_toastPosition(toastPosition)
    , m_selectedNavigationKey(selectedNavigationKey)
{
    qInfo().noquote() << QStringLiteral("[DemoStartup] DemoWindow ctor body begin");
    buildUi();
    qInfo().noquote() << QStringLiteral("[DemoStartup] DemoWindow ctor body done");
}

void DemoWindow::clearUi()
{
    QPointer<QWidget> oldRoot = m_rootWidget;
    QPointer<QWidget> oldLeftWidget = m_titleBarLeftOwnedWidget;
    QPointer<QWidget> oldRightWidget = m_titleBarRightOwnedWidget;
    QPointer<FluentMenuBar> oldMenuBar = m_ownedMenuBar;

    if (m_titleBarLeftOwnedWidget && fluentTitleBarLeftWidget() == m_titleBarLeftOwnedWidget) {
        setFluentTitleBarLeftWidget(nullptr);
    }
    if (m_titleBarRightOwnedWidget && fluentTitleBarRightWidget() == m_titleBarRightOwnedWidget) {
        setFluentTitleBarRightWidget(nullptr);
    }

    m_titleBarLeftOwnedWidget = nullptr;
    m_titleBarRightOwnedWidget = nullptr;

    if (oldMenuBar) {
        oldMenuBar->hide();
        oldMenuBar->setParent(nullptr);
        setFluentMenuBar(nullptr);
        oldMenuBar->deleteLater();
    }
    m_ownedMenuBar = nullptr;

    if (oldRoot) {
        oldRoot->hide();
        setCentralWidget(nullptr);
        oldRoot->deleteLater();
    }
    m_rootWidget = nullptr;

    if (oldLeftWidget) {
        oldLeftWidget->hide();
        oldLeftWidget->deleteLater();
    }
    if (oldRightWidget) {
        oldRightWidget->hide();
        oldRightWidget->deleteLater();
    }
}

void DemoWindow::buildUi()
{
    DemoStartupTrace trace(QStringLiteral("DemoWindow::buildUi"));
    auto &window = *this;

    window.setWindowTitle(QStringLiteral("QtFluentWidgets Showcase"));
    trace.mark(QStringLiteral("window title"));

    // Menu / Toolbar / StatusBar: demonstrate window-level Fluent components.
    auto *menuBar = new FluentMenuBar();
    m_ownedMenuBar = menuBar;
    trace.mark(QStringLiteral("menu bar object"));

    auto *fileMenu = menuBar->addFluentMenu(DEMO_TEXT("文件", "File"));
    QAction *exitAction = fileMenu->addAction(DEMO_TEXT("退出", "Exit"));
    exitAction->setIcon(FluentIcon::icon(FluentIconType::Close));
    QObject::connect(exitAction, &QAction::triggered, this, &QWidget::close);
    trace.mark(QStringLiteral("file menu"));

    auto *viewMenu = menuBar->addFluentMenu(DEMO_TEXT("视图", "View"));
    QAction *lightAction = viewMenu->addAction(DEMO_TEXT("浅色", "Light"));
    QAction *darkAction = viewMenu->addAction(DEMO_TEXT("深色", "Dark"));
    lightAction->setIcon(FluentIcon::icon(FluentIconType::Eye));
    darkAction->setIcon(FluentIcon::icon(FluentIconType::Settings));
    auto *themeGroup = new QActionGroup(viewMenu);
    themeGroup->setExclusive(true);
    themeGroup->addAction(lightAction);
    themeGroup->addAction(darkAction);
    lightAction->setCheckable(true);
    darkAction->setCheckable(true);
    const bool isDarkAtStartup = ThemeManager::instance().themeMode() == ThemeManager::ThemeMode::Dark;
    darkAction->setChecked(isDarkAtStartup);
    lightAction->setChecked(!isDarkAtStartup);
    QObject::connect(lightAction, &QAction::triggered, []() { ThemeManager::instance().setLightMode(); });
    QObject::connect(darkAction, &QAction::triggered, []() { ThemeManager::instance().setDarkMode(); });
    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, menuBar, [lightAction, darkAction]() {
        const bool isDark = ThemeManager::instance().themeMode() == ThemeManager::ThemeMode::Dark;
        darkAction->setChecked(isDark);
        lightAction->setChecked(!isDark);
    });
    trace.mark(QStringLiteral("view menu"));

    window.setFluentMenuBar(menuBar);
    trace.mark(QStringLiteral("menu bar"));

    auto *demoMenu = menuBar->addFluentMenu(DEMO_TEXT("演示", "Demo"));
    QAction *msgInfo = demoMenu->addAction(DEMO_TEXT("消息框", "Message box"));
    QAction *dlgAction = demoMenu->addAction(DEMO_TEXT("对话框", "Dialog"));
    QAction *toastAction = demoMenu->addAction(DEMO_TEXT("Toast 通知", "Toast notification"));
    msgInfo->setIcon(FluentIcon::icon(FluentIconType::Info));
    dlgAction->setIcon(FluentIcon::icon(FluentIconType::Dialog));
    toastAction->setIcon(FluentIcon::icon(FluentIconType::Bell));

    // TitleBar custom slots demo
    {
        auto *searchHost = new QWidget();
        m_titleBarLeftOwnedWidget = searchHost;
        auto *searchLayout = new QHBoxLayout(searchHost);
        searchLayout->setContentsMargins(0, 0, 0, 0);
        searchLayout->setSpacing(6);

        auto *search = new FluentLineEdit(searchHost);
        search->setPlaceholderText(DEMO_TEXT("搜索…", "Search..."));
        search->setFixedWidth(150);

        auto *searchButton = new FluentToolButton(searchHost);
        searchButton->setIcon(FluentIcon::icon(FluentIconType::Search));
        searchButton->setIconSize(QSize(18, 18));
        searchButton->setFixedSize(34, 28);
        searchButton->setToolTip(DEMO_TEXT("填入示例搜索词", "Fill a sample search term"));
        QObject::connect(searchButton, &QToolButton::clicked, search, [search]() {
            search->setText(QStringLiteral("AnimatedIcon"));
            search->setFocus();
            search->selectAll();
        });

        searchLayout->addWidget(search);
        searchLayout->addWidget(searchButton);
        searchHost->setFixedWidth(190);
        window.setFluentTitleBarLeftWidget(searchHost);

        auto *toastControls = new QWidget();
        m_titleBarRightOwnedWidget = toastControls;
        auto *tl = new QHBoxLayout(toastControls);
        tl->setContentsMargins(0, 0, 0, 0);
        tl->setSpacing(8);

        auto *languageHint = new FluentLabel(DEMO_TEXT("语言：", "Language:"));
        languageHint->setStyleSheet("font-size: 12px; opacity: 0.85;");
        languageHint->setToolTip(DEMO_TEXT("切换 demo 显示语言", "Switch the demo display language"));

        auto *languageCombo = new FluentComboBox();
        languageCombo->setFixedHeight(28);
        languageCombo->setFixedWidth(148);
        languageCombo->setToolTip(DEMO_TEXT("选择显示语言", "Choose the display language"));
        languageCombo->addItem(DEMO_TEXT("简体中文", "Chinese (Simplified)"), static_cast<int>(DemoLanguage::Chinese));
        languageCombo->addItem(QStringLiteral("English"), static_cast<int>(DemoLanguage::English));
        languageCombo->setCurrentIndex(currentLanguage() == DemoLanguage::English ? 1 : 0);

        auto *toastHint = new FluentLabel(DEMO_TEXT("Toast：", "Toast:"));
        toastHint->setStyleSheet("font-size: 12px; opacity: 0.85;");
        toastHint->setToolTip(DEMO_TEXT("快速测试 Toast：选择弹出位置，然后发送。", "Quick Toast test: choose a position and send."));

        auto *toastPosCombo = new FluentComboBox();
        toastPosCombo->setFixedHeight(28);
        toastPosCombo->setFixedWidth(120);
        toastPosCombo->setToolTip(DEMO_TEXT("选择 Toast 弹出位置", "Choose the Toast position"));
        toastPosCombo->addItem(DEMO_TEXT("左上", "Top left"), static_cast<int>(FluentToast::Position::TopLeft));
        toastPosCombo->addItem(DEMO_TEXT("顶中", "Top center"), static_cast<int>(FluentToast::Position::TopCenter));
        toastPosCombo->addItem(DEMO_TEXT("右上", "Top right"), static_cast<int>(FluentToast::Position::TopRight));
        toastPosCombo->addItem(DEMO_TEXT("左下", "Bottom left"), static_cast<int>(FluentToast::Position::BottomLeft));
        toastPosCombo->addItem(DEMO_TEXT("底中", "Bottom center"), static_cast<int>(FluentToast::Position::BottomCenter));
        toastPosCombo->addItem(DEMO_TEXT("右下", "Bottom right"), static_cast<int>(FluentToast::Position::BottomRight));

        auto *toastOne = new FluentButton(DEMO_TEXT("发一条", "Send one"));
        auto *toastAll = new FluentButton(DEMO_TEXT("全位置", "All positions"));
        fitCommandButtonText(toastOne);
        fitCommandButtonText(toastAll);
        toastOne->setToolTip(DEMO_TEXT("按当前选择的位置发送一条 Toast", "Send one Toast at the selected position"));
        toastAll->setToolTip(DEMO_TEXT("在所有位置依次弹出 Toast（用于对比布局）", "Show Toasts in every position for layout comparison"));

        tl->addWidget(languageHint);
        tl->addWidget(languageCombo);
        tl->addWidget(toastHint);
        tl->addWidget(toastPosCombo);
        tl->addWidget(toastOne);
        tl->addWidget(toastAll);

        window.setFluentTitleBarRightWidget(toastControls);

        const auto toastIndexFromPosition = [](FluentToast::Position position) {
            switch (position) {
            case FluentToast::Position::TopLeft:
                return 0;
            case FluentToast::Position::TopCenter:
                return 1;
            case FluentToast::Position::TopRight:
                return 2;
            case FluentToast::Position::BottomLeft:
                return 3;
            case FluentToast::Position::BottomCenter:
                return 4;
            case FluentToast::Position::BottomRight:
                return 5;
            }
            return 5;
        };

        toastPosCombo->setCurrentIndex(toastIndexFromPosition(m_toastPosition));

        auto posName = [](FluentToast::Position p) {
            switch (p) {
            case FluentToast::Position::TopLeft:
                return DEMO_TEXT("左上", "Top left");
            case FluentToast::Position::TopCenter:
                return DEMO_TEXT("顶部居中", "Top center");
            case FluentToast::Position::TopRight:
                return DEMO_TEXT("右上", "Top right");
            case FluentToast::Position::BottomLeft:
                return DEMO_TEXT("左下", "Bottom left");
            case FluentToast::Position::BottomCenter:
                return DEMO_TEXT("底部居中", "Bottom center");
            case FluentToast::Position::BottomRight:
                return DEMO_TEXT("右下", "Bottom right");
            }
            return DEMO_TEXT("右下", "Bottom right");
        };

        QObject::connect(toastPosCombo,
                         QOverload<int>::of(&QComboBox::currentIndexChanged),
                         this,
                         [this, toastPosCombo](int) {
                             m_toastPosition = static_cast<FluentToast::Position>(toastPosCombo->currentData().toInt());
                         });

        QObject::connect(languageCombo,
                         QOverload<int>::of(&QComboBox::currentIndexChanged),
                         this,
                         [this, languageCombo](int) {
                             switchLanguage(static_cast<DemoLanguage>(languageCombo->currentData().toInt()));
                         });

        auto showToast = [this, posName]() {
            FluentToast::showToast(this,
                                  QStringLiteral("Toast"),
                                  DEMO_TEXT("当前弹出位置：%1（点击可关闭）", "Current position: %1 (click to dismiss)").arg(posName(m_toastPosition)),
                                  m_toastPosition,
                                  2600);
        };

        QObject::connect(toastOne, &QPushButton::clicked, this, showToast);
        QObject::connect(toastAction, &QAction::triggered, this, showToast);

        QObject::connect(toastAll, &QPushButton::clicked, this, [this, posName]() {
            const std::array<FluentToast::Position, 6> positions = {
                FluentToast::Position::TopLeft,
                FluentToast::Position::TopCenter,
                FluentToast::Position::TopRight,
                FluentToast::Position::BottomLeft,
                FluentToast::Position::BottomCenter,
                FluentToast::Position::BottomRight,
            };

            for (int i = 0; i < static_cast<int>(positions.size()); ++i) {
                const auto p = positions[static_cast<size_t>(i)];
                QTimer::singleShot(i * 120, this, [this, p, posName]() {
                    FluentToast::showToast(this,
                                          QStringLiteral("Toast"),
                                          DEMO_TEXT("位置：%1（点击可关闭）", "Position: %1 (click to dismiss)").arg(posName(p)),
                                          p,
                                          2400);
                });
            }
        });
    }
    trace.mark(QStringLiteral("title bar controls"));

    auto *root = new FluentWidget();
    m_rootWidget = root;
    root->setBackgroundRole(FluentWidget::BackgroundRole::WindowBackground);
    window.setCentralWidget(root);

    auto *rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *contentHost = new QWidget(root);
    contentHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *contentLayout = new QVBoxLayout(contentHost);
    contentLayout->setContentsMargins(0, 8, 16, 16);
    contentLayout->setSpacing(0);
    trace.mark(QStringLiteral("root layout"));

    // ---- NavigationView (left pane) ----
    auto *nav = new FluentNavigationView(root);
    nav->setExpandedWidth(280);
    nav->setCompactWidth(48);
    nav->setAutoCollapseWidth(800);
    nav->setPaneTitle(QStringLiteral("Qt Fluent"));
    nav->loadPaneToggleAnimation(Demo::demoLottieResourcePath(QStringLiteral("menu.json")));
    nav->loadBackButtonAnimation(Demo::demoLottieResourcePath(QStringLiteral("left-arrow.json")));

    auto applyIcon = [](FluentNavigationItem &item, FluentIconType type) {
        item.hasFluentIcon = true;
        item.fluentIcon = type;
    };
    auto applyAnimatedIcon = [](FluentNavigationItem &item, const QString &fileName) {
        item.animatedIconSource = Demo::demoLottieResourcePath(fileName);
    };

    // Build navigation items matching the demo pages
    using NI = FluentNavigationItem;

    std::vector<NI> mainItems;
    {
        NI overview;
        overview.key  = QStringLiteral("overview");
        overview.text = DEMO_TEXT("总览", "Overview");
        applyIcon(overview, FluentIconType::Home);
        mainItems.push_back(overview);

        // "基本输入" category with sub-items
        NI basicInput;
        basicInput.key  = QStringLiteral("basic_input");
        basicInput.text = DEMO_TEXT("基本输入", "Basic Input");
        applyIcon(basicInput, FluentIconType::Input);
        {
            NI inputs;
            inputs.key  = QStringLiteral("inputs");
            inputs.text = DEMO_TEXT("输入", "Inputs");
            applyIcon(inputs, FluentIconType::Edit);
            basicInput.children.push_back(inputs);

            NI buttons;
            buttons.key  = QStringLiteral("buttons");
            buttons.text = DEMO_TEXT("按钮/开关", "Buttons / Toggles");
            applyIcon(buttons, FluentIconType::Controls);
            basicInput.children.push_back(buttons);

            NI angles;
            angles.key  = QStringLiteral("angles");
            angles.text = DEMO_TEXT("角度控件", "Angle Controls");
            applyIcon(angles, FluentIconType::Gauge);
            basicInput.children.push_back(angles);
        }
        mainItems.push_back(basicInput);

        NI icons;
        icons.key  = QStringLiteral("icons");
        icons.text = DEMO_TEXT("图标", "Icons");
        applyIcon(icons, FluentIconType::Icons);
        mainItems.push_back(icons);

        NI motion;
        motion.key  = QStringLiteral("motion");
        motion.text = DEMO_TEXT("动态", "Motion");
        applyIcon(motion, FluentIconType::Play);
        mainItems.push_back(motion);

        // "选择器" category
        NI pickers;
        pickers.key  = QStringLiteral("pickers");
        pickers.text = DEMO_TEXT("选择器", "Pickers");
        applyIcon(pickers, FluentIconType::Calendar);
        mainItems.push_back(pickers);

        NI dataViews;
        dataViews.key  = QStringLiteral("dataviews");
        dataViews.text = DEMO_TEXT("数据视图", "Data Views");
        applyIcon(dataViews, FluentIconType::Data);
        mainItems.push_back(dataViews);

        NI containers;
        containers.key  = QStringLiteral("containers");
        containers.text = DEMO_TEXT("容器/布局", "Containers / Layout");
        applyIcon(containers, FluentIconType::Layout);
        mainItems.push_back(containers);

        NI windows;
        windows.key  = QStringLiteral("windows");
        windows.text = DEMO_TEXT("窗口/对话框", "Windows / Dialogs");
        applyIcon(windows, FluentIconType::Dialog);
        mainItems.push_back(windows);
    }
    nav->setItems(mainItems);
    trace.mark(QStringLiteral("navigation items"));

    // Footer items (e.g. Settings / Theme)
    {
        NI settings;
        settings.key  = QStringLiteral("settings");
        settings.text = DEMO_TEXT("设置", "Settings");
        applyIcon(settings, FluentIconType::Settings);
        applyAnimatedIcon(settings, QStringLiteral("setting.json"));
        nav->addFooterItem(settings);
    }
    trace.mark(QStringLiteral("navigation footer"));

    // ---- Content area (stacked pages) ----
    auto *stack = new QStackedWidget(contentHost);

    const auto jumpTo = [nav](int pageIndex) {
        const QStringList keys = {
            QStringLiteral("overview"),
            QStringLiteral("basic_input"),
            QStringLiteral("inputs"),
            QStringLiteral("buttons"),
            QStringLiteral("icons"),
            QStringLiteral("motion"),
            QStringLiteral("pickers"),
            QStringLiteral("angles"),
            QStringLiteral("dataviews"),
            QStringLiteral("containers"),
            QStringLiteral("windows"),
        };
        if (pageIndex >= 0 && pageIndex < keys.size()) {
            nav->setSelectedKey(keys[pageIndex]);
        }
    };

    auto pageNames = std::make_shared<QStringList>();
    auto pageFactories = std::make_shared<QVector<std::function<QWidget *()>>>();
    auto pageWidgets = std::make_shared<QVector<QPointer<QWidget>>>();
    auto *windowPtr = this;

    const auto addLazyPage = [&](const QString &name, std::function<QWidget *()> factory) {
        auto *placeholder = new QWidget(stack);
        placeholder->setProperty("_demoLazyPlaceholder", true);
        placeholder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        stack->addWidget(placeholder);
        pageNames->append(name);
        pageFactories->append(std::move(factory));
        pageWidgets->append(placeholder);
    };

    addLazyPage(QStringLiteral("Overview"), [windowPtr, jumpTo]() { return Demo::Pages::createOverviewPage(windowPtr, jumpTo); });       // 0
    addLazyPage(QStringLiteral("BasicInput"), [windowPtr, jumpTo]() { return Demo::Pages::createBasicInputPage(windowPtr, jumpTo); });  // 1
    addLazyPage(QStringLiteral("Inputs"), [windowPtr]() { return Demo::Pages::createInputsPage(windowPtr); });                          // 2
    addLazyPage(QStringLiteral("Buttons"), [windowPtr]() { return Demo::Pages::createButtonsPage(windowPtr); });                        // 3
    addLazyPage(QStringLiteral("Icons"), []() { return Demo::Pages::createIconsPage(); });                                               // 4
    addLazyPage(QStringLiteral("Motion"), []() { return Demo::Pages::createMotionPage(); });                                             // 5
    addLazyPage(QStringLiteral("Pickers"), [windowPtr]() { return Demo::Pages::createPickersPage(windowPtr); });                        // 6
    addLazyPage(QStringLiteral("AngleControls"), [windowPtr]() { return Demo::Pages::createAngleControlsPage(windowPtr); });            // 7
    addLazyPage(QStringLiteral("DataViews"), [windowPtr]() { return Demo::Pages::createDataViewsPage(windowPtr); });                    // 8
    addLazyPage(QStringLiteral("Containers"), [windowPtr]() { return Demo::Pages::createContainersPage(windowPtr); });                  // 9
    addLazyPage(QStringLiteral("Windows"), [windowPtr]() { return Demo::Pages::createWindowsPage(windowPtr); });                        // 10
    addLazyPage(QStringLiteral("SettingsSidebar"), [this, windowPtr]() {                                                                 // 11
        auto *settingsPage = new DemoSidebar(windowPtr, nullptr, false);
        QObject::connect(settingsPage, &DemoSidebar::toastPositionChanged, this, [this](FluentToast::Position pos) {
            m_toastPosition = pos;
        });
        QObject::connect(settingsPage, &DemoSidebar::languageChanged, this, &DemoWindow::switchLanguage);
        return settingsPage;
    });
    trace.mark(QStringLiteral("lazy page placeholders"));

    auto ensurePage = [stack, pageNames, pageFactories, pageWidgets](int index) -> QWidget * {
        if (!stack || index < 0 || index >= pageFactories->size() || index >= pageWidgets->size()) {
            return nullptr;
        }

        QWidget *current = pageWidgets->at(index);
        if (current && !current->property("_demoLazyPlaceholder").toBool()) {
            return current;
        }

        const QString name = index < pageNames->size() ? pageNames->at(index) : QString::number(index);
        QElapsedTimer timer;
        timer.start();
        QWidget *page = pageFactories->at(index)();
        const qint64 createdMs = timer.elapsed();
        qInfo().noquote() << QStringLiteral("[DemoStartup] lazy page %1 created +%2 ms")
                                  .arg(name)
                                  .arg(createdMs);
        if (!page) {
            return nullptr;
        }

        const int stackIndex = current ? stack->indexOf(current) : index;
        if (current) {
            stack->removeWidget(current);
            current->deleteLater();
        }
        stack->insertWidget(qBound(0, stackIndex, stack->count()), page);
        pageWidgets->replace(index, page);

        qInfo().noquote() << QStringLiteral("[DemoStartup] lazy page %1 installed +%2 ms")
                                  .arg(name)
                                  .arg(timer.elapsed() - createdMs);
        return page;
    };

    // Map navigation keys to stack indices
    QObject::connect(nav, &FluentNavigationView::selectedKeyChanged, this, [this, stack, ensurePage](const QString &key) {
        m_selectedNavigationKey = key;
        static const QHash<QString, int> keyMap = {
            { QStringLiteral("overview"),   0 },
            { QStringLiteral("basic_input"), 1 },
            { QStringLiteral("inputs"),     2 },
            { QStringLiteral("buttons"),    3 },
            { QStringLiteral("icons"),      4 },
            { QStringLiteral("motion"),     5 },
            { QStringLiteral("pickers"),    6 },
            { QStringLiteral("angles"),     7 },
            { QStringLiteral("dataviews"),  8 },
            { QStringLiteral("containers"), 9 },
            { QStringLiteral("windows"),    10 },
            { QStringLiteral("settings"),   11 },
        };
        auto it = keyMap.find(key);
        if (it != keyMap.end()) {
            const int previousIndex = stack->currentIndex();
            ensurePage(it.value());
            stack->setCurrentIndex(it.value());
            animateStackPageTransition(stack, stack->widget(it.value()), previousIndex, it.value());
        }
    });

    nav->setSelectedKey(m_selectedNavigationKey.isEmpty() ? QStringLiteral("overview") : m_selectedNavigationKey);
    trace.mark(QStringLiteral("signals and selected page"));

    rootLayout->addWidget(nav);
    rootLayout->addSpacing(8);
    contentLayout->addWidget(stack);
    rootLayout->addWidget(contentHost, 1);

    QObject::connect(msgInfo, &QAction::triggered, this, [this]() {
        FluentMessageBox::information(this,
                                     DEMO_TEXT("消息框", "Message box"),
                                     DEMO_TEXT("这是从菜单触发的消息框示例。", "This message box was triggered from the menu."),
                                     DEMO_TEXT("用于展示 MessageBox 与 Theme/Accent 联动。", "It demonstrates MessageBox linkage with Theme and Accent."));
    });

    QObject::connect(dlgAction, &QAction::triggered, this, [nav]() {
        nav->setSelectedKey(QStringLiteral("windows"));
    });
    trace.mark(QStringLiteral("final connections"));
}

void DemoWindow::switchLanguage(DemoLanguage language)
{
    if (m_languageSwitchPending || language == currentLanguage()) {
        return;
    }

    m_languageSwitchPending = true;

    QTimer::singleShot(0,
                       this,
                       [this, language]() { performQueuedLanguageSwitch(language); });
}

void DemoWindow::performQueuedLanguageSwitch(DemoLanguage language)
{
    if (QWidget *popup = QApplication::activePopupWidget()) {
        popup->close();
        QTimer::singleShot(0,
                           this,
                           [this, language]() { performQueuedLanguageSwitch(language); });
        return;
    }

    setLanguage(language);
    setUpdatesEnabled(false);
    clearUi();
    buildUi();
    setUpdatesEnabled(true);
    update();
    m_languageSwitchPending = false;
}

} // namespace Demo
