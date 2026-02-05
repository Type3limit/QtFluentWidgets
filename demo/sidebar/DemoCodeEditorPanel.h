#pragma once

#include <QWidget>

class QLabel;
class QTimer;

namespace Fluent {
class FluentCheckBox;
class FluentCodeEditor;
class FluentLineEdit;
class FluentSpinBox;
}

namespace Demo {

class DemoCodeEditorPanel final : public QWidget
{
    Q_OBJECT
public:
    explicit DemoCodeEditorPanel(QWidget *parent = nullptr);

private:
    void refreshStatus();

    Fluent::FluentLineEdit *m_pathEdit = nullptr;
    QLabel *m_statusLabel = nullptr;

    Fluent::FluentCheckBox *m_lineNumbers = nullptr;
    Fluent::FluentCheckBox *m_autoFormat = nullptr;
    Fluent::FluentCheckBox *m_autoFormatOnEnterOrFocusOut = nullptr;
    Fluent::FluentSpinBox *m_debounce = nullptr;
    Fluent::FluentCheckBox *m_missingHint = nullptr;
    Fluent::FluentSpinBox *m_fontSize = nullptr;

    Fluent::FluentCodeEditor *m_preview = nullptr;

    QTimer *m_statusTimer = nullptr;
};

} // namespace Demo
