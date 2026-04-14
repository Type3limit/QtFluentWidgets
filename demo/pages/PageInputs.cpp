#include "PageInputs.h"

#include "../DemoHelpers.h"
#include "../DemoCodeEditorSettings.h"
#include "Fluent/FluentCard.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentLineEdit.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentMainWindow.h"
#include "Fluent/FluentScrollArea.h"
#include "Fluent/FluentSlider.h"
#include "Fluent/FluentSpinBox.h"
#include "Fluent/FluentCodeEditor.h"
#include "Fluent/FluentTextEdit.h"

namespace Demo::Pages {

using namespace Fluent;

QWidget *createInputsPage(FluentMainWindow *window)
{
    return Demo::makePage([&](QVBoxLayout *page) {
        auto s = Demo::makeSection(DEMO_TEXT("输入控件", "Inputs"),
                                   DEMO_TEXT("LineEdit / TextEdit / SpinBox（含禁用与占位符展示）", "LineEdit / TextEdit / SpinBox with disabled and placeholder states"));

        page->addWidget(s.card);

        // LineEdit
        {
            QString code;
#define INPUTS_LINEEDIT(X) \
    X(auto *row = new QHBoxLayout();) \
    X(row->setContentsMargins(0, 0, 0, 0);) \
    X(row->setSpacing(10);) \
    X(auto *le1 = new FluentLineEdit();) \
    X(le1->setPlaceholderText(DEMO_TEXT("普通 LineEdit", "Normal LineEdit"));) \
    X(auto *le2 = new FluentLineEdit();) \
    X(le2->setPlaceholderText(DEMO_TEXT("禁用 LineEdit", "Disabled LineEdit"));) \
    X(le2->setDisabled(true);) \
    X(row->addWidget(le1);) \
    X(row->addWidget(le2);) \
    X(body->addLayout(row);)

#define X(line) code += QStringLiteral(#line "\n");
            INPUTS_LINEEDIT(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentLineEdit"),
                DEMO_TEXT("占位符、禁用态、跟随主题", "Placeholder, disabled state, theme-aware styling"),
                DEMO_TEXT("要点：\n"
                          "-setPlaceholderText() 设置占位符\n"
                          "-setDisabled(true) 展示禁用态\n"
                          "-与 ThemeManager 联动更新样式",
                          "Highlights:\n"
                          "-Use setPlaceholderText() to set the placeholder\n"
                          "-Use setDisabled(true) to show the disabled state\n"
                          "-Styles update together with ThemeManager"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    INPUTS_LINEEDIT(X)
#undef X
                },
                false));

#undef INPUTS_LINEEDIT
        }

        // TextEdit
        {
            const QString textEditPlaceholder = DEMO_TEXT("TextEdit：滚动条是自绘 FluentScrollBar（圆角 pill）",
                                                         "TextEdit: the scrollbar is a custom FluentScrollBar with a rounded pill handle");
            const QString textEditContent = DEMO_TEXT("1. FluentTextEdit\n"
                                                     "2. Scrollbar pill handle\n"
                                                     "3. Hover/Wheel 显示\n\n"
                                                     "（多添加几行测试滚动…）\n\n"
                                                     "4\n5\n6\n7\n8\n9\n10\n11\n12\n",
                                                     "1. FluentTextEdit\n"
                                                     "2. Scrollbar pill handle\n"
                                                     "3. Visible on hover / wheel\n\n"
                                                     "(Add a few more lines to test scrolling...)\n\n"
                                                     "4\n5\n6\n7\n8\n9\n10\n11\n12\n");
            QString code;
#define INPUTS_TEXTEDIT(X) \
    X(auto *te = new FluentTextEdit();) \
    X(te->setPlaceholderText(textEditPlaceholder);) \
    X(te->setText(textEditContent);) \
    X(te->setFixedHeight(180);) \
    X(body->addWidget(te);)

#define X(line) code += QStringLiteral(#line "\n");
            INPUTS_TEXTEDIT(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentTextEdit"),
                DEMO_TEXT("内置 FluentScrollBar（自绘 pill）", "Built-in FluentScrollBar with a custom pill handle"),
                DEMO_TEXT("要点：\n"
                          "-setPlaceholderText()\n"
                          "-setReadOnly(true) 可作为日志/代码展示区\n"
                          "-滚动条随主题与 Accent 联动",
                          "Highlights:\n"
                          "-Use setPlaceholderText() for placeholder text\n"
                          "-Use setReadOnly(true) for log or code viewers\n"
                          "-The scrollbar follows Theme and Accent changes"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    INPUTS_TEXTEDIT(X)
#undef X
                },
                true,
                220));

#undef INPUTS_TEXTEDIT
        }

        // CodeEditor
        {
            QString code;
#define INPUTS_CODEEDITOR(X) \
    X(auto *ed = new FluentCodeEditor();) \
    X(ed->setFixedHeight(220);) \
    X(ed->setPlainText(QStringLiteral("#include <vector>\n\n" \
                                        "struct Foo { int x=1; void bar(){ if(x){x++;} } };\n\n" \
                                        "int main(){ std::vector<int> v{1,2,3}; Foo f; f.bar(); return v.size(); }\n"));) \
    X(Demo::DemoCodeEditorSettings::instance().attach(ed, false);) \
    X(body->addWidget(ed);)

#define X(line) code += QStringLiteral(#line "\n");
            INPUTS_CODEEDITOR(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentCodeEditor"),
                DEMO_TEXT("C++ 语法高亮 + clang-format（可在侧边栏配置路径）", "C++ syntax highlighting + clang-format (configured in the sidebar)"),
                DEMO_TEXT("要点：\n"
                          "-侧边栏 CodeEditor 面板可配置 clang-format 路径\n"
                          "-Ctrl+Shift+F 手动格式化\n"
                          "-可开启 debounce 自动格式化（同样在侧边栏配置）",
                          "Highlights:\n"
                          "-Configure the clang-format path from the sidebar CodeEditor panel\n"
                          "-Press Ctrl+Shift+F to format manually\n"
                          "-Debounced auto-formatting can also be enabled from the sidebar"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    INPUTS_CODEEDITOR(X)
#undef X
                },
                true,
                260));

#undef INPUTS_CODEEDITOR
        }

