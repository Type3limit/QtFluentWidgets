#include "DemoCodeEditorPanel.h"

#include "../DemoHelpers.h"
#include "../DemoCodeEditorSettings.h"

#include "Fluent/FluentButton.h"
#include "Fluent/FluentCheckBox.h"
#include "Fluent/FluentCodeEditor.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentLineEdit.h"
#include "Fluent/FluentSpinBox.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

namespace Demo {

using namespace Fluent;

static QWidget *row(QWidget *a, QWidget *b)
{
    auto *w = new QWidget();
    auto *l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(8);
    l->addWidget(a, 1);
    l->addWidget(b);
    return w;
}

DemoCodeEditorPanel::DemoCodeEditorPanel(QWidget *parent, bool showTitle)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    if (showTitle) {
        auto *title = new FluentLabel(QStringLiteral("CodeEditor"));
        title->setStyleSheet("font-size: 12px; font-weight: 650;");
        layout->addWidget(title);
    }

    m_pathEdit = new FluentLineEdit();
    m_pathEdit->setPlaceholderText(DEMO_TEXT("clang-format 路径（留空=自动检测）", "clang-format path (leave empty to auto-detect)"));
    m_pathEdit->setText(DemoCodeEditorSettings::instance().clangFormatPathText());

    auto *browse = new FluentButton(DEMO_TEXT("浏览…", "Browse..."));
    browse->setFixedHeight(28);

    auto *autoBtn = new FluentButton(DEMO_TEXT("自动", "Auto"));
    autoBtn->setFixedHeight(28);

    auto *pathRow = new QWidget();
    {
        auto *hl = new QHBoxLayout(pathRow);
        hl->setContentsMargins(0, 0, 0, 0);
        hl->setSpacing(8);
        hl->addWidget(m_pathEdit, 1);
        hl->addWidget(browse);
        hl->addWidget(autoBtn);
    }
    layout->addWidget(pathRow);

    m_statusLabel = new QLabel();
    m_statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_statusLabel->setStyleSheet("font-size: 12px; opacity: 0.85;");
    layout->addWidget(m_statusLabel);

    m_lineNumbers = new FluentCheckBox(DEMO_TEXT("行号", "Line numbers"));
    m_lineNumbers->setChecked(DemoCodeEditorSettings::instance().lineNumbersEnabled());

    m_autoFormat = new FluentCheckBox(DEMO_TEXT("自动格式化", "Auto format"));
    m_autoFormat->setChecked(DemoCodeEditorSettings::instance().autoFormatEnabled());

    m_autoFormatOnEnterOrFocusOut = new FluentCheckBox(DEMO_TEXT("仅失焦/回车后触发", "Only on focus-out / Enter"));
    m_autoFormatOnEnterOrFocusOut->setToolTip(DEMO_TEXT("开启后：不会在连续输入过程中触发自动格式化，仅在回车换行或失焦时触发。", "When enabled, auto-formatting will not run during continuous typing. It only runs after pressing Enter or when focus leaves the editor."));
    m_autoFormatOnEnterOrFocusOut->setChecked(
        DemoCodeEditorSettings::instance().autoFormatTriggerPolicy() == FluentCodeEditor::AutoFormatTriggerPolicy::OnEnterOrFocusOut);

    m_debounce = new FluentSpinBox();
    m_debounce->setRange(0, 5000);
    m_debounce->setValue(DemoCodeEditorSettings::instance().autoFormatDebounceMs());
    m_debounce->setSuffix(QStringLiteral(" ms"));
    m_debounce->setToolTip(DEMO_TEXT("自动格式化节流时间", "Auto-format debounce delay"));

    m_missingHint = new FluentCheckBox(DEMO_TEXT("缺失提示", "Missing-tool hint"));
    m_missingHint->setChecked(DemoCodeEditorSettings::instance().clangFormatMissingHintEnabled());

    m_fontSize = new FluentSpinBox();
    m_fontSize->setRange(8, 22);
    m_fontSize->setValue(DemoCodeEditorSettings::instance().fontPointSize());
    m_fontSize->setSuffix(QStringLiteral(" pt"));
    m_fontSize->setToolTip(DEMO_TEXT("代码字体大小", "Code font size"));

