#include "PagePickers.h"

#include "../DemoHelpers.h"

#include <QDate>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "Fluent/FluentButton.h"
#include "Fluent/FluentCalendarPicker.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentColorDialog.h"
#include "Fluent/FluentColorPicker.h"
#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentDateRangePicker.h"
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

        // DateRangePicker
        {
            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentDateRangePicker"),
                QStringLiteral("日期范围选择器（双面板，连续 Accent 高亮带）"),
                QStringLiteral("要点：\n"
                               "-点击控件弹出双月份面板\n"
                               "-第一次点击选开始日，第二次选结束日\n"
                               "-左右面板各自独立翻月；鼠标滚轮也可翻页\n"
                               "-ESC 取消当前选择，再按关闭弹窗"),
                QStringLiteral(
                    "auto *rangePicker = new FluentDateRangePicker();\n"
                    "rangePicker->setFixedWidth(400);\n"
                    "rangePicker->setDateRange(QDate::currentDate(),\n"
                    "                          QDate::currentDate().addDays(13));\n"
                    "body->addWidget(rangePicker);\n"
                    "QObject::connect(rangePicker, &FluentDateRangePicker::dateRangeChanged,\n"
                    "    label, [label](const QDate &s, const QDate &e) {\n"
                    "        label->setText(s.toString(\"yyyy-MM-dd\") + \" ~ \" + e.toString(\"yyyy-MM-dd\"));\n"
                    "    });"),
                [=](QVBoxLayout *body) {
                    auto *row = new QHBoxLayout();
                    row->setContentsMargins(0, 0, 0, 0);
                    row->setSpacing(12);

                    auto *rangePicker = new FluentDateRangePicker();
                    rangePicker->setFixedWidth(400);
                    rangePicker->setDateRange(QDate::currentDate(),
                                              QDate::currentDate().addDays(13));

                    auto *label = new FluentLabel();
                    label->setStyleSheet("font-size: 12px; opacity: 0.85;");

                    auto updateLabel = [label](const QDate &s, const QDate &e) {
                        if (s.isValid() && e.isValid()) {
                            label->setText(QStringLiteral("已选：") +
                                           s.toString("yyyy-MM-dd") +
                                           QStringLiteral("  ~  ") +
                                           e.toString("yyyy-MM-dd"));
                        } else {
                            label->setText(QStringLiteral("未选择"));
                        }
                    };
                    updateLabel(rangePicker->startDate(), rangePicker->endDate());

                    QObject::connect(rangePicker, &FluentDateRangePicker::dateRangeChanged,
                                     label, updateLabel);

                    row->addWidget(rangePicker);
                    row->addWidget(label);
                    row->addStretch(1);
                    body->addLayout(row);
                },
                false));

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentDateRangePicker（自定义前后缀 / 分隔符）"),
                QStringLiteral("prefix / suffix / separator 配置展示"),
                QStringLiteral("要点：\n"
                               "-开始/结束两侧都支持 prefix 与 suffix\n"
                               "-中间分隔文本可改成业务语义，例如“至”“→”“~”\n"
                               "-适合做入住/离店、起止时间、账期范围等场景"),
                QStringLiteral(
                    "auto *rangePicker = new FluentDateRangePicker();\n"
                    "rangePicker->setFixedWidth(440);\n"
                    "rangePicker->setStartPrefix(QStringLiteral(\"入住：\"));\n"
                    "rangePicker->setStartSuffix(QStringLiteral(\" 起\"));\n"
                    "rangePicker->setSeparator(QStringLiteral(\"  至  \"));\n"
                    "rangePicker->setEndPrefix(QStringLiteral(\"离店：\"));\n"
                    "rangePicker->setEndSuffix(QStringLiteral(\" 止\"));\n"
                    "rangePicker->setDateRange(QDate::currentDate().addDays(2),\n"
                    "                          QDate::currentDate().addDays(5));\n"),
                [=](QVBoxLayout *body) {
                    auto *row = new QHBoxLayout();
                    row->setContentsMargins(0, 0, 0, 0);
                    row->setSpacing(12);

                    auto *rangePicker = new FluentDateRangePicker();
                    rangePicker->setFixedWidth(440);
                    rangePicker->setStartPrefix(QStringLiteral("入住："));
                    rangePicker->setStartSuffix(QStringLiteral(" 起"));
                    rangePicker->setSeparator(QStringLiteral("  至  "));
                    rangePicker->setEndPrefix(QStringLiteral("离店："));
                    rangePicker->setEndSuffix(QStringLiteral(" 止"));
                    rangePicker->setDateRange(QDate::currentDate().addDays(2),
                                              QDate::currentDate().addDays(5));

                    auto *label = new FluentLabel();
                    label->setStyleSheet("font-size: 12px; opacity: 0.85;");

                    auto updateLabel = [label](const QDate &s, const QDate &e) {
                        if (s.isValid() && e.isValid()) {
                            label->setText(QStringLiteral("酒店场景：")
                                           + s.toString("MM-dd")
                                           + QStringLiteral(" 入住，")
                                           + e.toString("MM-dd")
                                           + QStringLiteral(" 离店"));
                        } else {
                            label->setText(QStringLiteral("未选择入住/离店日期"));
                        }
                    };
                    updateLabel(rangePicker->startDate(), rangePicker->endDate());

                    QObject::connect(rangePicker, &FluentDateRangePicker::dateRangeChanged,
                                     label, updateLabel);

                    row->addWidget(rangePicker);
                    row->addWidget(label);
                    row->addStretch(1);
                    body->addLayout(row);
                },
                true));
        }
    });
}

} // namespace Demo::Pages