        // Slider
        {
            QString code;
#define INPUTS_SLIDER(X) \
    X(auto *sliderRow = new QHBoxLayout();) \
    X(sliderRow->setContentsMargins(0, 0, 0, 0);) \
    X(sliderRow->setSpacing(10);) \
    X(auto *slider = new FluentSlider(Qt::Horizontal);) \
    X(slider->setRange(0, 100);) \
    X(slider->setValue(30);) \
    X(auto *sliderValue = new FluentLabel(DEMO_TEXT("值：30", "Value: 30"));) \
    X(sliderValue->setStyleSheet("font-size: 12px; opacity: 0.85;");) \
    X(QObject::connect(slider, &QSlider::valueChanged, sliderValue, [=](int v) { sliderValue->setText(DEMO_TEXT("值：%1", "Value: %1").arg(v)); });) \
    X(sliderRow->addWidget(slider, 1);) \
    X(sliderRow->addWidget(sliderValue);) \
    X(body->addLayout(sliderRow);)

#define X(line) code += QStringLiteral(#line "\n");
            INPUTS_SLIDER(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentSlider"),
                DEMO_TEXT("拖动手柄、Hover/Pressed 动效", "Drag handle with hover/pressed animation"),
                DEMO_TEXT("要点：\n"
                          "-横向/纵向：new FluentSlider(Qt::Horizontal/Vertical)\n"
                          "-setRange()/setValue()\n"
                          "-valueChanged 信号用于实时联动 UI",
                          "Highlights:\n"
                          "-Horizontal or vertical: new FluentSlider(Qt::Horizontal / Vertical)\n"
                          "-Use setRange() and setValue()\n"
                          "-Use valueChanged to drive live UI updates"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    INPUTS_SLIDER(X)
#undef X
                },
                false));

#undef INPUTS_SLIDER
        }

        // SpinBox
        {
            QString code;
#define INPUTS_SPINBOX(X) \
    X(auto *spinRow = new QHBoxLayout();) \
    X(spinRow->setContentsMargins(0, 0, 0, 0);) \
    X(spinRow->setSpacing(10);) \
    X(auto *spin = new FluentSpinBox();) \
    X(spin->setRange(0, 999);) \
    X(spin->setValue(12);) \
    X(auto *dspin = new FluentDoubleSpinBox();) \
    X(dspin->setRange(0.0, 99.9);) \
    X(dspin->setDecimals(1);) \
    X(dspin->setValue(3.5);) \
    X(spinRow->addWidget(spin);) \
    X(spinRow->addWidget(dspin);) \
    X(spinRow->addStretch(1);) \
    X(body->addLayout(spinRow);)

#define X(line) code += QStringLiteral(#line "\n");
            INPUTS_SPINBOX(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentSpinBox / FluentDoubleSpinBox"),
                DEMO_TEXT("数值输入（含范围、步进、小数位）", "Numeric input with range, step, and decimals"),
                DEMO_TEXT("要点：\n"
                          "-setRange(min,max) 限定范围\n"
                          "-DoubleSpinBox：setDecimals() 控制小数位\n"
                          "-valueChanged 信号用于参数联动",
                          "Highlights:\n"
                          "-Use setRange(min, max) to constrain the value\n"
                          "-Use setDecimals() on the double spin box to control precision\n"
                          "-Use valueChanged to drive dependent parameters"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    INPUTS_SPINBOX(X)
#undef X
                }));

