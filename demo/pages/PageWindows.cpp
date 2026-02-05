#include "PageWindows.h"

#include "../DemoHelpers.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "Fluent/FluentButton.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentDialog.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentMainWindow.h"
#include "Fluent/FluentMenu.h"
#include "Fluent/FluentMessageBox.h"
#include "Fluent/FluentScrollArea.h"
#include "Fluent/FluentToast.h"
#include "Fluent/FluentTextEdit.h"
#include "Fluent/FluentToggleSwitch.h"
#include "Fluent/FluentWidget.h"

#include <QCursor>

namespace Demo::Pages {

using namespace Fluent;

QWidget *createWindowsPage(FluentMainWindow *window)
{
    return Demo::makePage([&](QVBoxLayout *page) {
        auto s = Demo::makeSection(QStringLiteral("窗口 / 对话框"),
                                   QStringLiteral("FluentDialog 可选 resize；FluentMessageBox 四种类型"));
        page->addWidget(s.card);

        // FluentMessageBox
        {
            QString code;
#define WINDOWS_MSGBOX(X) \
    X(auto *row = new QHBoxLayout(); ) \
    X(row->setContentsMargins(0, 0, 0, 0); ) \
    X(row->setSpacing(10); ) \
    X(auto *maskToggle = new FluentToggleSwitch(QStringLiteral("启用蒙版")); ) \
    X(maskToggle->setChecked(false); ) \
    X(auto *openQuestion = new FluentButton(QStringLiteral("Question")); ) \
    X(auto *openWarn = new FluentButton(QStringLiteral("Warning")); ) \
    X(auto *openErr = new FluentButton(QStringLiteral("Error")); ) \
    X(row->addWidget(maskToggle); ) \
    X(row->addWidget(openQuestion); ) \
    X(row->addWidget(openWarn); ) \
    X(row->addWidget(openErr); ) \
    X(row->addStretch(1); ) \
    X(body->addLayout(row); ) \
    X(QObject::connect(openQuestion, &QPushButton::clicked, window, [=]() { FluentMessageBox::question(window, QStringLiteral("确认"), QStringLiteral("你觉得这个 demo 展示够全面吗？"), QStringLiteral("切换主题/Accent 观察全控件联动。"), maskToggle->isChecked()); }); ) \
    X(QObject::connect(openWarn, &QPushButton::clicked, window, [=]() { FluentMessageBox::warning(window, QStringLiteral("警告"), QStringLiteral("这是 Warning 示例"), QStringLiteral("用于演示 message box 样式与主题联动。"), maskToggle->isChecked()); }); ) \
    X(QObject::connect(openErr, &QPushButton::clicked, window, [=]() { FluentMessageBox::error(window, QStringLiteral("错误"), QStringLiteral("这是 Error 示例"), QStringLiteral("用于演示 error 色与边框/阴影。"), maskToggle->isChecked()); }); )

#define X(line) code += QStringLiteral(#line "\n");
            WINDOWS_MSGBOX(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentMessageBox"),
                QStringLiteral("四种类型：info/question/warning/error"),
                QStringLiteral("要点：\n"
                               "-静态函数一行调用\n"
                               "-可传入 maskEnabled 控制是否启用 overlay"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    WINDOWS_MSGBOX(X)
#undef X
                },
                false,
                210));

#undef WINDOWS_MSGBOX
        }

        // FluentDialog (Basics)
        {
            QString code;
#define WINDOWS_DIALOG_BASIC(X) \
    X(auto *row = new QHBoxLayout(); ) \
    X(row->setContentsMargins(0, 0, 0, 0); ) \
    X(row->setSpacing(10); ) \
    X(auto *maskToggle = new FluentToggleSwitch(QStringLiteral("启用蒙版")); ) \
    X(maskToggle->setChecked(false); ) \
    X(auto *resizeToggle = new FluentToggleSwitch(QStringLiteral("启用 Resize")); ) \
    X(resizeToggle->setChecked(false); ) \
    X(auto *openDialog = new FluentButton(QStringLiteral("打开 FluentDialog")); ) \
    X(row->addWidget(maskToggle); ) \
    X(row->addWidget(resizeToggle); ) \
    X(row->addWidget(openDialog); ) \
    X(row->addStretch(1); ) \
    X(body->addLayout(row); ) \
    X(QObject::connect(openDialog, &QPushButton::clicked, window, [=]() { FluentDialog dlg(window); dlg.setWindowTitle(QStringLiteral("FluentDialog")); dlg.setModal(true); dlg.setMaskEnabled(maskToggle->isChecked()); dlg.setFluentResizeEnabled(resizeToggle->isChecked()); dlg.resize(520, 320); auto *content = new FluentWidget(&dlg); content->setBackgroundRole(FluentWidget::BackgroundRole::Transparent); auto *l = new QVBoxLayout(content); l->setContentsMargins(18, 18, 18, 18); l->setSpacing(12); auto *title = new FluentLabel(QStringLiteral("对话框内容")); title->setStyleSheet("font-size: 16px; font-weight: 600;"); auto *hint = new FluentLabel(QStringLiteral("这是一个简化示例：只演示蒙版与 Resize 开关。")); hint->setStyleSheet("font-size: 12px; opacity: 0.85;"); auto *closeBtn = new FluentButton(QStringLiteral("关闭")); closeBtn->setPrimary(true); QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept); l->addWidget(title); l->addWidget(hint); l->addStretch(1); l->addWidget(closeBtn); auto *dlgLayout = new QVBoxLayout(&dlg); dlgLayout->setContentsMargins(0, 0, 0, 0); dlgLayout->addWidget(content); dlg.exec(); }); )

#define X(line) code += QStringLiteral(#line "\n");
            WINDOWS_DIALOG_BASIC(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentDialog"),
                QStringLiteral("对话框容器：可选蒙版 overlay、可选 Resize"),
                QStringLiteral("要点：\n"
                               "-setMaskEnabled(true) 开启蒙版\n"
                               "-setFluentResizeEnabled(true) 开启可调整大小"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    WINDOWS_DIALOG_BASIC(X)
#undef X
                },
                true,
                240));

#undef WINDOWS_DIALOG_BASIC
        }

