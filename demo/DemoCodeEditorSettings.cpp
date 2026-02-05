#include "DemoCodeEditorSettings.h"

#include "Fluent/FluentCodeEditor.h"

#include <QFont>
#include <QSettings>
#include <QStandardPaths>

namespace Demo {

static QSettings &demoSettings()
{
    static QSettings s(QStringLiteral("QtFluent"), QStringLiteral("QtFluentDemo"));
    return s;
}

DemoCodeEditorSettings &DemoCodeEditorSettings::instance()
{
    static DemoCodeEditorSettings s;
    return s;
}

DemoCodeEditorSettings::DemoCodeEditorSettings(QObject *parent)
    : QObject(parent)
{
    load();
}

void DemoCodeEditorSettings::load()
{
    auto &s = demoSettings();
    m_clangPathText = s.value(QStringLiteral("codeEditor/clangFormatPath"), QString()).toString();
    m_lineNumbersEnabled = s.value(QStringLiteral("codeEditor/lineNumbers"), true).toBool();
    m_autoFormatEnabled = s.value(QStringLiteral("codeEditor/autoFormat"), true).toBool();
    m_autoFormatDebounceMs = s.value(QStringLiteral("codeEditor/autoFormatDebounceMs"), 1200).toInt();
    m_autoFormatTriggerPolicy = s.value(QStringLiteral("codeEditor/autoFormatTriggerPolicy"), 0).toInt();
    m_clangMissingHintEnabled = s.value(QStringLiteral("codeEditor/clangMissingHint"), true).toBool();
    m_fontPointSize = s.value(QStringLiteral("codeEditor/fontPointSize"), 12).toInt();
}

void DemoCodeEditorSettings::save() const
{
    auto &s = demoSettings();
    s.setValue(QStringLiteral("codeEditor/clangFormatPath"), m_clangPathText);
    s.setValue(QStringLiteral("codeEditor/lineNumbers"), m_lineNumbersEnabled);
    s.setValue(QStringLiteral("codeEditor/autoFormat"), m_autoFormatEnabled);
    s.setValue(QStringLiteral("codeEditor/autoFormatDebounceMs"), m_autoFormatDebounceMs);
    s.setValue(QStringLiteral("codeEditor/autoFormatTriggerPolicy"), m_autoFormatTriggerPolicy);
    s.setValue(QStringLiteral("codeEditor/clangMissingHint"), m_clangMissingHintEnabled);
    s.setValue(QStringLiteral("codeEditor/fontPointSize"), m_fontPointSize);
}

void DemoCodeEditorSettings::setClangFormatPathText(const QString &text)
{
    const QString t = text.trimmed();
    if (m_clangPathText == t) {
        return;
    }
    m_clangPathText = t;
    save();
    emit changed();
}

QString DemoCodeEditorSettings::clangFormatPathText() const
{
    return m_clangPathText;
}

QString DemoCodeEditorSettings::resolvedClangFormatPath() const
{
    if (m_clangPathText.isEmpty()) {
        return QStandardPaths::findExecutable(QStringLiteral("clang-format"));
    }
    return QStandardPaths::findExecutable(m_clangPathText);
}

void DemoCodeEditorSettings::setLineNumbersEnabled(bool enabled)
{
    if (m_lineNumbersEnabled == enabled) {
        return;
    }
    m_lineNumbersEnabled = enabled;
    save();
    emit changed();
}

bool DemoCodeEditorSettings::lineNumbersEnabled() const
{
    return m_lineNumbersEnabled;
}

void DemoCodeEditorSettings::setAutoFormatEnabled(bool enabled)
{
    if (m_autoFormatEnabled == enabled) {
        return;
    }
    m_autoFormatEnabled = enabled;
    save();
    emit changed();
}

bool DemoCodeEditorSettings::autoFormatEnabled() const
{
    return m_autoFormatEnabled;
}

void DemoCodeEditorSettings::setAutoFormatDebounceMs(int ms)
{
    ms = qMax(0, ms);
    if (m_autoFormatDebounceMs == ms) {
        return;
    }
    m_autoFormatDebounceMs = ms;
    save();
    emit changed();
}

int DemoCodeEditorSettings::autoFormatDebounceMs() const
{
    return m_autoFormatDebounceMs;
}

void DemoCodeEditorSettings::setAutoFormatTriggerPolicy(Fluent::FluentCodeEditor::AutoFormatTriggerPolicy policy)
{
    const int p = static_cast<int>(policy);
    if (m_autoFormatTriggerPolicy == p) {
        return;
    }
    m_autoFormatTriggerPolicy = p;
    save();
    emit changed();
}

Fluent::FluentCodeEditor::AutoFormatTriggerPolicy DemoCodeEditorSettings::autoFormatTriggerPolicy() const
{
    const int p = m_autoFormatTriggerPolicy;
    if (p == static_cast<int>(Fluent::FluentCodeEditor::AutoFormatTriggerPolicy::OnEnterOrFocusOut)) {
        return Fluent::FluentCodeEditor::AutoFormatTriggerPolicy::OnEnterOrFocusOut;
    }
    return Fluent::FluentCodeEditor::AutoFormatTriggerPolicy::DebouncedOnTextChange;
}

void DemoCodeEditorSettings::setClangFormatMissingHintEnabled(bool enabled)
{
    if (m_clangMissingHintEnabled == enabled) {
        return;
    }
    m_clangMissingHintEnabled = enabled;
    save();
    emit changed();
}

bool DemoCodeEditorSettings::clangFormatMissingHintEnabled() const
{
    return m_clangMissingHintEnabled;
}

void DemoCodeEditorSettings::setFontPointSize(int pt)
{
    pt = qMax(8, pt);
    if (m_fontPointSize == pt) {
        return;
    }
    m_fontPointSize = pt;
    save();
    emit changed();
}

int DemoCodeEditorSettings::fontPointSize() const
{
    return m_fontPointSize;
}

void DemoCodeEditorSettings::applyTo(Fluent::FluentCodeEditor *editor, bool snippetMode) const
{
    if (!editor) {
        return;
    }

    editor->setLineNumbersEnabled(m_lineNumbersEnabled);

    editor->setClangFormatPath(resolvedClangFormatPath());
    editor->setClangFormatMissingHintEnabled(m_clangMissingHintEnabled);

    editor->setAutoFormatTriggerPolicy(autoFormatTriggerPolicy());

    // Snippets in cards should stay quiet & deterministic.
    if (snippetMode) {
        editor->setAutoFormatEnabled(false);
        editor->setAutoBraceNewlineEnabled(false);
    } else {
        editor->setAutoFormatEnabled(m_autoFormatEnabled);
        editor->setAutoFormatDebounceMs(m_autoFormatDebounceMs);
    }

    QFont f;
    f.setFamilies({QStringLiteral("Consolas"), QStringLiteral("Cascadia Mono"), QStringLiteral("Courier New")});
    f.setStyleHint(QFont::Monospace);
    f.setFixedPitch(true);
    f.setPointSize(m_fontPointSize);
    editor->setFont(f);
}

void DemoCodeEditorSettings::attach(Fluent::FluentCodeEditor *editor, bool snippetMode)
{
    if (!editor) {
        return;
    }

    applyTo(editor, snippetMode);

    QObject::connect(this, &DemoCodeEditorSettings::changed, editor, [this, editor, snippetMode]() {
        applyTo(editor, snippetMode);
    });
}

} // namespace Demo
