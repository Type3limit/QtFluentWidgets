#include "PageOverview.h"

#include "../DemoHelpers.h"

#include "Fluent/FluentAngleSelector.h"
#include "Fluent/FluentButton.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentDial.h"
#include "Fluent/FluentInfoBar.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentLineEdit.h"
#include "Fluent/FluentMainWindow.h"
#include "Fluent/FluentProgressBar.h"
#include "Fluent/FluentSlider.h"
#include "Fluent/FluentSpinBox.h"
#include "Fluent/FluentTabWidget.h"
#include "Fluent/FluentTextEdit.h"
#include "Fluent/FluentToggleSwitch.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace Demo::Pages {

using namespace Fluent;

namespace {

FluentLabel *makeBodyText(const QString &text)
{
    auto *label = new FluentLabel(text);
    label->setWordWrap(true);
    label->setStyleSheet(QStringLiteral("font-size: 12px; opacity: 0.84;"));
    return label;
}

FluentLabel *makeCaption(const QString &text)
{
    auto *label = new FluentLabel(text);
    label->setStyleSheet(QStringLiteral("font-size: 12px; font-weight: 650; opacity: 0.9;"));
    return label;
}

void addStretchingRow(QVBoxLayout *body, QHBoxLayout *row)
{
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(10);
    row->addStretch(1);
    body->addLayout(row);
}

} // namespace