        // FluentDialog (Window Buttons)
        {
            QString code;
#define WINDOWS_DIALOG_BUTTONS(X) \
    X(auto *row = new QHBoxLayout(); ) \
    X(row->setContentsMargins(0, 0, 0, 0); ) \
    X(row->setSpacing(10); ) \
    X(auto *openDialog = new FluentButton(QStringLiteral("窗口按钮示例")); ) \
    X(row->addWidget(openDialog); ) \
    X(row->addStretch(1); ) \
    X(body->addLayout(row); ) \
    X(QObject::connect(openDialog, &QPushButton::clicked, window, [=]() { FluentDialog dlg(window); dlg.setWindowTitle(QStringLiteral("Window Buttons")); dlg.setModal(true); dlg.resize(520, 340); auto *content = new FluentWidget(&dlg); content->setBackgroundRole(FluentWidget::BackgroundRole::Transparent); auto *l = new QVBoxLayout(content); l->setContentsMargins(18, 18, 18, 18); l->setSpacing(10); auto *minBtnToggle = new FluentToggleSwitch(QStringLiteral("显示最小化按钮")); auto *maxBtnToggle = new FluentToggleSwitch(QStringLiteral("显示最大化按钮")); auto *closeBtnToggle = new FluentToggleSwitch(QStringLiteral("显示关闭按钮")); minBtnToggle->setChecked(true); maxBtnToggle->setChecked(true); closeBtnToggle->setChecked(true); auto updateDialogButtons = [&dlg, minBtnToggle, maxBtnToggle, closeBtnToggle]() { FluentDialog::WindowButtons buttons; if (minBtnToggle->isChecked()) buttons |= FluentDialog::MinimizeButton; if (maxBtnToggle->isChecked()) buttons |= FluentDialog::MaximizeButton; if (closeBtnToggle->isChecked()) buttons |= FluentDialog::CloseButton; dlg.setFluentWindowButtons(buttons); }; QObject::connect(minBtnToggle, &FluentToggleSwitch::toggled, &dlg, [&](bool) { updateDialogButtons(); }); QObject::connect(maxBtnToggle, &FluentToggleSwitch::toggled, &dlg, [&](bool) { updateDialogButtons(); }); QObject::connect(closeBtnToggle, &FluentToggleSwitch::toggled, &dlg, [&](bool) { updateDialogButtons(); }); auto *closeBtn = new FluentButton(QStringLiteral("关闭")); closeBtn->setPrimary(true); QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept); updateDialogButtons(); l->addWidget(minBtnToggle); l->addWidget(maxBtnToggle); l->addWidget(closeBtnToggle); l->addStretch(1); l->addWidget(closeBtn); auto *dlgLayout = new QVBoxLayout(&dlg); dlgLayout->setContentsMargins(0, 0, 0, 0); dlgLayout->addWidget(content); dlg.exec(); }); )

#define X(line) code += QStringLiteral(#line "\n");
            WINDOWS_DIALOG_BUTTONS(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentDialog"),
                QStringLiteral("窗口按钮：最小化/最大化/关闭"),
                QStringLiteral("要点：\n"
                               "-setFluentWindowButtons() 控制窗口按钮组合\n"
                               "-常用于需要自定义标题栏行为的窗口/对话框"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    WINDOWS_DIALOG_BUTTONS(X)
#undef X
                },
                true,
                250));

