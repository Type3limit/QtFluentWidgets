#include "PageBasicInput.h"

#include "../DemoHelpers.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

#include "Fluent/FluentButton.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentCheckBox.h"
#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentLineEdit.h"
#include "Fluent/FluentMainWindow.h"
#include "Fluent/FluentRadioButton.h"
#include "Fluent/FluentScrollArea.h"
#include "Fluent/FluentSlider.h"
#include "Fluent/FluentSpinBox.h"
#include "Fluent/FluentToggleSwitch.h"
#include "Fluent/FluentToolButton.h"

namespace Demo::Pages {

using namespace Fluent;

QWidget *createBasicInputPage(FluentMainWindow *window, const std::function<void(int)> &jumpTo)
{
    return Demo::makePage([&](QVBoxLayout *page) {
        auto hero = Demo::makeSection(
            DEMO_TEXT("基本输入", "Basic Input"),
            DEMO_TEXT("NavigationView 父节点整合页。点击父节点进入这里，点击右侧箭头展开并查看子页。",
                      "NavigationView parent hub page. Click the parent item to enter here, or use the chevron to expand and inspect child pages."));
        page->addWidget(hero.card);

        auto *summary = new FluentLabel(DEMO_TEXT(
            "这个分组把最常用的输入与交互控件放在一起：文本输入、数值输入、组合选择，以及按钮、工具按钮、复选、单选和开关。",
            "This group brings together the most common input and interaction controls: text entry, numeric input, combo selection, plus buttons, tool buttons, check boxes, radio buttons, and toggles."));
        summary->setWordWrap(true);
        summary->setStyleSheet("font-size: 12px; opacity: 0.88;");
        hero.body->addWidget(summary);

        auto *jumpRow = new QHBoxLayout();
        jumpRow->setContentsMargins(0, 0, 0, 0);
        jumpRow->setSpacing(10);

        auto *inputsBtn = new FluentButton(DEMO_TEXT("查看输入详细页", "Open inputs page"));
        auto *buttonsBtn = new FluentButton(DEMO_TEXT("查看按钮详细页", "Open buttons page"));
        QObject::connect(inputsBtn, &QPushButton::clicked, window, [jumpTo]() { jumpTo(2); });
        QObject::connect(buttonsBtn, &QPushButton::clicked, window, [jumpTo]() { jumpTo(3); });

        jumpRow->addWidget(inputsBtn);
        jumpRow->addWidget(buttonsBtn);
        jumpRow->addStretch(1);
        hero.body->addLayout(jumpRow);

        page->addWidget(Demo::makeCollapsedExample(
            DEMO_TEXT("输入控件概览", "Input overview"),
            DEMO_TEXT("LineEdit / ComboBox / Slider / SpinBox 的组合预览", "Combined preview of LineEdit / ComboBox / Slider / SpinBox"),
            DEMO_TEXT("要点：\n-这个整合页负责总览和分组入口\n-详细行为仍在子页中完整展示\n-父节点和子节点现在都可以独立导航",
                      "Highlights:\n-This hub page provides the overview and group entry point\n-Detailed behavior is still fully demonstrated in the child pages\n-Parent and child navigation items can now be used independently"),
            QString(),
            [window, jumpTo](QVBoxLayout *body) {
                auto *lineRow = new QHBoxLayout();
                lineRow->setContentsMargins(0, 0, 0, 0);
                lineRow->setSpacing(10);

                auto *nameEdit = new FluentLineEdit();
                nameEdit->setPlaceholderText(DEMO_TEXT("搜索或输入内容", "Search or type"));
                auto *disabledEdit = new FluentLineEdit();
                disabledEdit->setPlaceholderText(DEMO_TEXT("禁用态示例", "Disabled example"));
                disabledEdit->setDisabled(true);
                lineRow->addWidget(nameEdit, 1);
                lineRow->addWidget(disabledEdit, 1);
                body->addLayout(lineRow);

                auto *valueRow = new QHBoxLayout();
                valueRow->setContentsMargins(0, 0, 0, 0);
                valueRow->setSpacing(10);

                auto *combo = new FluentComboBox();
                combo->addItems({DEMO_TEXT("选项 A", "Option A"), DEMO_TEXT("选项 B", "Option B"), DEMO_TEXT("选项 C", "Option C")});
                combo->setCurrentIndex(1);

                auto *spin = new FluentSpinBox();
                spin->setRange(0, 100);
                spin->setValue(24);

                valueRow->addWidget(combo);
                valueRow->addWidget(spin);
                valueRow->addStretch(1);
                body->addLayout(valueRow);

                auto *sliderRow = new QHBoxLayout();
                sliderRow->setContentsMargins(0, 0, 0, 0);
                sliderRow->setSpacing(10);

                auto *slider = new FluentSlider(Qt::Horizontal);
                slider->setRange(0, 100);
                slider->setValue(35);
                auto *valueLabel = new FluentLabel(QStringLiteral("35"));
                valueLabel->setMinimumWidth(28);
                QObject::connect(slider, &QSlider::valueChanged, valueLabel, [valueLabel](int value) {
                    valueLabel->setText(QString::number(value));
                });

                sliderRow->addWidget(slider, 1);
                sliderRow->addWidget(valueLabel);
                body->addLayout(sliderRow);

                auto *jumpBtn = new FluentButton(DEMO_TEXT("进入输入详细页 →", "Open inputs page ->"));
                QObject::connect(jumpBtn, &QPushButton::clicked, window, [jumpTo]() { jumpTo(2); });
                body->addWidget(jumpBtn);
            },
            false));

        page->addWidget(Demo::makeCollapsedExample(
            DEMO_TEXT("按钮 / 开关概览", "Buttons / toggles overview"),
            DEMO_TEXT("Primary Button / ToolButton / Toggle / Check / Radio 的组合预览", "Combined preview of Primary Button / ToolButton / Toggle / Check / Radio"),
            DEMO_TEXT("要点：\n-父节点现在可直接选中跳到整合页\n-展开箭头只负责子项显隐\n-详细状态展示依然保留在子页",
                      "Highlights:\n-The parent item can now be selected directly to open this hub page\n-The chevron is only responsible for expanding or collapsing child items\n-Detailed state demonstrations remain in the child pages"),
            QString(),
            [window, jumpTo](QVBoxLayout *body) {
                auto *buttonRow = new QHBoxLayout();
                buttonRow->setContentsMargins(0, 0, 0, 0);
                buttonRow->setSpacing(10);

                auto *primary = new FluentButton(QStringLiteral("Primary"));
                primary->setPrimary(true);
                auto *secondary = new FluentButton(QStringLiteral("Secondary"));
                auto *tool = new FluentToolButton(QStringLiteral("Tool"));
                buttonRow->addWidget(primary);
                buttonRow->addWidget(secondary);
                buttonRow->addWidget(tool);
                buttonRow->addStretch(1);
                body->addLayout(buttonRow);

                auto *optionRow = new QHBoxLayout();
                optionRow->setContentsMargins(0, 0, 0, 0);
                optionRow->setSpacing(14);

                auto *toggle = new FluentToggleSwitch(DEMO_TEXT("开关", "Toggle"));
                toggle->setChecked(true);
                auto *check = new FluentCheckBox(DEMO_TEXT("复选", "Check"));
                check->setChecked(true);
                auto *radio = new FluentRadioButton(DEMO_TEXT("单选", "Radio"));
                radio->setChecked(true);

                optionRow->addWidget(toggle);
                optionRow->addWidget(check);
                optionRow->addWidget(radio);
                optionRow->addStretch(1);
                body->addLayout(optionRow);

                auto *jumpBtn = new FluentButton(DEMO_TEXT("进入按钮详细页 →", "Open buttons page ->"));
                QObject::connect(jumpBtn, &QPushButton::clicked, window, [jumpTo]() { jumpTo(3); });
                body->addWidget(jumpBtn);
            },
            false));
    });
}

} // namespace Demo::Pages