QWidget *createOverviewPage(FluentMainWindow *window, const std::function<void(int)> &jumpTo)
{
    Q_UNUSED(window)

    return Demo::makePage([=](QVBoxLayout *page) {
        page->addWidget(Demo::makePageHeader(DEMO_TEXT("总览","OVERVIEW"), DEMO_TEXT("Fluent，为 Qt 桌面重制","Fluent, rebuilt for Qt desktop"), DEMO_TEXT("一套基于 Qt Widgets 的 Fluent Design 控件库，统一 ThemeManager 与 Style 联动。","A Fluent Design widget set on Qt Widgets, unified via ThemeManager and Style.")));

        // Showcase: FluentCard's opt-in animated flow background.
        {
            auto flow = Demo::makeSection(DEMO_TEXT("流光背景 Flow background", "Flow background"),
                                          DEMO_TEXT("FluentCard 可单独开启动态流光背景（线性渐变，跟随主题 accent，自动流转）。",
                                                    "FluentCard can opt into an animated flow background (linear gradient, follows the accent)."));
            flow.card->setFlowBackgroundEnabled(true);
            flow.body->addWidget(makeBodyText(DEMO_TEXT("内容在流光之上依旧清晰可读；渐变配色可在「设置 → 描边」里自定义，描边与卡片共用同一套。",
                                                        "Content stays readable over the flow; customise the gradient colours in Settings → Border (shared with the accent border).")));
            auto *row = new QHBoxLayout();
            auto *primary = new FluentButton(DEMO_TEXT("主按钮", "Primary"));
            primary->setPrimary(true);
            row->addWidget(primary);
            row->addWidget(new FluentButton(DEMO_TEXT("默认", "Default")));
            auto *toggle = new FluentToggleSwitch();
            toggle->setChecked(true);
            toggle->setText(DEMO_TEXT("启用", "On"));
            row->addWidget(toggle);
            addStretchingRow(flow.body, row);
            page->addWidget(flow.card);
        }

        const auto addJumpButton = [jumpTo](QHBoxLayout *row, const QString &text, int pageIndex, bool primary = false) {
            auto *button = new FluentButton(text);
            button->setPrimary(primary);
            QObject::connect(button, &QPushButton::clicked, button, [jumpTo, pageIndex]() {
                jumpTo(pageIndex);
            });
            row->addWidget(button);
            return button;
        };

        const auto addSection = [page](const QString &title,
                                       const QString &subtitle,
                                       const std::function<void(QVBoxLayout *)> &fill) {
            auto section = Demo::makeSection(title, subtitle);
            fill(section.body);
            page->addWidget(section.card);
        };

        addSection(
            QStringLiteral("Qt Fluent"),
            DEMO_TEXT("按场景进入子页，细节留给对应 card 展示。", "Jump by scenario; detailed controls live on the dedicated pages."),
            [&](QVBoxLayout *body) {
                body->addWidget(makeBodyText(DEMO_TEXT("总览页只保留入口、少量状态样例和导航线索。右侧标注滚动条会跟随这些 card 自动生成分段。",
                                                       "The overview keeps only entry points, a few state samples, and navigation cues. The annotated scrollbar is generated from these cards.")));

                auto *row = new QHBoxLayout();
                addJumpButton(row, DEMO_TEXT("基本输入", "Basic Input"), 1, true);
                addJumpButton(row, DEMO_TEXT("容器 / 布局", "Containers"), 5);
                addJumpButton(row, DEMO_TEXT("窗口 / 对话框", "Windows"), 6);
                addStretchingRow(body, row);
            });

        addSection(
            DEMO_TEXT("基本输入", "Basic Input"),
            DEMO_TEXT("输入、按钮、选择和值编辑入口。", "Inputs, buttons, selection, and value editing."),
            [&](QVBoxLayout *body) {
                auto *gridHost = new QWidget();
                auto *grid = new QGridLayout(gridHost);
                grid->setContentsMargins(0, 0, 0, 0);
                grid->setHorizontalSpacing(16);
                grid->setVerticalSpacing(10);

                auto *lineEdit = new FluentLineEdit();
                lineEdit->setPlaceholderText(DEMO_TEXT("搜索或输入", "Search or type"));

                auto *slider = new FluentSlider(Qt::Horizontal);
                slider->setRange(0, 100);
                slider->setValue(42);

                auto *spin = new FluentSpinBox();
                spin->setRange(0, 999);
                spin->setValue(128);

                auto *toggle = new FluentToggleSwitch(DEMO_TEXT("启用", "Enabled"));
                toggle->setChecked(true);

                grid->addWidget(makeCaption(QStringLiteral("LineEdit")), 0, 0);
                grid->addWidget(lineEdit, 0, 1);
                grid->addWidget(makeCaption(QStringLiteral("Slider")), 1, 0);
                grid->addWidget(slider, 1, 1);
                grid->addWidget(makeCaption(QStringLiteral("SpinBox")), 2, 0);
                grid->addWidget(spin, 2, 1);
                grid->addWidget(makeCaption(QStringLiteral("Toggle")), 3, 0);
                grid->addWidget(toggle, 3, 1);
                grid->setColumnStretch(1, 1);
                body->addWidget(gridHost);

                auto *row = new QHBoxLayout();
                addJumpButton(row, DEMO_TEXT("输入页", "Inputs"), 1, true);
                addJumpButton(row, DEMO_TEXT("按钮 / 开关", "Buttons"), 2);
                addStretchingRow(body, row);
            });

        addSection(
            DEMO_TEXT("角度控件", "Angle Controls"),
            DEMO_TEXT("FluentDial 和 FluentAngleSelector 归入基本输入分类。", "FluentDial and FluentAngleSelector belong under Basic Input."),
            [&](QVBoxLayout *body) {
                auto *rowHost = new QWidget();
                auto *row = new QHBoxLayout(rowHost);
                row->setContentsMargins(0, 0, 0, 0);
                row->setSpacing(18);

                auto *dial = new FluentDial();
                dial->setFixedSize(74, 74);
                dial->setTicksVisible(true);
                dial->setValue(135);

                auto *selector = new FluentAngleSelector();
                selector->setValue(210);

                row->addWidget(dial, 0, Qt::AlignVCenter);
                row->addWidget(selector, 0, Qt::AlignVCenter);
                row->addStretch(1);
                body->addWidget(rowHost);

                auto *buttonRow = new QHBoxLayout();
                addJumpButton(buttonRow, DEMO_TEXT("打开角度控件页", "Open Angle Controls"), 1, true);
                addStretchingRow(body, buttonRow);
            });

        addSection(
            DEMO_TEXT("选择器", "Pickers"),
            DEMO_TEXT("日期、时间、颜色等选择器集中在一个分页。", "Date, time, color, and related pickers are grouped together."),
            [&](QVBoxLayout *body) {
                auto *combo = new FluentComboBox();
                combo->addItems({DEMO_TEXT("日程", "Schedule"), DEMO_TEXT("颜色", "Color"), DEMO_TEXT("时间", "Time")});
                combo->setCurrentIndex(1);
                body->addWidget(combo);

                auto *row = new QHBoxLayout();
                addJumpButton(row, DEMO_TEXT("打开选择器页", "Open Pickers"), 3, true);
                addStretchingRow(body, row);
            });

        addSection(
            DEMO_TEXT("视觉与动效", "Icons And Motion"),
            DEMO_TEXT("图标矩阵与 Lottie 动效分开维护。", "Icon matrices and Lottie motion are maintained separately."),
            [&](QVBoxLayout *body) {
                auto *status = new FluentInfoBar(FluentInfoBar::Severity::Info,
                                                 DEMO_TEXT("主题联动", "Theme linked"),
                                                 DEMO_TEXT("图标、动效和 accent 会跟随全局主题变化。",
                                                           "Icons, motion, and accent follow the global theme."),
                                                 nullptr);
                status->setClosable(false);
                body->addWidget(status);

                auto *row = new QHBoxLayout();
                addJumpButton(row, DEMO_TEXT("图标", "Icons"), 7, true);
                addJumpButton(row, DEMO_TEXT("动态", "Motion"), 7);
                addStretchingRow(body, row);
            });

        addSection(
            DEMO_TEXT("数据视图", "Data Views"),
            DEMO_TEXT("列表、表格和树的 hover / selected 状态统一在数据视图页检查。",
                      "List, table, and tree hover / selected states are checked on the Data Views page."),
            [&](QVBoxLayout *body) {
                auto *progress = new FluentProgressBar();
                progress->setRange(0, 100);
                progress->setValue(68);
                body->addWidget(progress);

                auto *row = new QHBoxLayout();
                addJumpButton(row, DEMO_TEXT("打开数据视图页", "Open Data Views"), 4, true);
                addStretchingRow(body, row);
            });

        addSection(
            DEMO_TEXT("容器 / 布局", "Containers / Layout"),
            DEMO_TEXT("NavigationView、TabWidget、Card、ScrollArea 和 AnnotatedScrollBar 的集中示例。",
                      "Central examples for NavigationView, TabWidget, Card, ScrollArea, and AnnotatedScrollBar."),
            [&](QVBoxLayout *body) {
                auto *tabs = new FluentTabWidget();
                tabs->setFixedHeight(132);
                tabs->addTab(new FluentLabel(QStringLiteral("Overview")), QStringLiteral("Overview"));
                tabs->addTab(new FluentLabel(QStringLiteral("Details")), QStringLiteral("Details"));
                tabs->setCurrentIndex(0);
                body->addWidget(tabs);

                auto *row = new QHBoxLayout();
                addJumpButton(row, DEMO_TEXT("打开容器 / 布局页", "Open Containers"), 5, true);
                addStretchingRow(body, row);
            });

        addSection(
            DEMO_TEXT("窗口 / 对话框", "Windows / Dialogs"),
            DEMO_TEXT("弹层、Toast、菜单和窗口 chrome 的入口。", "Entry point for popups, toast, menus, and window chrome."),
            [&](QVBoxLayout *body) {
                auto *editor = new FluentTextEdit();
                editor->setFixedHeight(92);
                editor->setPlaceholderText(DEMO_TEXT("窗口页包含 Dialog / MessageBox / Menu / Toast。",
                                                     "The Windows page contains Dialog, MessageBox, Menu, and Toast."));
                body->addWidget(editor);

                auto *row = new QHBoxLayout();
                addJumpButton(row, DEMO_TEXT("打开窗口 / 对话框页", "Open Windows"), 6, true);
                addStretchingRow(body, row);
            });
    });
}

} // namespace Demo::Pages
