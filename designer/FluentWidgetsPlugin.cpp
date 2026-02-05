#include <QtUiPlugin/QDesignerCustomWidgetCollectionInterface>
#include <QtUiPlugin/QDesignerCustomWidgetInterface>

#include <QIcon>
#include <QColor>
#include <QObject>
#include <QWidget>

#include "Fluent/FluentButton.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentLineEdit.h"
#include "Fluent/FluentTextEdit.h"
#include "Fluent/FluentCalendarPicker.h"
#include "Fluent/FluentTimePicker.h"
#include "Fluent/FluentColorPicker.h"
#include "Fluent/FluentColorDialog.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentCheckBox.h"
#include "Fluent/FluentRadioButton.h"
#include "Fluent/FluentToggleSwitch.h"
#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentSlider.h"
#include "Fluent/FluentProgressBar.h"
#include "Fluent/FluentSpinBox.h"
#include "Fluent/FluentToolButton.h"
#include "Fluent/FluentTabWidget.h"
#include "Fluent/FluentListView.h"
#include "Fluent/FluentTableView.h"
#include "Fluent/FluentTreeView.h"
#include "Fluent/FluentGroupBox.h"
#include "Fluent/FluentMenuBar.h"
#include "Fluent/FluentMenu.h"
#include "Fluent/FluentToolBar.h"
#include "Fluent/FluentStatusBar.h"
#include "Fluent/FluentDialog.h"
#include "Fluent/FluentMessageBox.h"

namespace {

class FluentWidgetPlugin : public QObject, public QDesignerCustomWidgetInterface
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)

public:
    FluentWidgetPlugin(const QString &className,
                       const QString &includeFile,
                       const QString &group,
                       bool container,
                       QObject *parent = nullptr)
        : QObject(parent)
        , m_className(className)
        , m_includeFile(includeFile)
        , m_group(group)
        , m_container(container)
    {
    }

    bool isContainer() const override { return m_container; }
    bool isInitialized() const override { return m_initialized; }
    QIcon icon() const override { return {}; }
    QString domXml() const override
    {
        return QString("<widget class=\"%1\" name=\"%2\"/>")
            .arg(m_className, m_className.toLower());
    }
    QString group() const override { return m_group; }
    QString includeFile() const override { return m_includeFile; }
    QString name() const override { return m_className; }
    QString toolTip() const override { return m_className; }
    QString whatsThis() const override { return m_className; }

    QWidget *createWidget(QWidget *parent) override
    {
        if (m_className == "FluentButton") return new Fluent::FluentButton(parent);
        if (m_className == "FluentLabel") return new Fluent::FluentLabel(parent);
        if (m_className == "FluentLineEdit") return new Fluent::FluentLineEdit(parent);
        if (m_className == "FluentTextEdit") return new Fluent::FluentTextEdit(parent);
        if (m_className == "FluentCalendarPicker") return new Fluent::FluentCalendarPicker(parent);
        if (m_className == "FluentTimePicker") return new Fluent::FluentTimePicker(parent);
        if (m_className == "FluentColorPicker") return new Fluent::FluentColorPicker(parent);
        if (m_className == "FluentColorDialog") return new Fluent::FluentColorDialog(QColor("#0067C0"), parent);
        if (m_className == "FluentCard") return new Fluent::FluentCard(parent);
        if (m_className == "FluentCheckBox") return new Fluent::FluentCheckBox(parent);
        if (m_className == "FluentRadioButton") return new Fluent::FluentRadioButton(parent);
        if (m_className == "FluentToggleSwitch") return new Fluent::FluentToggleSwitch(parent);
        if (m_className == "FluentComboBox") return new Fluent::FluentComboBox(parent);
        if (m_className == "FluentSlider") return new Fluent::FluentSlider(Qt::Horizontal, parent);
        if (m_className == "FluentProgressBar") return new Fluent::FluentProgressBar(parent);
        if (m_className == "FluentSpinBox") return new Fluent::FluentSpinBox(parent);
        if (m_className == "FluentDoubleSpinBox") return new Fluent::FluentDoubleSpinBox(parent);
        if (m_className == "FluentToolButton") return new Fluent::FluentToolButton(parent);
        if (m_className == "FluentTabWidget") return new Fluent::FluentTabWidget(parent);
        if (m_className == "FluentListView") return new Fluent::FluentListView(parent);
        if (m_className == "FluentTableView") return new Fluent::FluentTableView(parent);
        if (m_className == "FluentTreeView") return new Fluent::FluentTreeView(parent);
        if (m_className == "FluentGroupBox") return new Fluent::FluentGroupBox(parent);
        if (m_className == "FluentMenuBar") return new Fluent::FluentMenuBar(parent);
        if (m_className == "FluentMenu") return new Fluent::FluentMenu(parent);
        if (m_className == "FluentToolBar") return new Fluent::FluentToolBar(QString(), parent);
        if (m_className == "FluentStatusBar") return new Fluent::FluentStatusBar(parent);
        if (m_className == "FluentDialog") return new Fluent::FluentDialog(parent);
        if (m_className == "FluentMessageBox") return new Fluent::FluentMessageBox(QStringLiteral("FluentMessageBox"), QStringLiteral("Message"), Fluent::FluentMessageBox::Info, parent);
        return nullptr;
    }

    void initialize(QDesignerFormEditorInterface *core) override
    {
        Q_UNUSED(core)
        if (m_initialized) {
            return;
        }
        m_initialized = true;
    }