#undef INPUTS_SPINBOX
        }

        // ComboBox
        {
            QString code;
#define INPUTS_COMBOBOX(X) \
    X(auto *row = new QHBoxLayout();) \
    X(row->setContentsMargins(0, 0, 0, 0);) \
    X(row->setSpacing(10);) \
    X(auto *combo = new FluentComboBox();) \
    X(combo->addItem(DEMO_TEXT("选项一", "Option 1"));) \
    X(combo->addItem(DEMO_TEXT("选项二", "Option 2"));) \
    X(combo->addItem(DEMO_TEXT("选项三", "Option 3"));) \
    X(combo->addItem(DEMO_TEXT("选项四", "Option 4"));) \
    X(combo->addItem(DEMO_TEXT("选项五", "Option 5"));) \
    X(auto *comboDisabled = new FluentComboBox();) \
    X(comboDisabled->addItem(DEMO_TEXT("禁用 ComboBox", "Disabled ComboBox"));) \
    X(comboDisabled->setDisabled(true);) \
    X(auto *label = new FluentLabel(DEMO_TEXT("当前：选项一", "Current: Option 1"));) \
    X(QObject::connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), label, [=](int i) { label->setText(DEMO_TEXT("当前：%1", "Current: %1").arg(combo->itemText(i))); });) \
    X(row->addWidget(combo);) \
    X(row->addWidget(comboDisabled);) \
    X(row->addWidget(label);) \
    X(row->addStretch(1);) \
    X(body->addLayout(row);)

#define X(line) code += QStringLiteral(#line "\n");
            INPUTS_COMBOBOX(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentComboBox"),
                DEMO_TEXT("下拉选择框（含禁用态、Fluent 弹出动效）", "Drop-down selector with disabled state and Fluent popup animation"),
                DEMO_TEXT("要点：\n"
                          "-addItem() 添加选项\n"
                          "-currentIndexChanged 信号用于联动\n"
                          "-setPopupScrollThreshold(n) 超过 n 项时出现滚动条\n"
                          "-setDisabled(true) 展示禁用态",
                          "Highlights:\n"
                          "-Use addItem() to add options\n"
                          "-Use currentIndexChanged for linked updates\n"
                          "-setPopupScrollThreshold(n) shows a scrollbar once the list grows beyond n rows\n"
                          "-Use setDisabled(true) to show the disabled state"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    INPUTS_COMBOBOX(X)
#undef X
                },
                false));

#undef INPUTS_COMBOBOX
        }

        // ComboBox with scroll threshold demo
        {
            QString code;
#define INPUTS_COMBOBOX_SCROLL(X) \
    X(auto *combo2 = new FluentComboBox();) \
    X(combo2->setPopupScrollThreshold(5);) \
    X(for (int i = 1; i <= 12; ++i)) \
    X(    combo2->addItem(DEMO_TEXT("列表项 %1", "Item %1").arg(i));) \
    X(body->addWidget(combo2);)

#define X(line) code += QStringLiteral(#line "\n");
            INPUTS_COMBOBOX_SCROLL(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentComboBox（滚动阈值）"),
                DEMO_TEXT("12 个选项，阈值设为 5：超出后弹窗启用滚动条", "12 options with a threshold of 5: the popup starts scrolling once the limit is exceeded"),
                DEMO_TEXT("要点：\n"
                          "-setPopupScrollThreshold(5) 最多显示 5 行，超出滚动\n"
                          "-默认值为 maxVisibleItems()（Qt 默认 10）",
                          "Highlights:\n"
                          "-setPopupScrollThreshold(5) caps the popup at five visible rows before scrolling\n"
                          "-The default comes from maxVisibleItems() (Qt defaults to 10)"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    INPUTS_COMBOBOX_SCROLL(X)
#undef X
                },
                false));

#undef INPUTS_COMBOBOX_SCROLL
        }

        Q_UNUSED(window);
    });
}

} // namespace Demo::Pages
