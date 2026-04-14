#include "PagePickers.h"

#include "../DemoHelpers.h"

#include <QDate>
#include <QHBoxLayout>
#include <QLabel>
#include <QTime>
#include <QVBoxLayout>

#include "Fluent/FluentButton.h"
#include "Fluent/FluentCalendarPicker.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentColorDialog.h"
#include "Fluent/FluentColorPicker.h"
#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentDatePicker.h"
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
        auto s = Demo::makeSection(DEMO_TEXT("选择器", "Pickers"),
                                   DEMO_TEXT("DatePicker / Calendar / Time / ComboBox（联动 Accent）", "DatePicker / Calendar / Time / ComboBox with Accent linkage"));

        page->addWidget(s.card);

        // DatePicker
        {
            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentDatePicker"),
                DEMO_TEXT("滚轮式日期选择器", "Wheel-style date picker"),
                DEMO_TEXT("要点：\n"
                          "-月 / 日 / 年列可滚动并吸附到中心选择位\n"
                          "-底部提供确认 / 取消操作，交互更接近 WinUI Gallery\n"
                          "-默认语言占位，可分别自定义月 / 日 / 年文案",
                          "Highlights:\n"
                          "-Month / day / year columns scroll and snap to the center selection slot\n"
                          "-The bottom confirm / cancel actions feel closer to WinUI Gallery\n"
                          "-Default placeholders follow the active language and can be customized per field"),
                DEMO_TEXT(
                    "auto *simpleDate = new FluentDatePicker();\n"
                    "simpleDate->setFixedWidth(320);\n"
                    "\n"
                    "auto *customTextDate = new FluentDatePicker();\n"
                    "customTextDate->setFixedWidth(240);\n"
                    "customTextDate->setVisibleParts(FluentDatePicker::MonthPart | FluentDatePicker::DayPart);\n"
                    "customTextDate->setMonthPlaceholderText(DEMO_TEXT(\"月份\", \"Month\"));\n"
                    "customTextDate->setDayPlaceholderText(DEMO_TEXT(\"日期\", \"Day\"));\n"
                    "customTextDate->setDayDisplayFormat(QStringLiteral(\"d (ddd)\"));\n",
                    "auto *simpleDate = new FluentDatePicker();\n"
                    "simpleDate->setFixedWidth(320);\n"
                    "\n"
                    "auto *customTextDate = new FluentDatePicker();\n"
                    "customTextDate->setFixedWidth(240);\n"
                    "customTextDate->setVisibleParts(FluentDatePicker::MonthPart | FluentDatePicker::DayPart);\n"
                    "customTextDate->setMonthPlaceholderText(QStringLiteral(\"Month\"));\n"
                    "customTextDate->setDayPlaceholderText(QStringLiteral(\"Day\"));\n"
                    "customTextDate->setDayDisplayFormat(QStringLiteral(\"d (ddd)\"));\n"),
                [=](QVBoxLayout *body) {
                    auto addLabeledRow = [body](const QString &labelText, QWidget *control) {
                        auto *label = new FluentLabel(labelText);
                        label->setStyleSheet("font-size: 12px; font-weight: 600;");
                        body->addWidget(label);

                        auto *row = new QHBoxLayout();
                        row->setContentsMargins(0, 0, 0, 0);
                        row->setSpacing(12);
                        row->addWidget(control);
                        row->addStretch(1);
                        body->addLayout(row);
                    };

                    auto *simpleDate = new FluentDatePicker();
                    simpleDate->setFixedWidth(320);

                    auto *customTextDate = new FluentDatePicker();
                    customTextDate->setFixedWidth(240);
                    customTextDate->setVisibleParts(FluentDatePicker::MonthPart | FluentDatePicker::DayPart);
                    customTextDate->setMonthPlaceholderText(DEMO_TEXT("月份", "Month"));
                    customTextDate->setDayPlaceholderText(DEMO_TEXT("日期", "Day"));
                    customTextDate->setDayDisplayFormat(QStringLiteral("d (ddd)"));

                    addLabeledRow(DEMO_TEXT("默认语言", "Default language"), simpleDate);
                    addLabeledRow(DEMO_TEXT("自定义占位文案", "Custom placeholder text"), customTextDate);
                },
                false,
                220));
        }

        // CalendarPicker
        {
            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentCalendarPicker"),
                DEMO_TEXT("日历式日期选择器", "Calendar-style date picker"),
                DEMO_TEXT("要点：\n"
                          "-保留月份网格弹出层，适合需要完整月历视图的场景\n"
                          "-星期与月份会跟随当前 locale，可通过 setLocale 进一步覆盖\n"
                          "-Today 按钮文案可自定义",
                          "Highlights:\n"
                          "-Keeps the month-grid popup for scenarios that need a full calendar view\n"
                          "-Weekday and month text follow the current locale and can still be overridden via setLocale\n"
                          "-The Today button text is customizable"),
                DEMO_TEXT(
                    "auto *calendar = new FluentCalendarPicker();\n"
                    "calendar->setFixedWidth(320);\n"
                    "calendar->setTodayText(DEMO_TEXT(\"回到今天\", \"Back to today\"));\n"
                    "calendar->setDate(QDate::currentDate());\n",
                    "auto *calendar = new FluentCalendarPicker();\n"
                    "calendar->setFixedWidth(320);\n"
                    "calendar->setTodayText(QStringLiteral(\"Back to today\"));\n"
                    "calendar->setDate(QDate::currentDate());\n"),
                [=](QVBoxLayout *body) {
                    auto *row = new QHBoxLayout();
                    row->setContentsMargins(0, 0, 0, 0);
                    row->setSpacing(12);

                    auto *calendar = new FluentCalendarPicker();
                    calendar->setFixedWidth(320);
                    calendar->setTodayText(DEMO_TEXT("回到今天", "Back to today"));
                    calendar->setDate(QDate::currentDate());

                    row->addWidget(calendar);
                    row->addStretch(1);
                    body->addLayout(row);
                },
                true));
        }

        // TimePicker
        {
            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentTimePicker"),
                DEMO_TEXT("滚轮式时间选择器", "Wheel-style time picker"),
                DEMO_TEXT("要点：\n"
                          "-小时 / 分钟 / 上午下午 列可滚动选择\n"
                          "-支持 minuteIncrement 与 24 小时制\n"
                          "-默认语言占位，可自定义小时 / 分钟 / 上午下午文案",
                          "Highlights:\n"
                          "-Hour / minute / AM-PM columns can be scrolled directly\n"
                          "-Supports minuteIncrement and 24-hour mode\n"
                          "-Default placeholders follow the active language and each label can be customized"),
                DEMO_TEXT(
                    "auto *simpleTime = new FluentTimePicker();\n"
                    "simpleTime->setFixedWidth(200);\n"
                    "\n"
                    "auto *customTextTime = new FluentTimePicker();\n"
                    "customTextTime->setFixedWidth(200);\n"
                    "customTextTime->setHourPlaceholderText(DEMO_TEXT(\"小时\", \"Hour\"));\n"
                    "customTextTime->setMinutePlaceholderText(DEMO_TEXT(\"分钟\", \"Minute\"));\n"
                    "customTextTime->setAnteMeridiemText(DEMO_TEXT(\"上午\", \"AM\"));\n"
                    "customTextTime->setPostMeridiemText(DEMO_TEXT(\"下午\", \"PM\"));\n"
                    "\n"
                    "auto *time24 = new FluentTimePicker();\n"
                    "time24->setFixedWidth(180);\n"
                    "time24->setUse24HourClock(true);\n"
                    "time24->setMinuteIncrement(5);\n"
                    "time24->setTime(QTime(16, 8));\n",
                    "auto *simpleTime = new FluentTimePicker();\n"
                    "simpleTime->setFixedWidth(200);\n"
                    "\n"
                    "auto *customTextTime = new FluentTimePicker();\n"
                    "customTextTime->setFixedWidth(200);\n"
                    "customTextTime->setHourPlaceholderText(QStringLiteral(\"Hour\"));\n"
                    "customTextTime->setMinutePlaceholderText(QStringLiteral(\"Minute\"));\n"
                    "customTextTime->setAnteMeridiemText(QStringLiteral(\"AM\"));\n"
                    "customTextTime->setPostMeridiemText(QStringLiteral(\"PM\"));\n"
                    "\n"
                    "auto *time24 = new FluentTimePicker();\n"
                    "time24->setFixedWidth(180);\n"
                    "time24->setUse24HourClock(true);\n"
                    "time24->setMinuteIncrement(5);\n"
                    "time24->setTime(QTime(16, 8));\n"),
                [=](QVBoxLayout *body) {
                    auto addLabeledRow = [body](const QString &labelText, QWidget *control) {
                        auto *label = new FluentLabel(labelText);
                        label->setStyleSheet("font-size: 12px; font-weight: 600;");
                        body->addWidget(label);

                        auto *row = new QHBoxLayout();
                        row->setContentsMargins(0, 0, 0, 0);
                        row->setSpacing(12);
                        row->addWidget(control);
                        row->addStretch(1);
                        body->addLayout(row);
                    };

                    auto *simpleTime = new FluentTimePicker();
                    simpleTime->setFixedWidth(200);

                    auto *customTextTime = new FluentTimePicker();
                    customTextTime->setFixedWidth(200);
                    customTextTime->setHourPlaceholderText(DEMO_TEXT("小时", "Hour"));
                    customTextTime->setMinutePlaceholderText(DEMO_TEXT("分钟", "Minute"));
                    customTextTime->setAnteMeridiemText(DEMO_TEXT("上午", "AM"));
                    customTextTime->setPostMeridiemText(DEMO_TEXT("下午", "PM"));

                    auto *time24 = new FluentTimePicker();
                    time24->setFixedWidth(180);
                    time24->setUse24HourClock(true);
                    time24->setMinuteIncrement(5);
                    time24->setTime(QTime(16, 8));

                    addLabeledRow(DEMO_TEXT("默认语言", "Default language"), simpleTime);
                    addLabeledRow(DEMO_TEXT("自定义占位文案", "Custom placeholder text"), customTextTime);
                    addLabeledRow(DEMO_TEXT("24 小时制", "24-hour clock"), time24);
                },
                false,
                260));
        }

        // ComboBox (Accent linkage)
        {
            QString code;
#define PICKERS_COMBO(X) \
    X(auto *comboRow = new QHBoxLayout(); ) \
    X(comboRow->setContentsMargins(0, 0, 0, 0); ) \
    X(comboRow->setSpacing(10); ) \
    X(auto *combo = new FluentComboBox(); ) \
    X(combo->addItems({DEMO_TEXT("Accent: 蓝", "Accent: Blue"), DEMO_TEXT("Accent: 绿", "Accent: Green"), DEMO_TEXT("Accent: 紫", "Accent: Purple")}); ) \
    X(auto *comboHint = new FluentLabel(DEMO_TEXT("选择项会改变 Accent（演示 ThemeManager → 全控件联动）", "Selecting an item changes Accent to demonstrate ThemeManager-wide control linkage")); ) \
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
                DEMO_TEXT("下拉选择（示例：联动 Accent）", "Drop-down selection with Accent linkage"),
                DEMO_TEXT("要点：\n"
                          "-addItems() 快速填充\n"
                          "-currentIndexChanged 驱动联动逻辑",
                          "Highlights:\n"
                          "-Use addItems() for quick population\n"
                          "-Use currentIndexChanged to drive linked behavior"),
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
    X(auto *openDialog = new FluentButton(DEMO_TEXT("打开 FluentColorDialog…", "Open FluentColorDialog...")); ) \
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
                DEMO_TEXT("颜色选择（实时预览 + 取消回滚）", "Color selection with live preview and rollback on cancel"),
                DEMO_TEXT("要点：\n"
                          "-ColorPicker：轻量内嵌控件\n"
                          "-ColorDialog：HSV/Alpha/吸管，保持 exec()+selectedColor 兼容\n"
                          "-示例里：colorChanged 实时预览，Rejected 回滚",
                          "Highlights:\n"
                          "-ColorPicker: a lightweight embedded control\n"
                          "-ColorDialog: HSV, alpha, eyedropper, and exec()+selectedColor compatibility\n"
                          "-The example uses colorChanged for live preview and rolls back on reject"),
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
                DEMO_TEXT("日期范围选择器（双面板，连续 Accent 高亮带）", "Date-range picker with dual panels and a continuous Accent highlight band"),
                DEMO_TEXT("要点：\n"
                          "-点击控件弹出双月份面板\n"
                          "-第一次点击选开始日，第二次选结束日\n"
                          "-左右面板各自独立翻月；鼠标滚轮也可翻页\n"
                          "-ESC 取消当前选择，再按关闭弹窗",
                          "Highlights:\n"
                          "-Click the control to open a dual-month popup\n"
                          "-First click selects the start date, second click selects the end date\n"
                          "-The left and right panels flip months independently; the mouse wheel also pages\n"
                          "-Press ESC to cancel the current selection, then press it again to close the popup"),
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
                            label->setText(DEMO_TEXT("已选：", "Selected: ") +
                                           s.toString("yyyy-MM-dd") +
                                           QStringLiteral("  ~  ") +
                                           e.toString("yyyy-MM-dd"));
                        } else {
                            label->setText(DEMO_TEXT("未选择", "Not selected"));
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
                DEMO_TEXT("prefix / suffix / separator 配置展示", "prefix / suffix / separator configuration demo"),
                DEMO_TEXT("要点：\n"
                          "-开始/结束两侧都支持 prefix 与 suffix\n"
                          "-中间分隔文本可改成业务语义，例如“至”“→”“~”\n"
                          "-适合做入住/离店、起止时间、账期范围等场景",
                          "Highlights:\n"
                          "-Both sides support prefix and suffix text\n"
                          "-The separator can be changed to business-specific wording such as 'to', '->', or '~'\n"
                          "-Useful for hotel stays, time spans, or billing periods"),
                DEMO_TEXT(
                    "auto *rangePicker = new FluentDateRangePicker();\n"
                    "rangePicker->setFixedWidth(440);\n"
                    "rangePicker->setStartPrefix(DEMO_TEXT(\"入住：\", \"Check-in: \"));\n"
                    "rangePicker->setStartSuffix(DEMO_TEXT(\" 起\", \"\"));\n"
                    "rangePicker->setSeparator(DEMO_TEXT(\"  至  \", \"  to  \"));\n"
                    "rangePicker->setEndPrefix(DEMO_TEXT(\"离店：\", \"Check-out: \"));\n"
                    "rangePicker->setEndSuffix(DEMO_TEXT(\" 止\", \"\"));\n"
                    "rangePicker->setDateRange(QDate::currentDate().addDays(2),\n"
                    "                          QDate::currentDate().addDays(5));\n",
                    "auto *rangePicker = new FluentDateRangePicker();\n"
                    "rangePicker->setFixedWidth(440);\n"
                    "rangePicker->setStartPrefix(QStringLiteral(\"Check-in: \"));\n"
                    "rangePicker->setStartSuffix(QStringLiteral(\"\"));\n"
                    "rangePicker->setSeparator(QStringLiteral(\"  to  \"));\n"
                    "rangePicker->setEndPrefix(QStringLiteral(\"Check-out: \"));\n"
                    "rangePicker->setEndSuffix(QStringLiteral(\"\"));\n"
                    "rangePicker->setDateRange(QDate::currentDate().addDays(2),\n"
                    "                          QDate::currentDate().addDays(5));\n"),
                [=](QVBoxLayout *body) {
                    auto *row = new QHBoxLayout();
                    row->setContentsMargins(0, 0, 0, 0);
                    row->setSpacing(12);

                    auto *rangePicker = new FluentDateRangePicker();
                    rangePicker->setFixedWidth(440);
                    rangePicker->setStartPrefix(DEMO_TEXT("入住：", "Check-in: "));
                    rangePicker->setStartSuffix(DEMO_TEXT(" 起", ""));
                    rangePicker->setSeparator(DEMO_TEXT("  至  ", "  to  "));
                    rangePicker->setEndPrefix(DEMO_TEXT("离店：", "Check-out: "));
                    rangePicker->setEndSuffix(DEMO_TEXT(" 止", ""));
                    rangePicker->setDateRange(QDate::currentDate().addDays(2),
                                              QDate::currentDate().addDays(5));

                    auto *label = new FluentLabel();
                    label->setStyleSheet("font-size: 12px; opacity: 0.85;");

                    auto updateLabel = [label](const QDate &s, const QDate &e) {
                        if (s.isValid() && e.isValid()) {
                            label->setText(DEMO_TEXT("酒店场景：", "Hotel stay: ")
                                           + s.toString("MM-dd")
                                           + DEMO_TEXT(" 入住，", " check in, ")
                                           + e.toString("MM-dd")
                                           + DEMO_TEXT(" 离店", " check out"));
                        } else {
                            label->setText(DEMO_TEXT("未选择入住/离店日期", "No check-in/check-out dates selected"));
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
