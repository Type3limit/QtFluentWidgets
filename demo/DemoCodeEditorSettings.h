#pragma once

#include <QObject>
#include <QString>

#include "Fluent/FluentCodeEditor.h"

namespace Demo {

class DemoCodeEditorSettings final : public QObject
{
    Q_OBJECT
public:
    static DemoCodeEditorSettings &instance();

    // If empty => auto-detect via PATH (clang-format)
    void setClangFormatPathText(const QString &text);
    QString clangFormatPathText() const;

    QString resolvedClangFormatPath() const;

    void setLineNumbersEnabled(bool enabled);
    bool lineNumbersEnabled() const;

    void setAutoFormatEnabled(bool enabled);
    bool autoFormatEnabled() const;

    void setAutoFormatDebounceMs(int ms);
    int autoFormatDebounceMs() const;

    void setAutoFormatTriggerPolicy(Fluent::FluentCodeEditor::AutoFormatTriggerPolicy policy);
    Fluent::FluentCodeEditor::AutoFormatTriggerPolicy autoFormatTriggerPolicy() const;

    void setClangFormatMissingHintEnabled(bool enabled);
    bool clangFormatMissingHintEnabled() const;

    void setFontPointSize(int pt);
    int fontPointSize() const;

    // Binds editor to current settings and keeps it in sync.
    // Use snippetMode=true for read-only code blocks (keeps auto-format off).
    void attach(Fluent::FluentCodeEditor *editor, bool snippetMode);

signals:
    void changed();

private:
    explicit DemoCodeEditorSettings(QObject *parent = nullptr);
    void load();
    void save() const;

    void applyTo(Fluent::FluentCodeEditor *editor, bool snippetMode) const;

    QString m_clangPathText;
    bool m_lineNumbersEnabled = true;
    bool m_autoFormatEnabled = true;
    int m_autoFormatDebounceMs = 1200;
    int m_autoFormatTriggerPolicy = 0;
    bool m_clangMissingHintEnabled = true;
    int m_fontPointSize = 12;
};

} // namespace Demo