#undef WINDOWS_DIALOG_BUTTONS
        }

        // FluentMenu
        {
            QString code;
#define WINDOWS_MENU(X) \
    X(auto *row = new QHBoxLayout(); ) \
    X(row->setContentsMargins(0, 0, 0, 0); ) \
    X(row->setSpacing(10); ) \
    X(auto *openMenu = new FluentButton(QStringLiteral("弹出 FluentMenu")); ) \
    X(row->addWidget(openMenu); ) \
    X(row->addStretch(1); ) \
    X(body->addLayout(row); ) \
    X(QObject::connect(openMenu, &QPushButton::clicked, window, [window]() { auto *menu = new FluentMenu(window); menu->addAction(QStringLiteral("操作 A")); menu->addAction(QStringLiteral("操作 B")); auto *sub = menu->addFluentMenu(QStringLiteral("更多")); sub->addAction(QStringLiteral("子菜单项")); menu->exec(QCursor::pos()); menu->deleteLater(); }); )

#define X(line) code += QStringLiteral(#line "\n");
            WINDOWS_MENU(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentMenu"),
                QStringLiteral("弹出菜单：支持子菜单"),
                QStringLiteral("要点：\n"
                               "-addAction() 添加菜单项\n"
                               "-addFluentMenu() 添加子菜单\n"
                               "-exec(QCursor::pos()) 弹出"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    WINDOWS_MENU(X)
#undef X
                },
                true,
                210));

#undef WINDOWS_MENU
        }

        // FluentToast
        {
            QString code;
#define WINDOWS_TOAST(X) \
    X(auto *row = new QHBoxLayout(); ) \
    X(row->setContentsMargins(0, 0, 0, 0); ) \
    X(row->setSpacing(10); ) \
    X(auto *openToast = new FluentButton(QStringLiteral("发一条 Toast")); ) \
    X(row->addWidget(openToast); ) \
    X(row->addStretch(1); ) \
    X(body->addLayout(row); ) \
    X(QObject::connect(openToast, &QPushButton::clicked, window, [window]() { FluentToast::showToast(window, QStringLiteral("Toast"), QStringLiteral("这是一条 Toast（点击可关闭）"), FluentToast::Position::BottomRight, 2400); }); )

#define X(line) code += QStringLiteral(#line "\n");
            WINDOWS_TOAST(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentToast"),
                QStringLiteral("轻提示：位置/持续时间可配置"),
                QStringLiteral("要点：\n"
                               "-showToast(host, title, message, pos, durationMs)\n"
                               "-点击可关闭（默认行为）"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    WINDOWS_TOAST(X)
#undef X
                },
                true,
                200));

#undef WINDOWS_TOAST
        }

        page->addWidget(Demo::makeCollapsedCard(
            QStringLiteral("Window Chrome"),
            QStringLiteral("窗口框架相关控件（适合在主窗口中演示）"),
            QStringLiteral("包含：\n"
                           "-FluentMainWindow\n"
                           "-FluentMenuBar / FluentToolBar / FluentStatusBar\n"
                           "-FluentResizeHelper\n"
                           "-FluentTheme / FluentStyle"),
            QStringLiteral(
                "// 通常在 FluentMainWindow 里组合使用\n"
                "// 并监听 ThemeManager::themeChanged 做联动\n")));
    });
}

} // namespace Demo::Pages
