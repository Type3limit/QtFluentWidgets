#include "PageInputs.h"

#include "../DemoHelpers.h"
#include "../DemoCodeEditorSettings.h"
#include "Fluent/FluentCard.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

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
        auto s = Demo::makeSection(QStringLiteral("输入控件"),
                                   QStringLiteral("LineEdit / TextEdit / SpinBox（含禁用与占位符展示）"));

        page->addWidget(s.card);

        // LineEdit
        {
            QString code;
#define INPUTS_LINEEDIT(X) \
    X(auto *row = new QHBoxLayout();) \
    X(row->setContentsMargins(0, 0, 0, 0);) \
    X(row->setSpacing(10);) \
    X(auto *le1 = new FluentLineEdit();) \
    X(le1->setPlaceholderText(QStringLiteral("普通 LineEdit"));) \
    X(auto *le2 = new FluentLineEdit();) \
    X(le2->setPlaceholderText(QStringLiteral("禁用 LineEdit"));) \
    X(le2->setDisabled(true);) \
    X(row->addWidget(le1);) \
    X(row->addWidget(le2);) \
    X(body->addLayout(row);)

#define X(line) code += QStringLiteral(#line "\n");
            INPUTS_LINEEDIT(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentLineEdit"),
                QStringLiteral("占位符、禁用态、跟随主题"),
                QStringLiteral("要点：\n"
                               "-setPlaceholderText() 设置占位符\n"
                               "-setDisabled(true) 展示禁用态\n"
                               "-与 ThemeManager 联动更新样式"),
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
            QString code;
#define INPUTS_TEXTEDIT(X) \
    X(auto *te = new FluentTextEdit();) \
    X(te->setPlaceholderText(QStringLiteral("TextEdit：滚动条是自绘 FluentScrollBar（圆角 pill）"));) \
    X(te->setText(QStringLiteral("1. FluentTextEdit\n" \
                                "2. Scrollbar pill handle\n" \
                                "3. Hover/Wheel 显示\n\n" \
                                "（多添加几行测试滚动…）\n\n" \
                                "4\n5\n6\n7\n8\n9\n10\n11\n12\n"));) \
    X(te->setFixedHeight(180);) \
    X(body->addWidget(te);)

#define X(line) code += QStringLiteral(#line "\n");
            INPUTS_TEXTEDIT(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentTextEdit"),
                QStringLiteral("内置 FluentScrollBar（自绘 pill）"),
                QStringLiteral("要点：\n"
                               "-setPlaceholderText()\n"
                               "-setReadOnly(true) 可作为日志/代码展示区\n"
                               "-滚动条随主题与 Accent 联动"),
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
                QStringLiteral("C++ 语法高亮 + clang-format（可在侧边栏配置路径）"),
                QStringLiteral("要点：\n"
                               "-侧边栏 CodeEditor 面板可配置 clang-format 路径\n"
                               "-Ctrl+Shift+F 手动格式化\n"
                               "-可开启 debounce 自动格式化（同样在侧边栏配置）"),
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
    X(auto *sliderValue = new FluentLabel(QStringLiteral("值：30"));) \
    X(sliderValue->setStyleSheet("font-size: 12px; opacity: 0.85;");) \
    X(QObject::connect(slider, &QSlider::valueChanged, sliderValue, [=](int v) { sliderValue->setText(QStringLiteral("值：%1").arg(v)); });) \
    X(sliderRow->addWidget(slider, 1);) \
    X(sliderRow->addWidget(sliderValue);) \
    X(body->addLayout(sliderRow);)

#define X(line) code += QStringLiteral(#line "\n");
            INPUTS_SLIDER(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentSlider"),
                QStringLiteral("拖动手柄、Hover/Pressed 动效"),
                QStringLiteral("要点：\n"
                               "-横向/纵向：new FluentSlider(Qt::Horizontal/Vertical)\n"
                               "-setRange()/setValue()\n"
                               "-valueChanged 信号用于实时联动 UI"),
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
                QStringLiteral("数值输入（含范围、步进、小数位）"),
                QStringLiteral("要点：\n"
                               "-setRange(min,max) 限定范围\n"
                               "-DoubleSpinBox：setDecimals() 控制小数位\n"
                               "-valueChanged 信号用于参数联动"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    INPUTS_SPINBOX(X)
#undef X
                }));

#undef INPUTS_SPINBOX
        }

        Q_UNUSED(window);
    });
}

} // namespace Demo::Pages