private:
    QString m_className;
    QString m_includeFile;
    QString m_group;
    bool m_container = false;
    bool m_initialized = false;
};

class FluentWidgetsCollectionPlugin final : public QObject, public QDesignerCustomWidgetCollectionInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QDesignerCustomWidgetCollectionInterface")
    Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)

public:
    explicit FluentWidgetsCollectionPlugin(QObject *parent = nullptr)
        : QObject(parent)
    {
        const QString group = "Fluent Widgets";
        m_plugins.append(new FluentWidgetPlugin("FluentButton", "Fluent/FluentButton.h", group, false, this));
        m_plugins.append(new FluentWidgetPlugin("FluentLabel", "Fluent/FluentLabel.h", group, false, this));
        m_plugins.append(new FluentWidgetPlugin("FluentLineEdit", "Fluent/FluentLineEdit.h", group, false, this));
        m_plugins.append(new FluentWidgetPlugin("FluentTextEdit", "Fluent/FluentTextEdit.h", group, false, this));
        m_plugins.append(new FluentWidgetPlugin("FluentCalendarPicker", "Fluent/FluentCalendarPicker.h", group, false, this));
        m_plugins.append(new FluentWidgetPlugin("FluentTimePicker", "Fluent/FluentTimePicker.h", group, false, this));
        m_plugins.append(new FluentWidgetPlugin("FluentColorPicker", "Fluent/FluentColorPicker.h", group, false, this));
        m_plugins.append(new FluentWidgetPlugin("FluentColorDialog", "Fluent/FluentColorDialog.h", group, true, this));
        m_plugins.append(new FluentWidgetPlugin("FluentCard", "Fluent/FluentCard.h", group, true, this));
        m_plugins.append(new FluentWidgetPlugin("FluentCheckBox", "Fluent/FluentCheckBox.h", group, false, this));
        m_plugins.append(new FluentWidgetPlugin("FluentRadioButton", "Fluent/FluentRadioButton.h", group, false, this));
        m_plugins.append(new FluentWidgetPlugin("FluentToggleSwitch", "Fluent/FluentToggleSwitch.h", group, false, this));
        m_plugins.append(new FluentWidgetPlugin("FluentComboBox", "Fluent/FluentComboBox.h", group, false, this));
        m_plugins.append(new FluentWidgetPlugin("FluentSlider", "Fluent/FluentSlider.h", group, false, this));
        m_plugins.append(new FluentWidgetPlugin("FluentProgressBar", "Fluent/FluentProgressBar.h", group, false, this));
        m_plugins.append(new FluentWidgetPlugin("FluentSpinBox", "Fluent/FluentSpinBox.h", group, false, this));
        m_plugins.append(new FluentWidgetPlugin("FluentDoubleSpinBox", "Fluent/FluentSpinBox.h", group, false, this));
        m_plugins.append(new FluentWidgetPlugin("FluentToolButton", "Fluent/FluentToolButton.h", group, false, this));
        m_plugins.append(new FluentWidgetPlugin("FluentTabWidget", "Fluent/FluentTabWidget.h", group, true, this));
        m_plugins.append(new FluentWidgetPlugin("FluentListView", "Fluent/FluentListView.h", group, false, this));
        m_plugins.append(new FluentWidgetPlugin("FluentTableView", "Fluent/FluentTableView.h", group, false, this));
        m_plugins.append(new FluentWidgetPlugin("FluentTreeView", "Fluent/FluentTreeView.h", group, false, this));
        m_plugins.append(new FluentWidgetPlugin("FluentGroupBox", "Fluent/FluentGroupBox.h", group, true, this));
        m_plugins.append(new FluentWidgetPlugin("FluentMenuBar", "Fluent/FluentMenuBar.h", group, true, this));
        m_plugins.append(new FluentWidgetPlugin("FluentMenu", "Fluent/FluentMenu.h", group, true, this));
        m_plugins.append(new FluentWidgetPlugin("FluentToolBar", "Fluent/FluentToolBar.h", group, true, this));
        m_plugins.append(new FluentWidgetPlugin("FluentStatusBar", "Fluent/FluentStatusBar.h", group, true, this));
        m_plugins.append(new FluentWidgetPlugin("FluentDialog", "Fluent/FluentDialog.h", group, true, this));
        m_plugins.append(new FluentWidgetPlugin("FluentMessageBox", "Fluent/FluentMessageBox.h", group, true, this));
    }

    QList<QDesignerCustomWidgetInterface *> customWidgets() const override
    {
        return m_plugins;
    }

private:
    QList<QDesignerCustomWidgetInterface *> m_plugins;
};

} // namespace

#include "FluentWidgetsPlugin.moc"
