#include "DemoWindow.h"

#include "DemoHelpers.h"
#include "DemoSidebar.h"

#include "pages/PageButtons.h"
#include "pages/PageBasicInput.h"
#include "pages/PageContainers.h"
#include "pages/PageDataViews.h"
#include "pages/PageAngleControls.h"
#include "pages/PageInputs.h"
#include "pages/PageOverview.h"
#include "pages/PagePickers.h"
#include "pages/PageWindows.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QCursor>
#include <QHBoxLayout>
#include <QPointer>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <array>

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
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentLineEdit.h"
#include "Fluent/FluentMenu.h"
#include "Fluent/FluentMenuBar.h"
#include "Fluent/FluentMessageBox.h"
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

DemoWindow::DemoWindow(QWidget *parent,
                       const QString &selectedNavigationKey,
                       FluentToast::Position toastPosition)
    : FluentMainWindow(parent)
    , m_toastPosition(toastPosition)
    , m_selectedNavigationKey(selectedNavigationKey)
{
    buildUi();
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
    auto &window = *this;

    window.setWindowTitle(QStringLiteral("QtFluentWidgets Showcase"));

    // Menu / Toolbar / StatusBar: demonstrate window-level Fluent components.
    auto *menuBar = new FluentMenuBar();
    m_ownedMenuBar = menuBar;
    auto *fileMenu = menuBar->addFluentMenu(DEMO_TEXT("文件", "File"));
    QAction *exitAction = fileMenu->addAction(DEMO_TEXT("退出", "Exit"));
    QObject::connect(exitAction, &QAction::triggered, this, &QWidget::close);

    auto *viewMenu = menuBar->addFluentMenu(DEMO_TEXT("视图", "View"));
    QAction *lightAction = viewMenu->addAction(DEMO_TEXT("浅色", "Light"));
    QAction *darkAction = viewMenu->addAction(DEMO_TEXT("深色", "Dark"));
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
    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [lightAction, darkAction]() {
        const bool isDark = ThemeManager::instance().themeMode() == ThemeManager::ThemeMode::Dark;
        darkAction->setChecked(isDark);
        lightAction->setChecked(!isDark);
    });

    window.setFluentMenuBar(menuBar);

    auto *demoMenu = menuBar->addFluentMenu(DEMO_TEXT("演示", "Demo"));
    QAction *msgInfo = demoMenu->addAction(DEMO_TEXT("消息框", "Message box"));
    QAction *dlgAction = demoMenu->addAction(DEMO_TEXT("对话框", "Dialog"));
    QAction *toastAction = demoMenu->addAction(DEMO_TEXT("Toast 通知", "Toast notification"));

    // TitleBar custom slots demo
    {
        auto *search = new FluentLineEdit();
        m_titleBarLeftOwnedWidget = search;
        search->setPlaceholderText(DEMO_TEXT("搜索…", "Search..."));
        search->setFixedWidth(160);
        window.setFluentTitleBarLeftWidget(search);

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
        toastOne->setFixedSize(72, 28);
        toastAll->setFixedSize(72, 28);
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

    // ---- NavigationView (left pane) ----
    auto *nav = new FluentNavigationView(root);
    nav->setExpandedWidth(280);
    nav->setCompactWidth(48);
    nav->setAutoCollapseWidth(800);
    nav->setPaneTitle(QStringLiteral("Qt Fluent"));

    auto applyGlyph = [](FluentNavigationItem &item, ushort codePoint) {
        item.iconGlyph = QString(QChar(codePoint));
        item.iconFontFamily = QStringLiteral("Segoe Fluent Icons");
    };

    // Build navigation items matching the demo pages
    using NI = FluentNavigationItem;

    std::vector<NI> mainItems;
    {
        NI overview;
        overview.key  = QStringLiteral("overview");
        overview.text = DEMO_TEXT("总览", "Overview");
        applyGlyph(overview, 0xE80F);
        mainItems.push_back(overview);

        // "基本输入" category with sub-items
        NI basicInput;
        basicInput.key  = QStringLiteral("basic_input");
        basicInput.text = DEMO_TEXT("基本输入", "Basic Input");
        applyGlyph(basicInput, 0xE961);
        {
            NI inputs;
            inputs.key  = QStringLiteral("inputs");
            inputs.text = DEMO_TEXT("输入", "Inputs");
            applyGlyph(inputs, 0xEF60);
            basicInput.children.push_back(inputs);

            NI buttons;
            buttons.key  = QStringLiteral("buttons");
            buttons.text = DEMO_TEXT("按钮/开关", "Buttons / Toggles");
            applyGlyph(buttons, 0xF19F);
            basicInput.children.push_back(buttons);
        }
        mainItems.push_back(basicInput);

        // "选择器" category
        NI pickers;
        pickers.key  = QStringLiteral("pickers");
        pickers.text = DEMO_TEXT("选择器", "Pickers");
        applyGlyph(pickers, 0xE787);
        mainItems.push_back(pickers);

        NI angles;
        angles.key  = QStringLiteral("angles");
        angles.text = DEMO_TEXT("角度控件", "Angle Controls");
        applyGlyph(angles, 0xF0B4);
        mainItems.push_back(angles);

        NI dataViews;
        dataViews.key  = QStringLiteral("dataviews");
        dataViews.text = DEMO_TEXT("数据视图", "Data Views");
        applyGlyph(dataViews, 0xEA37);
        mainItems.push_back(dataViews);

        NI containers;
        containers.key  = QStringLiteral("containers");
        containers.text = DEMO_TEXT("容器/布局", "Containers / Layout");
        applyGlyph(containers, 0xF168);
        mainItems.push_back(containers);

        NI windows;
        windows.key  = QStringLiteral("windows");
        windows.text = DEMO_TEXT("窗口/对话框", "Windows / Dialogs");
        applyGlyph(windows, 0xE73F);
        mainItems.push_back(windows);
    }
    nav->setItems(mainItems);

    // Footer items (e.g. Settings / Theme)
    {
        NI settings;
        settings.key  = QStringLiteral("settings");
        settings.text = DEMO_TEXT("设置", "Settings");
        applyGlyph(settings, 0xE713);
        nav->addFooterItem(settings);
    }

    // ---- Content area (stacked pages) ----
    auto *stack = new QStackedWidget(contentHost);

    const auto jumpTo = [nav](int pageIndex) {
        const QStringList keys = {
            QStringLiteral("overview"),
            QStringLiteral("basic_input"),
            QStringLiteral("inputs"),
            QStringLiteral("buttons"),
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

    stack->addWidget(Demo::Pages::createOverviewPage(&window, jumpTo));   // 0
    stack->addWidget(Demo::Pages::createBasicInputPage(&window, jumpTo)); // 1
    stack->addWidget(Demo::Pages::createInputsPage(&window));             // 2
    stack->addWidget(Demo::Pages::createButtonsPage(&window));            // 3
    stack->addWidget(Demo::Pages::createPickersPage(&window));            // 4
    stack->addWidget(Demo::Pages::createAngleControlsPage(&window));      // 5
    stack->addWidget(Demo::Pages::createDataViewsPage(&window));          // 6
    stack->addWidget(Demo::Pages::createContainersPage(&window));         // 7
    stack->addWidget(Demo::Pages::createWindowsPage(&window));            // 8

    // Settings page: reuse the DemoSidebar as a standalone settings panel
    auto *settingsPage = new DemoSidebar(&window, nullptr, false);
    stack->addWidget(settingsPage);                                       // 9

    // Map navigation keys to stack indices
    QObject::connect(nav, &FluentNavigationView::selectedKeyChanged, this, [this, stack](const QString &key) {
        m_selectedNavigationKey = key;
        static const QHash<QString, int> keyMap = {
            { QStringLiteral("overview"),   0 },
            { QStringLiteral("basic_input"), 1 },
            { QStringLiteral("inputs"),     2 },
            { QStringLiteral("buttons"),    3 },
            { QStringLiteral("pickers"),    4 },
            { QStringLiteral("angles"),     5 },
            { QStringLiteral("dataviews"),  6 },
            { QStringLiteral("containers"), 7 },
            { QStringLiteral("windows"),    8 },
            { QStringLiteral("settings"),   9 },
        };
        auto it = keyMap.find(key);
        if (it != keyMap.end()) {
            stack->setCurrentIndex(it.value());
        }
    });

    QObject::connect(settingsPage, &DemoSidebar::toastPositionChanged, this, [this](FluentToast::Position pos) {
        m_toastPosition = pos;
    });
    QObject::connect(settingsPage, &DemoSidebar::languageChanged, this, &DemoWindow::switchLanguage);

    nav->setSelectedKey(m_selectedNavigationKey.isEmpty() ? QStringLiteral("overview") : m_selectedNavigationKey);

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