    layout->addWidget(row(m_lineNumbers, m_fontSize));
    layout->addWidget(row(m_autoFormat, m_debounce));
    layout->addWidget(m_autoFormatOnEnterOrFocusOut);
    layout->addWidget(m_missingHint);

    m_preview = new FluentCodeEditor();
    m_preview->setFixedHeight(200);
    m_preview->setPlainText(QStringLiteral(
        "#include <vector>\n\n"
        "struct Foo {\n"
        "    int x=1;\n"
        "    void bar(){ if(x){x++;} }\n"
        "};\n\n"
        "int main(){ std::vector<int> v{1,2,3}; Foo f; f.bar(); }\n"));
    DemoCodeEditorSettings::instance().attach(m_preview, false);
    layout->addWidget(m_preview);

    // Apply settings to all existing/new snippet editors via DemoHelpers.
    QObject::connect(&DemoCodeEditorSettings::instance(), &DemoCodeEditorSettings::changed, this, [this]() {
        refreshStatus();
    });

    // typing in the path edit: debounce status refresh + settings apply
    m_statusTimer = new QTimer(this);
    m_statusTimer->setSingleShot(true);
    m_statusTimer->setInterval(250);
    QObject::connect(m_statusTimer, &QTimer::timeout, this, [this]() {
        DemoCodeEditorSettings::instance().setClangFormatPathText(m_pathEdit->text());
        refreshStatus();
    });

    QObject::connect(m_pathEdit, &QLineEdit::textChanged, this, [this]() {
        m_statusTimer->start();
    });

    QObject::connect(browse, &QPushButton::clicked, this, [this]() {
        const QString filter = QStringLiteral("clang-format (clang-format*);;Executables (*.exe);;All Files (*.*)");
        const QString file = QFileDialog::getOpenFileName(this, DEMO_TEXT("选择 clang-format", "Select clang-format"), QString(), filter);
        if (!file.isEmpty()) {
            m_pathEdit->setText(file);
            DemoCodeEditorSettings::instance().setClangFormatPathText(file);
            refreshStatus();
        }
    });

    QObject::connect(autoBtn, &QPushButton::clicked, this, [this]() {
        m_pathEdit->setText(QString());
        DemoCodeEditorSettings::instance().setClangFormatPathText(QString());
        refreshStatus();
    });

    QObject::connect(m_lineNumbers, &QAbstractButton::toggled, this, [](bool on) {
        DemoCodeEditorSettings::instance().setLineNumbersEnabled(on);
    });

    QObject::connect(m_autoFormat, &QAbstractButton::toggled, this, [](bool on) {
        DemoCodeEditorSettings::instance().setAutoFormatEnabled(on);
    });

    QObject::connect(m_autoFormatOnEnterOrFocusOut, &QAbstractButton::toggled, this, [](bool on) {
        DemoCodeEditorSettings::instance().setAutoFormatTriggerPolicy(
            on ? FluentCodeEditor::AutoFormatTriggerPolicy::OnEnterOrFocusOut
               : FluentCodeEditor::AutoFormatTriggerPolicy::DebouncedOnTextChange);
    });

    QObject::connect(m_debounce, QOverload<int>::of(&QSpinBox::valueChanged), this, [](int v) {
        DemoCodeEditorSettings::instance().setAutoFormatDebounceMs(v);
    });

    QObject::connect(m_missingHint, &QAbstractButton::toggled, this, [](bool on) {
        DemoCodeEditorSettings::instance().setClangFormatMissingHintEnabled(on);
    });

    QObject::connect(m_fontSize, QOverload<int>::of(&QSpinBox::valueChanged), this, [](int v) {
        DemoCodeEditorSettings::instance().setFontPointSize(v);
    });

    refreshStatus();
}

void DemoCodeEditorPanel::refreshStatus()
{
    const QString resolved = DemoCodeEditorSettings::instance().resolvedClangFormatPath();
    if (resolved.isEmpty()) {
        m_statusLabel->setText(DEMO_TEXT("clang-format：未找到（将使用基础格式化）", "clang-format: not found (falling back to basic formatting)"));
    } else {
        m_statusLabel->setText(DEMO_TEXT("clang-format：%1", "clang-format: %1").arg(resolved));
    }
}

} // namespace Demo
