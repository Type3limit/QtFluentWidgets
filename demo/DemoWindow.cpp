#include "DemoWindow.h"

#include "DemoHelpers.h"
#include "DemoSidebar.h"

#include "pages/PageButtons.h"
#include "pages/PageContainers.h"
#include "pages/PageDataViews.h"
#include "pages/PageInputs.h"
#include "pages/PageOverview.h"
#include "pages/PagePickers.h"
#include "pages/PageWindows.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QCursor>
#include <QHBoxLayout>
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
    lightAction->setCheckable(true);
    darkAction->setCheckable(true);
    lightAction->setChecked(true);
    QObject::connect(lightAction, &QAction::triggered, []() { ThemeManager::instance().setLightMode(); });
    QObject::connect(darkAction, &QAction::triggered, []() { ThemeManager::instance().setDarkMode(); });
    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [lightAction, darkAction]() {
        const bool isDark = ThemeManager::instance().themeMode() == ThemeManager::ThemeMode::Dark;
        if (darkAction->isChecked() != isDark) {
            (isDark ? darkAction : lightAction)->setChecked(true);
        }
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
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(14);

    auto *sidebar = new DemoSidebar(&window, root, false);

    auto *tabs = new FluentTabWidget(root);
    tabs->setTabPosition(QTabWidget::West);

    const auto jumpTo = [tabs](int pageIndex) {
        if (!tabs) {
            return;
        }
        if (pageIndex >= 0 && pageIndex < tabs->count()) {
            tabs->setCurrentIndex(pageIndex);
        }
    };

    tabs->addTab(Demo::Pages::createOverviewPage(&window, jumpTo), QStringLiteral("总览"));
    tabs->addTab(Demo::Pages::createInputsPage(&window), QStringLiteral("输入"));
    tabs->addTab(Demo::Pages::createButtonsPage(&window), QStringLiteral("按钮"));
    tabs->addTab(Demo::Pages::createPickersPage(&window), QStringLiteral("选择器"));
    tabs->addTab(Demo::Pages::createDataViewsPage(&window), QStringLiteral("数据视图"));
    tabs->addTab(Demo::Pages::createContainersPage(&window), QStringLiteral("容器"));
    tabs->addTab(Demo::Pages::createWindowsPage(&window), QStringLiteral("窗口"));

    rootLayout->addWidget(sidebar);
    rootLayout->addWidget(tabs, 1);

    QObject::connect(msgInfo, &QAction::triggered, this, [this]() {
        FluentMessageBox::information(this,
                                     QStringLiteral("消息框"),
                                     QStringLiteral("这是从菜单触发的消息框示例。"),
                                     QStringLiteral("用于展示 MessageBox 与 Theme/Accent 联动。"));
    });

    QObject::connect(dlgAction, &QAction::triggered, this, [tabs]() {
        if (tabs && tabs->count() > 6) {
            tabs->setCurrentIndex(6);
        }
    });
}

} // namespace Demo
