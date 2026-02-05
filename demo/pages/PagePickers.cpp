#include "PagePickers.h"

#include "../DemoHelpers.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

#include "Fluent/FluentButton.h"
#include "Fluent/FluentCalendarPicker.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentColorDialog.h"
#include "Fluent/FluentColorPicker.h"
#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentMainWindow.h"
#include "Fluent/FluentScrollArea.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentTimePicker.h"

namespace Demo::Pages {

using namespace Fluent;

QWidget *createPickersPage(FluentMainWindow *window)
{
    return Demo::makePage([&](QVBoxLayout *page) {
        auto s = Demo::makeSection(QStringLiteral("选择器"),
                                   QStringLiteral("Calendar / Time / ComboBox（联动 Accent）"));

        page->addWidget(s.card);

        // Calendar + Time
        {
            QString code;
#define PICKERS_CAL_TIME(X) \
    X(auto *row = new QHBoxLayout(); ) \
    X(row->setContentsMargins(0, 0, 0, 0); ) \
    X(row->setSpacing(10); ) \
    X(auto *cal = new FluentCalendarPicker(); ) \
    X(cal->setFixedWidth(320); ) \
    X(auto *time = new FluentTimePicker(); ) \
    X(time->setFixedWidth(160); ) \
    X(row->addWidget(cal); ) \
    X(row->addWidget(time); ) \
    X(row->addStretch(1); ) \
    X(body->addLayout(row); )

#define X(line) code += QStringLiteral(#line "\n");
            PICKERS_CAL_TIME(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("CalendarPicker / TimePicker"),
                QStringLiteral("日期与时间选择器"),
                QStringLiteral("要点：\n"
                               "-弹出面板、hover/选中高亮\n"
                               "-跟随主题与 Accent 联动"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    PICKERS_CAL_TIME(X)
#undef X
                },
                false));

#undef PICKERS_CAL_TIME
        }

        // ComboBox (Accent linkage)
        {
            QString code;
#define PICKERS_COMBO(X) \
    X(auto *comboRow = new QHBoxLayout(); ) \
    X(comboRow->setContentsMargins(0, 0, 0, 0); ) \
    X(comboRow->setSpacing(10); ) \
    X(auto *combo = new FluentComboBox(); ) \
    X(combo->addItems({QStringLiteral("Accent: 蓝"), QStringLiteral("Accent: 绿"), QStringLiteral("Accent: 紫")}); ) \
    X(auto *comboHint = new FluentLabel(QStringLiteral("选择项会改变 Accent（演示 ThemeManager → 全控件联动）")); ) \
    X(comboHint->setStyleSheet("font-size: 12px; opacity: 0.85;" ); ) \
    X(comboRow->addWidget(combo); ) \
    X(comboRow->addWidget(comboHint); ) \
    X(comboRow->addStretch(1); ) \
    X(body->addLayout(comboRow); ) \
    X(QObject::connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), window, [](int idx) { if (idx == 0) Demo::applyAccent(QColor("#0067C0")); if (idx == 1) Demo::applyAccent(QColor("#0F7B0F")); if (idx == 2) Demo::applyAccent(QColor("#6B4EFF")); }); )

#define X(line) code += QStringLiteral(#line "\n");
            PICKERS_COMBO(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentComboBox"),
                QStringLiteral("下拉选择（示例：联动 Accent）"),
                QStringLiteral("要点：\n"
                               "-addItems() 快速填充\n"
                               "-currentIndexChanged 驱动联动逻辑"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    PICKERS_COMBO(X)
#undef X
                }));

#undef PICKERS_COMBO
        }

        // ColorPicker + ColorDialog
        {
            QString code;
#define PICKERS_COLOR(X) \
    X(auto *colorRow = new QHBoxLayout(); ) \
    X(colorRow->setContentsMargins(0, 0, 0, 0); ) \
    X(colorRow->setSpacing(10); ) \
    X(auto *picker = new FluentColorPicker(); ) \
    X(picker->setColor(ThemeManager::instance().colors().accent); ) \
    X(auto *openDialog = new FluentButton(QStringLiteral("打开 FluentColorDialog…")); ) \
    X(openDialog->setPrimary(true); ) \
    X(colorRow->addWidget(picker); ) \
    X(colorRow->addWidget(openDialog); ) \
    X(colorRow->addStretch(1); ) \
    X(body->addLayout(colorRow); ) \
    X(QObject::connect(picker, &FluentColorPicker::colorChanged, window, [](const QColor &c) { if (c.isValid()) Demo::applyAccent(c); }); ) \
    X(QObject::connect(openDialog, &QPushButton::clicked, window, [window]() { const QColor before = ThemeManager::instance().colors().accent; FluentColorDialog dlg(before, window); QObject::connect(&dlg, &FluentColorDialog::colorChanged, window, [](const QColor &c) { if (c.isValid()) Demo::applyAccent(c); }); if (dlg.exec() == QDialog::Accepted) { const QColor selected = dlg.selectedColor(); if (selected.isValid()) Demo::applyAccent(selected); } else { Demo::applyAccent(before); } }); )

#define X(line) code += QStringLiteral(#line "\n");
            PICKERS_COLOR(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("ColorPicker / ColorDialog"),
                QStringLiteral("颜色选择（实时预览 + 取消回滚）"),
                QStringLiteral("要点：\n"
                               "-ColorPicker：轻量内嵌控件\n"
                               "-ColorDialog：HSV/Alpha/吸管，保持 exec()+selectedColor 兼容\n"
                               "-示例里：colorChanged 实时预览，Rejected 回滚"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    PICKERS_COLOR(X)
#undef X
                },
                false,
                230));

#undef PICKERS_COLOR
        }
    });
}

} // namespace Demo::Pages
