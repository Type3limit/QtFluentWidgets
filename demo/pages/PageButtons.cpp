#include "PageButtons.h"

#include "../DemoHelpers.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

#include "Fluent/FluentButton.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentCheckBox.h"
#include "Fluent/FluentMainWindow.h"
#include "Fluent/FluentProgressBar.h"
#include "Fluent/FluentRadioButton.h"
#include "Fluent/FluentScrollArea.h"
#include "Fluent/FluentToggleSwitch.h"
#include "Fluent/FluentToolButton.h"

namespace Demo::Pages {

using namespace Fluent;

QWidget *createButtonsPage(FluentMainWindow *window)
{
    return Demo::makePage([&](QVBoxLayout *page) {
        auto s = Demo::makeSection(QStringLiteral("按钮 / 开关"),
                                   QStringLiteral("包含 primary/secondary、disabled、checked，以及 CheckBox/Radio/Toggle"));

        page->addWidget(s.card);

        // FluentButton demo
        {
            QString code;
#define BUTTONS_FLUENT_BUTTON(X) \
    X(auto *btnRow = new QHBoxLayout();) \
    X(btnRow->setContentsMargins(0, 0, 0, 0);) \
    X(btnRow->setSpacing(10);) \
    X(auto *p = new FluentButton(QStringLiteral("Primary"));) \
    X(p->setPrimary(true);) \
    X(auto *pDis = new FluentButton(QStringLiteral("Primary Disabled"));) \
    X(pDis->setPrimary(true);) \
    X(pDis->setDisabled(true);) \
    X(auto *sec = new FluentButton(QStringLiteral("Secondary"));) \
    X(auto *secChecked = new FluentButton(QStringLiteral("Secondary Checked"));) \
    X(secChecked->setCheckable(true);) \
    X(secChecked->setChecked(true);) \
    X(btnRow->addWidget(p);) \
    X(btnRow->addWidget(pDis);) \
    X(btnRow->addWidget(sec);) \
    X(btnRow->addWidget(secChecked);) \
    X(btnRow->addStretch(1);) \
    X(body->addLayout(btnRow);)

#define X(line) code += QStringLiteral(#line "\n");
            BUTTONS_FLUENT_BUTTON(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentButton"),
                QStringLiteral("Primary/Secondary、禁用、可勾选状态"),
                QStringLiteral("要点：\n"
                               "-setPrimary(true) 切换主按钮样式\n"
                               "-setDisabled(true) 展示禁用态\n"
                               "-setCheckable(true) + setChecked(true) 展示 Checked"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    BUTTONS_FLUENT_BUTTON(X)
#undef X
                },
                false));

#undef BUTTONS_FLUENT_BUTTON
        }

        // FluentToolButton demo
        {
            QString code;
#define BUTTONS_TOOL_BUTTON(X) \
    X(auto *toolRow = new QHBoxLayout();) \
    X(toolRow->setContentsMargins(0, 0, 0, 0);) \
    X(toolRow->setSpacing(10);) \
    X(auto *tb1 = new FluentToolButton(QStringLiteral("Tool"));) \
    X(auto *tb2 = new FluentToolButton(QStringLiteral("Tool Checked"));) \
    X(tb2->setCheckable(true);) \
    X(tb2->setChecked(true);) \
    X(toolRow->addWidget(tb1);) \
    X(toolRow->addWidget(tb2);) \
    X(toolRow->addStretch(1);) \
    X(body->addLayout(toolRow);)

#define X(line) code += QStringLiteral(#line "\n");
            BUTTONS_TOOL_BUTTON(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentToolButton"),
                QStringLiteral("更紧凑的按钮，适合工具栏/图标动作"),
                QStringLiteral("要点：\n"
                               "-可 checkable（用于 Toggle 工具）\n"
                               "-尺寸更小，适合与菜单/弹出配合"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    BUTTONS_TOOL_BUTTON(X)
#undef X
                }));

#undef BUTTONS_TOOL_BUTTON
        }

        // Check/Radio/Toggle demo
        {
            QString code;
#define BUTTONS_OPTION(X) \
    X(auto *optRow = new QHBoxLayout();) \
    X(optRow->setContentsMargins(0, 0, 0, 0);) \
    X(optRow->setSpacing(14);) \
    X(auto *cb = new FluentCheckBox(QStringLiteral("CheckBox"));) \
    X(auto *ra = new FluentRadioButton(QStringLiteral("Radio A"));) \
    X(auto *rb = new FluentRadioButton(QStringLiteral("Radio B"));) \
    X(ra->setChecked(true);) \
    X(auto *tg = new FluentToggleSwitch(QStringLiteral("Toggle"));) \
    X(tg->setChecked(true);) \
    X(optRow->addWidget(cb);) \
    X(optRow->addWidget(ra);) \
    X(optRow->addWidget(rb);) \
    X(optRow->addWidget(tg);) \
    X(optRow->addStretch(1);) \
    X(body->addLayout(optRow);)

#define X(line) code += QStringLiteral(#line "\n");
            BUTTONS_OPTION(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("CheckBox / RadioButton / ToggleSwitch"),
                QStringLiteral("常见选择/开关控件"),
                QStringLiteral("要点：\n"
                               "-CheckBox：多选\n"
                               "-RadioButton：单选（同父布局/同组）\n"
                               "-ToggleSwitch：开关语义更明确"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    BUTTONS_OPTION(X)
#undef X
                }));

#undef BUTTONS_OPTION
        }

        // ProgressBar demo
        {
            QString code;
#define BUTTONS_PROGRESS(X) \
    X(auto *pb = new FluentProgressBar();) \
    X(pb->setRange(0, 100);) \
    X(pb->setValue(62);) \
    X(pb->setFixedWidth(340);) \
    X(body->addWidget(pb);)

#define X(line) code += QStringLiteral(#line "\n");
            BUTTONS_PROGRESS(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentProgressBar"),
                QStringLiteral("进度条（Accent 会影响进度颜色）"),
                QStringLiteral("要点：\n"
                               "-setRange()/setValue()\n"
                               "-主题/Accent 联动（填充色、背景）"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    BUTTONS_PROGRESS(X)
#undef X
                }));

#undef BUTTONS_PROGRESS
        }

        Q_UNUSED(window);
    });
}

} // namespace Demo::Pages
