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

DemoWindow::DemoWindow(QWidget *parent)
    : FluentMainWindow(parent)
{
    auto &window = *this;

    window.setWindowTitle(QStringLiteral("QtFluentWidgets Showcase"));

    // Menu / Toolbar / StatusBar: demonstrate window-level Fluent components.
    auto *menuBar = new FluentMenuBar();
    auto *fileMenu = menuBar->addFluentMenu(QStringLiteral("文件"));
    QAction *exitAction = fileMenu->addAction(QStringLiteral("退出"));
    QObject::connect(exitAction, &QAction::triggered, this, &QWidget::close);

    auto *viewMenu = menuBar->addFluentMenu(QStringLiteral("视图"));
    QAction *lightAction = viewMenu->addAction(QStringLiteral("浅色"));
    QAction *darkAction = viewMenu->addAction(QStringLiteral("深色"));
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

    auto *demoMenu = menuBar->addFluentMenu(QStringLiteral("演示"));
    QAction *msgInfo = demoMenu->addAction(QStringLiteral("消息框"));
    QAction *dlgAction = demoMenu->addAction(QStringLiteral("对话框"));
    QAction *toastAction = demoMenu->addAction(QStringLiteral("Toast 通知"));

    // TitleBar custom slots demo
    {
        auto *search = new FluentLineEdit();
        search->setPlaceholderText(QStringLiteral("搜索…"));
        search->setFixedWidth(160);
        window.setFluentTitleBarLeftWidget(search);

        auto *toastControls = new QWidget();
        auto *tl = new QHBoxLayout(toastControls);
        tl->setContentsMargins(0, 0, 0, 0);
        tl->setSpacing(8);

        auto *toastHint = new FluentLabel(QStringLiteral("Toast："));
        toastHint->setStyleSheet("font-size: 12px; opacity: 0.85;");
        toastHint->setToolTip(QStringLiteral("快速测试 Toast：选择弹出位置，然后发送。"));

        auto *toastPosCombo = new FluentComboBox();
        toastPosCombo->setFixedHeight(28);
        toastPosCombo->setFixedWidth(120);
        toastPosCombo->setToolTip(QStringLiteral("选择 Toast 弹出位置"));
        toastPosCombo->addItem(QStringLiteral("左上"), static_cast<int>(FluentToast::Position::TopLeft));
        toastPosCombo->addItem(QStringLiteral("顶中"), static_cast<int>(FluentToast::Position::TopCenter));
        toastPosCombo->addItem(QStringLiteral("右上"), static_cast<int>(FluentToast::Position::TopRight));
        toastPosCombo->addItem(QStringLiteral("左下"), static_cast<int>(FluentToast::Position::BottomLeft));
        toastPosCombo->addItem(QStringLiteral("底中"), static_cast<int>(FluentToast::Position::BottomCenter));
        toastPosCombo->addItem(QStringLiteral("右下"), static_cast<int>(FluentToast::Position::BottomRight));
        toastPosCombo->setCurrentIndex(5);

        auto *toastOne = new FluentButton(QStringLiteral("发一条"));
        auto *toastAll = new FluentButton(QStringLiteral("全位置"));
        toastOne->setFixedSize(72, 28);
        toastAll->setFixedSize(72, 28);
        toastOne->setToolTip(QStringLiteral("按当前选择的位置发送一条 Toast"));
        toastAll->setToolTip(QStringLiteral("在所有位置依次弹出 Toast（用于对比布局）"));

        tl->addWidget(toastHint);
        tl->addWidget(toastPosCombo);
        tl->addWidget(toastOne);
        tl->addWidget(toastAll);

        window.setFluentTitleBarRightWidget(toastControls);

        auto posName = [](FluentToast::Position p) {
            switch (p) {
            case FluentToast::Position::TopLeft:
                return QStringLiteral("左上");
            case FluentToast::Position::TopCenter:
                return QStringLiteral("顶部居中");
            case FluentToast::Position::TopRight:
                return QStringLiteral("右上");
            case FluentToast::Position::BottomLeft:
                return QStringLiteral("左下");
            case FluentToast::Position::BottomCenter:
                return QStringLiteral("底部居中");
            case FluentToast::Position::BottomRight:
                return QStringLiteral("右下");
            }
            return QStringLiteral("右下");
        };

        m_toastPosition = FluentToast::Position::BottomRight;
        QObject::connect(toastPosCombo,
                         QOverload<int>::of(&QComboBox::currentIndexChanged),
                         this,
                         [this, toastPosCombo](int) {
                             m_toastPosition = static_cast<FluentToast::Position>(toastPosCombo->currentData().toInt());
                         });

        auto showToast = [this, posName]() {
            FluentToast::showToast(this,
                                  QStringLiteral("Toast"),
                                  QStringLiteral("当前弹出位置：%1（点击可关闭）").arg(posName(m_toastPosition)),
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
                                          QStringLiteral("位置：%1（点击可关闭）").arg(posName(p)),
                                          p,
                                          2400);
                });
            }
        });
    }

    auto *root = new FluentWidget();
    root->setBackgroundRole(FluentWidget::BackgroundRole::WindowBackground);
    window.setCentralWidget(root);

    auto *rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0, 8, 16, 16);
    rootLayout->setSpacing(0);

    // ---- NavigationView (left pane) ----
    auto *nav = new FluentNavigationView(root);
    nav->setExpandedWidth(280);
    nav->setCompactWidth(48);
    nav->setAutoCollapseWidth(800);

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
        overview.text = QStringLiteral("总览");
        applyGlyph(overview, 0xE80F);
        mainItems.push_back(overview);

        // "基本输入" category with sub-items
        NI basicInput;
        basicInput.key  = QStringLiteral("basic_input");
        basicInput.text = QStringLiteral("基本输入");
        applyGlyph(basicInput, 0xE961);
        {
            NI inputs;
            inputs.key  = QStringLiteral("inputs");
            inputs.text = QStringLiteral("输入");
            applyGlyph(inputs, 0xEF60);
            basicInput.children.push_back(inputs);

            NI buttons;
            buttons.key  = QStringLiteral("buttons");
            buttons.text = QStringLiteral("按钮/开关");
            applyGlyph(buttons, 0xF19F);
            basicInput.children.push_back(buttons);
        }
        mainItems.push_back(basicInput);

        // "选择器" category
        NI pickers;
        pickers.key  = QStringLiteral("pickers");
        pickers.text = QStringLiteral("选择器");
        applyGlyph(pickers, 0xE787);
        mainItems.push_back(pickers);

        NI angles;
        angles.key  = QStringLiteral("angles");
        angles.text = QStringLiteral("角度控件");
        applyGlyph(angles, 0xF0B4);
        mainItems.push_back(angles);

        NI dataViews;
        dataViews.key  = QStringLiteral("dataviews");
        dataViews.text = QStringLiteral("数据视图");
        applyGlyph(dataViews, 0xEA37);
        mainItems.push_back(dataViews);

        NI containers;
        containers.key  = QStringLiteral("containers");
        containers.text = QStringLiteral("容器/布局");
        applyGlyph(containers, 0xF168);
        mainItems.push_back(containers);

        NI windows;
        windows.key  = QStringLiteral("windows");
        windows.text = QStringLiteral("窗口/对话框");
        applyGlyph(windows, 0xE73F);
        mainItems.push_back(windows);
    }
    nav->setItems(mainItems);

    // Footer items (e.g. Settings / Theme)
    std::vector<NI> footerItems;
    {
        NI settings;
        settings.key  = QStringLiteral("settings");
        settings.text = QStringLiteral("设置");
        applyGlyph(settings, 0xE713);
        footerItems.push_back(settings);
    }
    nav->setFooterItems(footerItems);

    // Default selection
    nav->setSelectedKey(QStringLiteral("overview"));

    // ---- Content area (stacked pages) ----
    auto *stack = new QStackedWidget(root);

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
    QObject::connect(nav, &FluentNavigationView::selectedKeyChanged, this, [stack](const QString &key) {
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

    rootLayout->addWidget(nav);
    rootLayout->addSpacing(8);
    rootLayout->addWidget(stack, 1);

    QObject::connect(msgInfo, &QAction::triggered, this, [this]() {
        FluentMessageBox::information(this,
                                     QStringLiteral("消息框"),
                                     QStringLiteral("这是从菜单触发的消息框示例。"),
                                     QStringLiteral("用于展示 MessageBox 与 Theme/Accent 联动。"));
    });

    QObject::connect(dlgAction, &QAction::triggered, this, [nav]() {
        nav->setSelectedKey(QStringLiteral("windows"));
    });
}

} // namespace Demo
