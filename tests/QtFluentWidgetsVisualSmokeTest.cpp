#include "Fluent/FluentButton.h"
#include "Fluent/FluentCalendarPicker.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentCheckBox.h"
#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentCommandBar.h"
#include "Fluent/FluentDatePicker.h"
#include "Fluent/FluentDropDownButton.h"
#include "Fluent/FluentAutoSuggestBox.h"
#include "Fluent/FluentFlowLayout.h"
#include "Fluent/FluentInfoBar.h"
#include "Fluent/FluentIcon.h"
#include "Fluent/FluentKeySequenceEdit.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentLineEdit.h"
#include "Fluent/FluentListView.h"
#include "Fluent/FluentMenu.h"
#include "Fluent/FluentMotion.h"
#include "Fluent/FluentPopupSurface.h"
#include "Fluent/FluentProgressBar.h"
#include "Fluent/FluentProgressRing.h"
#include "Fluent/FluentRadioButton.h"
#include "Fluent/FluentSlider.h"
#include "Fluent/FluentSplitButton.h"
#include "Fluent/FluentSpinBox.h"
#include "Fluent/FluentTableView.h"
#include "Fluent/FluentTableWidget.h"
#include "Fluent/FluentTabWidget.h"
#include "Fluent/FluentTeachingTip.h"
#include "Fluent/FluentTextEdit.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentTimePicker.h"
#include "Fluent/FluentToggleSwitch.h"
#include "Fluent/FluentToolButton.h"
#include "Fluent/FluentTreeView.h"

#include <QtTest/QtTest>

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QDate>
#include <QDir>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QHash>
#include <QHeaderView>
#include <QImage>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QListView>
#include <QListWidget>
#include <QPainter>
#include <QPointer>
#include <QPixmap>
#include <QRegularExpression>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QTableWidgetItem>
#include <QTabBar>
#include <QTime>
#include <QVBoxLayout>

#include <cmath>
#include <memory>
#include <utility>

using namespace Fluent;

namespace {

struct Bucket {
    qint64 count = 0;
    qint64 r = 0;
    qint64 g = 0;
    qint64 b = 0;
};

struct ContrastStats {
    QColor background;
    qreal maxContrast = 1.0;
    qreal readableRatio = 0.0;
    int readablePixels = 0;
    int sampledPixels = 0;
};

struct CheckTarget {
    QString name;
    QPointer<QWidget> widget;
    QRect area;
    qreal minContrast = 3.0;
    qreal minReadableRatio = 0.006;
};

QStringList *g_teachingTipGeometryWarnings = nullptr;
QtMessageHandler g_previousMessageHandler = nullptr;

void captureTeachingTipGeometryWarnings(QtMsgType type,
                                        const QMessageLogContext &context,
                                        const QString &message)
{
    if (type == QtWarningMsg
        && g_teachingTipGeometryWarnings
        && message.contains(QStringLiteral("QWindowsWindow::setGeometry"))
        && message.contains(QStringLiteral("FluentTeachingTipClassWindow"))) {
        g_teachingTipGeometryWarnings->append(message);
    }

    if (g_previousMessageHandler) {
        g_previousMessageHandler(type, context, message);
    }
}

class SmokeScene final {
public:
    QWidget *window = nullptr;
    QList<CheckTarget> targets;
    QPointer<FluentButton> hoverButton;
    QPointer<FluentLineEdit> focusEdit;
    QPointer<FluentListView> listView;
    QPointer<FluentTableView> tableView;
    QPointer<FluentTableWidget> tableWidget;
    QPointer<FluentTabWidget> tabWidget;
    QPointer<FluentTreeView> treeView;
    QPointer<FluentSearchBox> searchBox;
    QPointer<FluentComboBox> comboBox;
    QPointer<FluentCalendarPicker> calendarPicker;
    QPointer<FluentDatePicker> datePicker;
    QPointer<FluentTimePicker> timePicker;
    QPointer<FluentTextEdit> textEdit;
};

qreal linearChannel(qreal value)
{
    return value <= 0.03928 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
}

qreal relativeLuminance(const QColor &color)
{
    return 0.2126 * linearChannel(color.redF())
        + 0.7152 * linearChannel(color.greenF())
        + 0.0722 * linearChannel(color.blueF());
}

qreal contrastRatio(const QColor &a, const QColor &b)
{
    const qreal la = relativeLuminance(a);
    const qreal lb = relativeLuminance(b);
    const qreal lighter = qMax(la, lb);
    const qreal darker = qMin(la, lb);
    return (lighter + 0.05) / (darker + 0.05);
}

QRect normalizedArea(const QImage &image, const QRect &area)
{
    const QRect imageRect(QPoint(0, 0), image.size());
    const QRect result = (area.isValid() ? area : imageRect.adjusted(4, 4, -4, -4)).intersected(imageRect);
    return result.isValid() ? result : imageRect;
}

QColor dominantColor(const QImage &image, const QRect &rawArea)
{
    const QRect area = normalizedArea(image, rawArea);
    QHash<int, Bucket> buckets;
    buckets.reserve(64);

    for (int y = area.top(); y <= area.bottom(); ++y) {
        const auto *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = area.left(); x <= area.right(); ++x) {
            const QColor c = QColor::fromRgba(line[x]);
            if (c.alpha() < 220) {
                continue;
            }
            const int key = ((c.red() >> 4) << 8) | ((c.green() >> 4) << 4) | (c.blue() >> 4);
            Bucket &bucket = buckets[key];
            ++bucket.count;
            bucket.r += c.red();
            bucket.g += c.green();
            bucket.b += c.blue();
        }
    }

    Bucket best;
    for (auto it = buckets.constBegin(); it != buckets.constEnd(); ++it) {
        if (it.value().count > best.count) {
            best = it.value();
        }
    }

    if (best.count <= 0) {
        return QColor(Qt::transparent);
    }
    return QColor(static_cast<int>(best.r / best.count),
                  static_cast<int>(best.g / best.count),
                  static_cast<int>(best.b / best.count));
}

ContrastStats analyzeContrast(const QImage &image, const QRect &rawArea, qreal minContrast)
{
    const QRect area = normalizedArea(image, rawArea);
    ContrastStats stats;
    stats.background = dominantColor(image, area);
    if (!stats.background.isValid()) {
        return stats;
    }

    for (int y = area.top(); y <= area.bottom(); ++y) {
        const auto *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = area.left(); x <= area.right(); ++x) {
            const QColor c = QColor::fromRgba(line[x]);
            if (c.alpha() < 220) {
                continue;
            }
            ++stats.sampledPixels;
            const qreal contrast = contrastRatio(c, stats.background);
            stats.maxContrast = qMax(stats.maxContrast, contrast);
            if (contrast >= minContrast) {
                ++stats.readablePixels;
            }
        }
    }

    if (stats.sampledPixels > 0) {
        stats.readableRatio = static_cast<qreal>(stats.readablePixels) / stats.sampledPixels;
    }
    return stats;
}

QString safeFileName(QString text)
{
    text.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_.-]+")), QStringLiteral("_"));
    return text.left(120);
}

QString outputDir()
{
    const QString env = qEnvironmentVariable("QTFLUENT_VISUAL_SMOKE_OUTPUT_DIR");
    const QString dir = env.isEmpty()
        ? QDir(QDir::currentPath()).filePath(QStringLiteral("visual-smoke-output"))
        : env;
    QDir().mkpath(dir);
    return dir;
}

bool visualModeEnabled()
{
    return qEnvironmentVariableIntValue("QTFLUENT_VISUAL_SMOKE_VISIBLE") != 0;
}

int visualHoldMs()
{
    bool ok = false;
    const int value = qEnvironmentVariableIntValue("QTFLUENT_VISUAL_SMOKE_VISIBLE_MS", &ok);
    if (!ok) {
        return 3000;
    }
    return value < 0 ? 0 : value;
}

void showForInspection(QWidget *window)
{
    if (!visualModeEnabled() || !window) {
        return;
    }

    const int holdMs = visualHoldMs();
    if (holdMs == 0) {
        while (window->isVisible()) {
            QTest::qWait(50);
        }
        return;
    }
    QTest::qWait(holdMs);
}

QString saveImage(const QString &name, const QImage &image)
{
    const QString path = QDir(outputDir()).filePath(safeFileName(name) + QStringLiteral(".png"));
    image.save(path);
    return path;
}

QString verifyReadableTarget(const CheckTarget &target, const QString &modeName)
{
    if (!target.widget) {
        return QStringLiteral("%1/%2 widget was deleted").arg(modeName, target.name);
    }

    QWidget *root = target.widget->window();
    if (!root) {
        root = target.widget;
    }

    QPixmap pixmap = root->grab();
    QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    const QPoint widgetOrigin = target.widget->mapTo(root, QPoint(0, 0));
    const QRect widgetRect(widgetOrigin, target.widget->size());
    const QRect area = target.area.isValid()
        ? QRect(widgetOrigin + target.area.topLeft(), target.area.size())
        : widgetRect.adjusted(5, 5, -5, -5);
    const ContrastStats stats = analyzeContrast(image, area, target.minContrast);

    if (stats.maxContrast < target.minContrast || stats.readableRatio < target.minReadableRatio) {
        const QString path = saveImage(modeName + QStringLiteral("_") + target.name, image);
        return QStringLiteral("%1/%2 low text contrast: max=%3 min=%4 readable=%5 minReadable=%6 bg=%7 image=%8")
            .arg(modeName,
                 target.name,
                 QString::number(stats.maxContrast, 'f', 2),
                 QString::number(target.minContrast, 'f', 2),
                 QString::number(stats.readableRatio, 'f', 4),
                 QString::number(target.minReadableRatio, 'f', 4),
                 stats.background.name(),
                 path);
    }

    return {};
}

void syncTheme(bool dark, const QColor &accent)
{
    QColor baseAccent = accent;
    if (!baseAccent.isValid()) {
        baseAccent = QColor(QStringLiteral("#0066B4"));
    }
    ThemeManager::instance().setAccentColor(baseAccent);
    ThemeManager::instance().setThemeMode(dark ? ThemeManager::ThemeMode::Dark : ThemeManager::ThemeMode::Light);
    QCoreApplication::processEvents();
    QTest::qWait(20);
    QCoreApplication::processEvents();
}

FluentCard *makeCard(const QString &title, QWidget *parent)
{
    auto *card = new FluentCard(parent);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(10);

    auto *label = new FluentLabel(title, card);
    QFont font = label->font();
    font.setWeight(QFont::DemiBold);
    label->setFont(font);
    layout->addWidget(label);
    return card;
}

QAbstractButton *findButtonByText(QWidget *root, const QString &text)
{
    const auto buttons = root->findChildren<QAbstractButton *>();
    for (QAbstractButton *button : buttons) {
        if (button && button->text() == text) {
            return button;
        }
    }
    return nullptr;
}

QRect buttonTextArea(const QAbstractButton *button)
{
    if (!button || button->text().isEmpty()) {
        return {};
    }

    const QFontMetrics fm(button->font());
    const int width = qMin(button->width() - 8, fm.horizontalAdvance(button->text()) + 8);
    const int height = qMin(button->height() - 6, fm.height() + 6);
    return QRect((button->width() - width) / 2,
                 (button->height() - height) / 2,
                 width,
                 height).intersected(button->rect());
}

QRect viewTextArea(const QAbstractItemView *view, const QModelIndex &index, int leftInset = 12)
{
    if (!view || !index.isValid()) {
        return {};
    }

    const QRect visual = view->visualRect(index);
    if (!visual.isValid()) {
        return {};
    }

    return visual.adjusted(leftInset, 3, -8, -3).intersected(view->viewport()->rect());
}

QRect headerTextArea(const QHeaderView *header, int logicalIndex)
{
    if (!header || logicalIndex < 0 || logicalIndex >= header->count()) {
        return {};
    }

    QRect rect(header->sectionViewportPosition(logicalIndex), 0,
               header->sectionSize(logicalIndex), header->height());
    return rect.adjusted(10, 3, -8, -3).intersected(header->viewport()->rect());
}

QListView *findAutoSuggestPopupView()
{
    const auto widgets = QApplication::topLevelWidgets();
    for (QWidget *widget : widgets) {
        if (!widget || !widget->isVisible() || widget->objectName() != QStringLiteral("FluentAutoSuggestPopup")) {
            continue;
        }
        if (auto *view = widget->findChild<QListView *>(QStringLiteral("FluentAutoSuggestPopupView"))) {
            return view;
        }
    }
    return nullptr;
}

QWidget *findVisibleTopLevelByObjectName(const QString &objectName)
{
    const auto widgets = QApplication::topLevelWidgets();
    for (QWidget *widget : widgets) {
        if (widget && widget->isVisible() && widget->objectName() == objectName) {
            return widget;
        }
    }
    return nullptr;
}

QListView *findComboPopupView()
{
    if (QWidget *popup = findVisibleTopLevelByObjectName(QStringLiteral("FluentComboPopup"))) {
        return popup->findChild<QListView *>(QStringLiteral("FluentComboPopupView"));
    }
    return nullptr;
}

QRect popupMenuTextArea(QWidget *popupHost, int itemIndex)
{
    if (!popupHost || itemIndex < 0) {
        return {};
    }

    constexpr int kOuterPadding = 4;
    constexpr int kItemHeight = 34;
    const QRect content = PopupSurface::shadowContentRect(popupHost->rect());
    const QRect itemRect(content.left() + 4,
                         content.top() + kOuterPadding + itemIndex * kItemHeight,
                         qMax(0, content.width() - 8),
                         kItemHeight);
    return itemRect.adjusted(32, 4, -18, -4).intersected(popupHost->rect());
}

QDate calendarGridStartForMonth(int year, int month, Qt::DayOfWeek firstDayOfWeek)
{
    const QDate first(year, month, 1);
    if (!first.isValid()) {
        return {};
    }

    const int shift = (first.dayOfWeek() - int(firstDayOfWeek) + 7) % 7;
    return first.addDays(-shift);
}

QRect calendarPopupDateCellArea(QWidget *popupHost, const QDate &date, Qt::DayOfWeek firstDayOfWeek)
{
    if (!popupHost || !date.isValid()) {
        return {};
    }

    constexpr int kPadding = 10;
    constexpr int kHeaderH = 44;
    constexpr int kDayNamesH = 24;

    const QRect content = PopupSurface::shadowContentRect(popupHost->rect()).adjusted(
        kPadding, kPadding, -kPadding, -kPadding);
    const QRect grid(content.x(),
                     content.y() + kHeaderH,
                     content.width(),
                     content.height() - kHeaderH);
    const QRect cells(grid.x(),
                      grid.y() + kDayNamesH,
                      grid.width(),
                      grid.height() - kDayNamesH);
    const int cw = cells.width() / 7;
    const int ch = cells.height() / 6;
    if (cw <= 0 || ch <= 0) {
        return {};
    }

    const QDate start = calendarGridStartForMonth(date.year(), date.month(), firstDayOfWeek);
    if (!start.isValid()) {
        return {};
    }
    const int index = start.daysTo(date);
    if (index < 0 || index >= 42) {
        return {};
    }

    const int row = index / 7;
    const int col = index % 7;
    const QRect cell(cells.x() + col * cw, cells.y() + row * ch, cw, ch);
    return cell.adjusted(7, 5, -7, -5).intersected(popupHost->rect());
}

QStringList verifyWheelPickerPopupColumns(QWidget *popupHost,
                                          const QString &prefix,
                                          const QString &modeName)
{
    QStringList failures;
    if (!popupHost) {
        failures << QStringLiteral("%1/%2 popup was not visible").arg(modeName, prefix);
        return failures;
    }

    const auto columns = popupHost->findChildren<QListWidget *>();
    int visibleColumnIndex = 0;
    for (QListWidget *column : columns) {
        if (!column || !column->isVisible() || !column->viewport()) {
            continue;
        }

        const QModelIndex index = column->currentIndex();
        if (!index.isValid()) {
            failures << QStringLiteral("%1/%2 column %3 has no current index")
                            .arg(modeName, prefix, QString::number(visibleColumnIndex));
            ++visibleColumnIndex;
            continue;
        }

        const QString failure = verifyReadableTarget(
            {QStringLiteral("%1-column-%2-current").arg(prefix, QString::number(visibleColumnIndex)),
             column->viewport(),
             viewTextArea(column, index, 9),
             3.0,
             0.004},
            modeName);
        if (!failure.isEmpty()) {
            failures << failure;
        }
        ++visibleColumnIndex;
    }

    if (visibleColumnIndex == 0) {
        failures << QStringLiteral("%1/%2 popup had no visible wheel columns").arg(modeName, prefix);
    }
    return failures;
}

SmokeScene createSmokeScene()
{
    SmokeScene scene;
    auto *window = new QWidget();
    scene.window = window;
    window->setObjectName(QStringLiteral("QtFluentVisualSmokeWindow"));
    window->resize(980, 680);
    window->setAutoFillBackground(true);

    QPalette pal = window->palette();
    pal.setColor(QPalette::Window, ThemeManager::instance().colors().background);
    window->setPalette(pal);

    auto *root = new QVBoxLayout(window);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(14);

    auto addTarget = [&](const QString &name, QWidget *widget, QRect area = {}, qreal contrast = 3.0, qreal ratio = 0.006) {
        scene.targets.append({name, widget, area, contrast, ratio});
    };

    auto *labelsCard = makeCard(QStringLiteral("Text"), window);
    auto *labelsLayout = qobject_cast<QVBoxLayout *>(labelsCard->layout());
    auto *normalLabel = new FluentLabel(QStringLiteral("FluentLabel Normal"), labelsCard);
    auto *strongLabel = new FluentLabel(QStringLiteral("FluentLabel Strong"), labelsCard);
    QFont strongFont = strongLabel->font();
    strongFont.setWeight(QFont::Bold);
    strongLabel->setFont(strongFont);
    labelsLayout->addWidget(normalLabel);
    labelsLayout->addWidget(strongLabel);
    auto *tabs = new FluentTabWidget(labelsCard);
    tabs->setFixedHeight(104);
    auto *tabOne = new FluentLabel(QStringLiteral("Selected tab content"), tabs);
    auto *tabTwo = new FluentLabel(QStringLiteral("Hover tab content"), tabs);
    tabOne->setAlignment(Qt::AlignCenter);
    tabTwo->setAlignment(Qt::AlignCenter);
    tabs->addTab(tabOne, QStringLiteral("Overview"));
    tabs->addTab(tabTwo, QStringLiteral("Details"));
    labelsLayout->addWidget(tabs);
    scene.tabWidget = tabs;
    root->addWidget(labelsCard);
    addTarget(QStringLiteral("label-normal"), normalLabel);
    addTarget(QStringLiteral("label-strong"), strongLabel);

    auto *buttonCard = makeCard(QStringLiteral("Buttons and command surfaces"), window);
    auto *buttonLayout = qobject_cast<QVBoxLayout *>(buttonCard->layout());
    auto *buttonRow = new QHBoxLayout();
    auto *neutral = new FluentButton(QStringLiteral("Neutral"), buttonCard);
    auto *primary = new FluentButton(QStringLiteral("Primary"), buttonCard);
    primary->setPrimary(true);
    auto *disabled = new FluentButton(QStringLiteral("Disabled"), buttonCard);
    disabled->setEnabled(false);
    auto *checked = new FluentButton(QStringLiteral("Checked"), buttonCard);
    checked->setCheckable(true);
    checked->setChecked(true);
    for (auto *button : {neutral, primary, disabled, checked}) {
        button->setFixedSize(116, 36);
        buttonRow->addWidget(button);
    }
    buttonRow->addStretch(1);
    buttonLayout->addLayout(buttonRow);
    scene.hoverButton = neutral;
    addTarget(QStringLiteral("button-neutral"), neutral, buttonTextArea(neutral));
    addTarget(QStringLiteral("button-primary"), primary, buttonTextArea(primary));
    addTarget(QStringLiteral("button-disabled"), disabled, buttonTextArea(disabled), 1.65, 0.004);
    addTarget(QStringLiteral("button-checked"), checked, buttonTextArea(checked));

    auto *command = new FluentCommandBar(buttonCard);
    auto *newAction = command->addAction(QStringLiteral("New"));
    Q_UNUSED(newAction);
    QAction *editAction = command->addAction(QStringLiteral("Edit"));
    editAction->setEnabled(false);
    command->addSeparator();
    auto *exportMenu = new FluentMenu(QStringLiteral("Export"), command);
    exportMenu->addAction(QStringLiteral("PDF"));
    auto *exportAction = new QAction(QStringLiteral("Export"), command);
    exportAction->setMenu(exportMenu);
    command->addCommand(exportAction);
    command->setFixedHeight(44);
    buttonLayout->addWidget(command);

    auto *commandRow = new QHBoxLayout();
    auto *primaryDrop = new FluentDropDownButton(QStringLiteral("Accent menu"), buttonCard);
    primaryDrop->setPrimary(true);
    primaryDrop->addAction(QStringLiteral("First"));
    primaryDrop->addAction(QStringLiteral("Second"));
    auto *split = new FluentSplitButton(QStringLiteral("Split"), buttonCard);
    split->setPrimary(true);
    auto *splitAction = new QAction(QStringLiteral("Split"), split);
    split->setDefaultAction(splitAction);
    auto *splitMenu = new FluentMenu(split);
    splitMenu->addAction(QStringLiteral("Alternative"));
    split->setMenu(splitMenu);
    commandRow->addWidget(primaryDrop);
    commandRow->addWidget(split);
    commandRow->addStretch(1);
    buttonLayout->addLayout(commandRow);
    addTarget(QStringLiteral("dropdown-primary"), primaryDrop, buttonTextArea(primaryDrop));
    if (split->mainButton()) {
        addTarget(QStringLiteral("split-primary-main"), split->mainButton(), buttonTextArea(split->mainButton()));
    }
    auto *inputCard = makeCard(QStringLiteral("Inputs and selection"), window);
    auto *inputLayout = qobject_cast<QVBoxLayout *>(inputCard->layout());
    auto *inputRow = new QHBoxLayout();
    auto *edit = new FluentLineEdit(QStringLiteral("Focus text"), inputCard);
    edit->setFixedSize(180, 34);
    auto *disabledEdit = new FluentLineEdit(QStringLiteral("Disabled text"), inputCard);
    disabledEdit->setFixedSize(180, 34);
    disabledEdit->setEnabled(false);
    auto *combo = new FluentComboBox(inputCard);
    combo->addItems({QStringLiteral("Alpha"), QStringLiteral("Beta"), QStringLiteral("Gamma")});
    combo->setCurrentIndex(1);
    combo->setFixedSize(150, 34);
    auto *spin = new FluentSpinBox(inputCard);
    spin->setRange(0, 999);
    spin->setValue(128);
    spin->setFixedSize(120, 34);
    inputRow->addWidget(edit);
    inputRow->addWidget(disabledEdit);
    inputRow->addWidget(combo);
    inputRow->addWidget(spin);
    inputRow->addStretch(1);
    inputLayout->addLayout(inputRow);
    scene.focusEdit = edit;
    scene.comboBox = combo;
    addTarget(QStringLiteral("lineedit-normal"), edit, edit->rect().adjusted(12, 4, -12, -4));
    addTarget(QStringLiteral("lineedit-disabled"), disabledEdit, disabledEdit->rect().adjusted(12, 4, -12, -4), 1.65, 0.004);
    addTarget(QStringLiteral("combo-selected"), combo, combo->rect().adjusted(12, 4, -34, -4));
    addTarget(QStringLiteral("spinbox-value"), spin, spin->rect().adjusted(12, 4, -42, -4));

    auto *selectionRow = new QHBoxLayout();
    auto *toggle = new FluentToggleSwitch(QStringLiteral("Toggle"), inputCard);
    toggle->setChecked(true);
    auto *check = new FluentCheckBox(QStringLiteral("Check"), inputCard);
    check->setChecked(true);
    auto *radio = new FluentRadioButton(QStringLiteral("Radio"), inputCard);
    radio->setChecked(true);
    selectionRow->addWidget(toggle);
    selectionRow->addWidget(check);
    selectionRow->addWidget(radio);
    selectionRow->addStretch(1);
    inputLayout->addLayout(selectionRow);
    addTarget(QStringLiteral("toggle-checked-text"), toggle, toggle->rect().adjusted(54, 0, 0, 0));
    addTarget(QStringLiteral("checkbox-checked-text"), check, check->rect().adjusted(30, 0, 0, 0));
    addTarget(QStringLiteral("radio-checked-text"), radio, radio->rect().adjusted(30, 0, 0, 0));

    auto *textEdit = new FluentTextEdit(inputCard);
    textEdit->setPlainText(QStringLiteral("First line\nSecond fluent line\nThird line"));
    textEdit->setFixedHeight(96);
    inputLayout->addWidget(textEdit);
    scene.textEdit = textEdit;
    addTarget(QStringLiteral("textedit-content"), textEdit->viewport(), textEdit->viewport()->rect().adjusted(6, 4, -6, -4));

    auto *pickerRow = new QHBoxLayout();
    auto *calendar = new FluentCalendarPicker(inputCard);
    calendar->setDate(QDate(2026, 5, 26));
    calendar->setFixedSize(180, 34);
    auto *datePicker = new FluentDatePicker(inputCard);
    datePicker->setDate(QDate(2026, 5, 26));
    datePicker->setFixedSize(220, 34);
    auto *timePicker = new FluentTimePicker(inputCard);
    timePicker->setTime(QTime(14, 35));
    timePicker->setFixedSize(180, 34);
    auto *shortcut = new FluentKeySequenceEdit(QKeySequence(QStringLiteral("Ctrl+K")), inputCard);
    shortcut->setFixedSize(180, 34);
    pickerRow->addWidget(calendar);
    pickerRow->addWidget(datePicker);
    pickerRow->addWidget(timePicker);
    pickerRow->addWidget(shortcut);
    pickerRow->addStretch(1);
    inputLayout->addLayout(pickerRow);
    scene.calendarPicker = calendar;
    scene.datePicker = datePicker;
    scene.timePicker = timePicker;
    addTarget(QStringLiteral("calendar-picker-text"), calendar, calendar->rect().adjusted(12, 4, -38, -4));
    addTarget(QStringLiteral("date-picker-text"), datePicker, datePicker->rect().adjusted(12, 4, -38, -4));
    addTarget(QStringLiteral("time-picker-text"), timePicker, timePicker->rect().adjusted(12, 4, -38, -4));
    addTarget(QStringLiteral("keysequence-text"), shortcut, shortcut->rect().adjusted(12, 4, -12, -4));

    auto *progress = new FluentProgressBar(inputCard);
    progress->setValue(66);
    inputLayout->addWidget(progress);
    root->addWidget(buttonCard);
    root->addWidget(inputCard);

    auto *dataCard = makeCard(QStringLiteral("Data views and popups"), window);
    auto *dataLayout = qobject_cast<QVBoxLayout *>(dataCard->layout());
    auto *search = new FluentSearchBox(dataCard);
    search->setPlaceholderText(QStringLiteral("Search fluent controls"));
    search->setSuggestions({
        QStringLiteral("FluentAutoSuggestBox"),
        QStringLiteral("FluentComboBox"),
        QStringLiteral("FluentListView"),
        QStringLiteral("FluentTableView"),
        QStringLiteral("FluentTreeView")
    });
    dataLayout->addWidget(search);
    scene.searchBox = search;

    auto *list = new FluentListView(dataCard);
    auto *listModel = new QStringListModel({
        QStringLiteral("Overview"),
        QStringLiteral("Selected list item"),
        QStringLiteral("Hover candidate"),
        QStringLiteral("Disabled-like row label")
    }, list);
    list->setModel(listModel);
    list->setFixedHeight(120);
    const QModelIndex listSelected = listModel->index(1, 0);
    list->setCurrentIndex(listSelected);
    list->selectionModel()->select(listSelected, QItemSelectionModel::ClearAndSelect);
    dataLayout->addWidget(list);
    scene.listView = list;

    auto *table = new FluentTableView(dataCard);
    auto *tableModel = new QStandardItemModel(3, 2, table);
    tableModel->setHorizontalHeaderLabels({QStringLiteral("Name"), QStringLiteral("State")});
    tableModel->setItem(0, 0, new QStandardItem(QStringLiteral("Alpha")));
    tableModel->setItem(0, 1, new QStandardItem(QStringLiteral("Ready")));
    tableModel->setItem(1, 0, new QStandardItem(QStringLiteral("Beta")));
    tableModel->setItem(1, 1, new QStandardItem(QStringLiteral("Selected")));
    tableModel->setItem(2, 0, new QStandardItem(QStringLiteral("Gamma")));
    tableModel->setItem(2, 1, new QStandardItem(QStringLiteral("Idle")));
    table->setModel(tableModel);
    table->setFixedHeight(128);
    const QModelIndex tableSelected = tableModel->index(1, 0);
    table->setCurrentIndex(tableSelected);
    table->selectionModel()->select(
        tableModel->index(1, 0),
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    dataLayout->addWidget(table);
    scene.tableView = table;

    auto *tableWidget = new FluentTableWidget(3, 2, dataCard);
    tableWidget->setHorizontalHeaderLabels({QStringLiteral("Field"), QStringLiteral("Value")});
    tableWidget->setItem(0, 0, new QTableWidgetItem(QStringLiteral("One")));
    tableWidget->setItem(0, 1, new QTableWidgetItem(QStringLiteral("Ready")));
    tableWidget->setItem(1, 0, new QTableWidgetItem(QStringLiteral("Two")));
    tableWidget->setItem(1, 1, new QTableWidgetItem(QStringLiteral("Selected")));
    tableWidget->setItem(2, 0, new QTableWidgetItem(QStringLiteral("Three")));
    tableWidget->setItem(2, 1, new QTableWidgetItem(QStringLiteral("Idle")));
    tableWidget->setFixedHeight(128);
    tableWidget->setCurrentCell(1, 0);
    tableWidget->selectionModel()->select(
        tableWidget->model()->index(1, 0),
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    dataLayout->addWidget(tableWidget);
    scene.tableWidget = tableWidget;

    auto *tree = new FluentTreeView(dataCard);
    auto *treeModel = new QStandardItemModel(tree);
    treeModel->setHorizontalHeaderLabels({QStringLiteral("Section"), QStringLiteral("Value")});
    auto *rootItem = new QStandardItem(QStringLiteral("Controls"));
    auto *rootValue = new QStandardItem(QStringLiteral("Group"));
    rootItem->appendRow({
        new QStandardItem(QStringLiteral("Selected tree item")),
        new QStandardItem(QStringLiteral("Ready"))
    });
    rootItem->appendRow({
        new QStandardItem(QStringLiteral("Hover tree item")),
        new QStandardItem(QStringLiteral("Idle"))
    });
    treeModel->appendRow({rootItem, rootValue});
    tree->setModel(treeModel);
    tree->expandAll();
    tree->setFixedHeight(144);
    const QModelIndex treeSelected = treeModel->index(0, 0, rootItem->index());
    tree->setCurrentIndex(treeSelected);
    tree->selectionModel()->select(
        treeSelected,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    dataLayout->addWidget(tree);
    scene.treeView = tree;

    root->addWidget(dataCard);

    auto *info = new FluentInfoBar(FluentInfoBar::Severity::Warning,
                                   QStringLiteral("Warning"),
                                   QStringLiteral("Readable themed info bar text."),
                                   window);
    info->setActionText(QStringLiteral("Action"));
    root->addWidget(info);
    addTarget(QStringLiteral("infobar-warning-title-message"), info, {}, 2.4, 0.005);

    window->show();
    QCoreApplication::processEvents();
    QTest::qWait(60);
    QCoreApplication::processEvents();

    if (auto *editButton = findButtonByText(command, QStringLiteral("Edit"))) {
        addTarget(QStringLiteral("commandbar-disabled-edit"), editButton, buttonTextArea(editButton), 1.65, 0.004);
    }
    if (auto *newButton = findButtonByText(command, QStringLiteral("New"))) {
        addTarget(QStringLiteral("commandbar-new"), newButton, buttonTextArea(newButton));
    }
    if (auto *exportButton = findButtonByText(command, QStringLiteral("Export"))) {
        addTarget(QStringLiteral("commandbar-dropdown-export"), exportButton, buttonTextArea(exportButton));
    }
    if (scene.tabWidget && scene.tabWidget->tabBar() && scene.tabWidget->tabBar()->count() > 0) {
        QTabBar *bar = scene.tabWidget->tabBar();
        addTarget(QStringLiteral("tabwidget-current-tab"), bar, bar->tabRect(bar->currentIndex()).adjusted(12, 4, -12, -4));
    }
    if (scene.listView && scene.listView->model()) {
        const QModelIndex selected = scene.listView->model()->index(1, 0);
        const QModelIndex hover = scene.listView->model()->index(2, 0);
        addTarget(QStringLiteral("listview-selected"), scene.listView->viewport(), viewTextArea(scene.listView, selected, 14));
        addTarget(QStringLiteral("listview-hover-candidate"), scene.listView->viewport(), viewTextArea(scene.listView, hover, 14));
    }
    if (scene.tableView && scene.tableView->model()) {
        const QModelIndex selected = scene.tableView->model()->index(1, 0);
        addTarget(QStringLiteral("tableview-header"), scene.tableView->horizontalHeader()->viewport(),
                  headerTextArea(scene.tableView->horizontalHeader(), 0));
        addTarget(QStringLiteral("tableview-selected-row"), scene.tableView->viewport(),
                  viewTextArea(scene.tableView, selected, 14));
    }
    if (scene.tableWidget && scene.tableWidget->model()) {
        const QModelIndex selected = scene.tableWidget->model()->index(1, 0);
        addTarget(QStringLiteral("tablewidget-header"), scene.tableWidget->horizontalHeader()->viewport(),
                  headerTextArea(scene.tableWidget->horizontalHeader(), 0));
        addTarget(QStringLiteral("tablewidget-selected-row"), scene.tableWidget->viewport(),
                  viewTextArea(scene.tableWidget, selected, 14));
    }
    if (scene.treeView && scene.treeView->model()) {
        const QModelIndex selected = scene.treeView->model()->index(0, 0, scene.treeView->model()->index(0, 0));
        addTarget(QStringLiteral("treeview-header"), scene.treeView->header()->viewport(),
                  headerTextArea(scene.treeView->header(), 0));
        addTarget(QStringLiteral("treeview-selected-row"), scene.treeView->viewport(),
                  viewTextArea(scene.treeView, selected, 28));
    }
    if (scene.searchBox) {
        scene.searchBox->lineEdit()->setFocus(Qt::OtherFocusReason);
        scene.searchBox->setText(QStringLiteral("Fluent"));
        QCoreApplication::processEvents();
        QTest::qWait(180);
        QCoreApplication::processEvents();
        if (QListView *popupView = findAutoSuggestPopupView()) {
            const QModelIndex index = popupView->currentIndex();
            addTarget(QStringLiteral("autosuggest-popup-selected"), popupView->viewport(),
                      viewTextArea(popupView, index, 16));
        }
    }

    return scene;
}

} // namespace

class QtFluentWidgetsVisualSmokeTest final : public QObject
{
    Q_OBJECT

private slots:
    void visualContrast_data()
    {
        QTest::addColumn<bool>("dark");
        QTest::addColumn<QColor>("accent");
        QTest::newRow("light-default") << false << QColor(QStringLiteral("#0066B4"));
        QTest::newRow("dark-default") << true << QColor(QStringLiteral("#0066B4"));
        QTest::newRow("light-bright-accent") << false << QColor(QStringLiteral("#43FF43"));
        QTest::newRow("dark-bright-accent") << true << QColor(QStringLiteral("#43FF43"));
    }

    void visualContrast()
    {
        QFETCH(bool, dark);
        QFETCH(QColor, accent);
        syncTheme(dark, accent);

        const QString modeName = QString::fromLatin1(QTest::currentDataTag());
        std::unique_ptr<QWidget> window;
        SmokeScene scene = createSmokeScene();
        window.reset(scene.window);

        const QString scenePath = saveImage(modeName + QStringLiteral("_scene"), window->grab().toImage());
        qInfo().noquote() << QStringLiteral("[VisualSmoke] scene %1").arg(scenePath);
        showForInspection(window.get());

        QStringList failures;
        for (const CheckTarget &target : std::as_const(scene.targets)) {
            const QString failure = verifyReadableTarget(target, modeName);
            if (!failure.isEmpty()) {
                failures << failure;
            }
        }

        QVERIFY2(failures.isEmpty(), qPrintable(failures.join(QLatin1Char('\n'))));
    }

    void interactiveStatesStayReadable_data()
    {
        visualContrast_data();
    }

    void interactiveStatesStayReadable()
    {
        QFETCH(bool, dark);
        QFETCH(QColor, accent);
        syncTheme(dark, accent);

        const QString modeName = QString::fromLatin1(QTest::currentDataTag());
        std::unique_ptr<QWidget> window;
        SmokeScene scene = createSmokeScene();
        window.reset(scene.window);

        QStringList failures;
        if (scene.hoverButton) {
            QTest::mouseMove(scene.hoverButton, scene.hoverButton->rect().center());
            QTest::qWait(180);
            const QString failure = verifyReadableTarget(
                {QStringLiteral("hover-button-neutral"), scene.hoverButton, {}, 3.0, 0.006},
                modeName);
            if (!failure.isEmpty()) {
                failures << failure;
            }
        }

        if (scene.focusEdit) {
            scene.focusEdit->setFocus(Qt::TabFocusReason);
            QTest::qWait(240);
            const QString failure = verifyReadableTarget(
                {QStringLiteral("focus-lineedit"), scene.focusEdit, scene.focusEdit->rect().adjusted(12, 4, -12, -4), 3.0, 0.006},
                modeName);
            if (!failure.isEmpty()) {
                failures << failure;
            }
        }

        if (scene.textEdit) {
            scene.textEdit->setFocus(Qt::TabFocusReason);
            QTest::qWait(240);
            const QString failure = verifyReadableTarget(
                {QStringLiteral("focus-textedit"), scene.textEdit->viewport(), scene.textEdit->viewport()->rect().adjusted(6, 4, -6, -4), 3.0, 0.006},
                modeName);
            if (!failure.isEmpty()) {
                failures << failure;
            }
        }

        if (scene.listView && scene.listView->model()) {
            const QModelIndex hoverIndex = scene.listView->model()->index(2, 0);
            const QRect hoverRect = scene.listView->visualRect(hoverIndex);
            if (hoverRect.isValid()) {
                QTest::mouseMove(scene.listView->viewport(), hoverRect.center());
                QTest::qWait(180);
                const QString failure = verifyReadableTarget(
                    {QStringLiteral("hover-listview-row"), scene.listView->viewport(), viewTextArea(scene.listView, hoverIndex, 14), 3.0, 0.006},
                    modeName);
                if (!failure.isEmpty()) {
                    failures << failure;
                }
            }
        }

        if (scene.tabWidget && scene.tabWidget->tabBar() && scene.tabWidget->tabBar()->count() > 1) {
            QTabBar *bar = scene.tabWidget->tabBar();
            const QRect hoverRect = bar->tabRect(1);
            if (hoverRect.isValid()) {
                QTest::mouseMove(bar, hoverRect.center());
                QTest::qWait(180);
                const QString failure = verifyReadableTarget(
                    {QStringLiteral("hover-tabwidget-tab"), bar, hoverRect.adjusted(12, 4, -12, -4), 3.0, 0.006},
                    modeName);
                if (!failure.isEmpty()) {
                    failures << failure;
                }
            }
        }

        QVERIFY2(failures.isEmpty(), qPrintable(failures.join(QLatin1Char('\n'))));
    }

    void popupSurfacesStayReadable_data()
    {
        visualContrast_data();
    }

    void popupSurfacesStayReadable()
    {
        QFETCH(bool, dark);
        QFETCH(QColor, accent);
        syncTheme(dark, accent);

        const QString modeName = QString::fromLatin1(QTest::currentDataTag());
        std::unique_ptr<QWidget> window;
        SmokeScene scene = createSmokeScene();
        window.reset(scene.window);

        QStringList failures;

        if (scene.comboBox) {
            scene.comboBox->showPopup();
            QCoreApplication::processEvents();
            QTest::qWait(220);
            QCoreApplication::processEvents();

            if (QListView *comboPopup = findComboPopupView()) {
                const QModelIndex index = comboPopup->currentIndex();
                const QString failure = verifyReadableTarget(
                    {QStringLiteral("combobox-popup-current"), comboPopup->viewport(), viewTextArea(comboPopup, index, 16), 3.0, 0.006},
                    modeName);
                if (!failure.isEmpty()) {
                    failures << failure;
                }
            } else {
                failures << QStringLiteral("%1/combobox popup was not visible").arg(modeName);
            }
            scene.comboBox->hidePopup();
            QCoreApplication::processEvents();
        }

        if (scene.calendarPicker) {
            const QPoint arrowPoint(scene.calendarPicker->width() - 12,
                                    scene.calendarPicker->height() / 2);
            QTest::mouseClick(scene.calendarPicker, Qt::LeftButton, Qt::NoModifier, arrowPoint);
            QCoreApplication::processEvents();
            QTest::qWait(240);
            QCoreApplication::processEvents();

            if (QWidget *calendarPopup = findVisibleTopLevelByObjectName(QStringLiteral("FluentCalendarPopup"))) {
                const QRect selectedDateArea = calendarPopupDateCellArea(
                    calendarPopup,
                    scene.calendarPicker->date(),
                    scene.calendarPicker->locale().firstDayOfWeek());
                const QString failure = verifyReadableTarget(
                    {QStringLiteral("calendar-popup-selected-date"), calendarPopup, selectedDateArea, 3.0, 0.004},
                    modeName);
                if (!failure.isEmpty()) {
                    failures << failure;
                }
                calendarPopup->close();
            } else {
                failures << QStringLiteral("%1/calendar popup was not visible").arg(modeName);
            }
            QCoreApplication::processEvents();
        }

        auto openWheelPickerAndVerify = [&](QWidget *picker, const QString &prefix) {
            if (!picker) {
                return;
            }

            QTest::mouseClick(picker, Qt::LeftButton, Qt::NoModifier, picker->rect().center());
            QCoreApplication::processEvents();
            QTest::qWait(240);
            QCoreApplication::processEvents();

            QWidget *popup = findVisibleTopLevelByObjectName(QStringLiteral("FluentWheelPickerPopup"));
            failures << verifyWheelPickerPopupColumns(popup, prefix, modeName);
            if (popup) {
                popup->close();
            }
            QCoreApplication::processEvents();
        };

        openWheelPickerAndVerify(scene.datePicker, QStringLiteral("date-picker-popup"));
        openWheelPickerAndVerify(scene.timePicker, QStringLiteral("time-picker-popup"));

        auto *menu = new FluentMenu(window.get());
        menu->addAction(QStringLiteral("Open"));
        auto *checked = menu->addAction(QStringLiteral("Checked item"));
        checked->setCheckable(true);
        checked->setChecked(true);
        auto *disabled = menu->addAction(QStringLiteral("Disabled item"));
        disabled->setEnabled(false);
        auto *submenu = menu->addFluentMenu(QStringLiteral("More"));
        submenu->addAction(QStringLiteral("Child command"));
        menu->popup(window->mapToGlobal(QPoint(24, 24)));
        QCoreApplication::processEvents();
        QTest::qWait(180);
        QCoreApplication::processEvents();

        if (QWidget *popup = findVisibleTopLevelByObjectName(QStringLiteral("FluentMenuPopupHost"))) {
            const QRect content = PopupSurface::shadowContentRect(popup->rect());
            QTest::mouseMove(popup, QPoint(content.left() + 24, content.top() + 4 + 17));
            QTest::qWait(140);
            const QString hoverFailure = verifyReadableTarget(
                {QStringLiteral("menu-popup-hover-open"), popup, popupMenuTextArea(popup, 0), 3.0, 0.006},
                modeName);
            if (!hoverFailure.isEmpty()) {
                failures << hoverFailure;
            }

            const QString checkedFailure = verifyReadableTarget(
                {QStringLiteral("menu-popup-checked-item"), popup, popupMenuTextArea(popup, 1), 3.0, 0.006},
                modeName);
            if (!checkedFailure.isEmpty()) {
                failures << checkedFailure;
            }

            const QString disabledFailure = verifyReadableTarget(
                {QStringLiteral("menu-popup-disabled-item"), popup, popupMenuTextArea(popup, 2), 1.65, 0.004},
                modeName);
            if (!disabledFailure.isEmpty()) {
                failures << disabledFailure;
            }
            popup->close();
        } else {
            failures << QStringLiteral("%1/menu popup was not visible").arg(modeName);
        }

        QVERIFY2(failures.isEmpty(), qPrintable(failures.join(QLatin1Char('\n'))));
    }

    void motionTokensCanDisablePopupAnimation()
    {
        syncTheme(false, QColor(QStringLiteral("#0066B4")));

        const bool oldAnimationsEnabled = ThemeManager::instance().animationsEnabled();
        const FluentMotionTokens oldMotionTokens = ThemeManager::instance().motionTokens();
        struct AnimationRestore {
            bool enabled = true;
            FluentMotionTokens tokens;
            ~AnimationRestore()
            {
                ThemeManager::instance().setMotionTokens(tokens);
                ThemeManager::instance().setAnimationsEnabled(enabled);
                QCoreApplication::processEvents();
            }
        } restore{oldAnimationsEnabled, oldMotionTokens};

        ThemeManager::instance().setAnimationsEnabled(true);
        QCoreApplication::processEvents();

        QWidget window;
        auto *layout = new QVBoxLayout(&window);
        auto *button = new FluentButton(QStringLiteral("Button"), &window);
        auto *toolButton = new FluentToolButton(QStringLiteral("Tool"), &window);
        auto *edit = new FluentLineEdit(&window);
        auto *toggle = new FluentToggleSwitch(QStringLiteral("Toggle"), &window);
        auto *check = new FluentCheckBox(QStringLiteral("Check"), &window);
        auto *radio = new FluentRadioButton(QStringLiteral("Radio"), &window);
        auto *slider = new FluentSlider(Qt::Horizontal, &window);
        auto *progress = new FluentProgressBar(&window);
        auto *ring = new FluentProgressRing(&window);
        auto *combo = new FluentComboBox(&window);
        combo->addItems({QStringLiteral("Alpha"), QStringLiteral("Beta"), QStringLiteral("Gamma")});
        combo->setCurrentIndex(1);
        slider->setRange(0, 100);
        progress->setRange(0, 100);
        ring->setRange(0, 100);
        layout->addWidget(button);
        layout->addWidget(toolButton);
        layout->addWidget(edit);
        layout->addWidget(toggle);
        layout->addWidget(check);
        layout->addWidget(radio);
        layout->addWidget(slider);
        layout->addWidget(progress);
        layout->addWidget(ring);
        layout->addWidget(combo);
        window.show();
        QCoreApplication::processEvents();

        ThemeManager::instance().setAnimationsEnabled(false);
        QCoreApplication::processEvents();

        QCOMPARE(FluentMotion::duration(FluentMotionRole::PopupOpen), 0);
        QCOMPARE(FluentMotion::duration(FluentMotionRole::Hover), 0);

        QTest::mouseMove(button, button->rect().center());
        QCoreApplication::processEvents();
        QVERIFY2(button->hoverLevel() >= 0.99, "Button hover animation did not reconfigure to complete immediately");

        QTest::mouseMove(toolButton, toolButton->rect().center());
        QCoreApplication::processEvents();
        QVERIFY2(toolButton->hoverLevel() >= 0.99, "ToolButton hover animation did not reconfigure to complete immediately");

        edit->setFocus(Qt::OtherFocusReason);
        QCoreApplication::processEvents();
        QVERIFY2(edit->focusLevel() >= 0.99, "LineEdit focus animation did not reconfigure to complete immediately");

        QTest::mouseMove(combo, combo->rect().center());
        QCoreApplication::processEvents();
        QVERIFY2(combo->hoverLevel() >= 0.99, "ComboBox hover animation did not reconfigure to complete immediately");

        toggle->setChecked(true);
        QCOMPARE(toggle->progress(), 1.0);
        toggle->setChecked(false);
        QCOMPARE(toggle->progress(), 0.0);

        QTest::mouseMove(check, check->rect().center());
        QCoreApplication::processEvents();
        QVERIFY2(check->hoverLevel() >= 0.99, "CheckBox hover animation did not complete immediately");
        check->setFocus(Qt::OtherFocusReason);
        QCoreApplication::processEvents();
        QVERIFY2(check->focusLevel() >= 0.99, "CheckBox focus animation did not complete immediately");

        QTest::mouseMove(radio, radio->rect().center());
        QCoreApplication::processEvents();
        QVERIFY2(radio->hoverLevel() >= 0.99, "RadioButton hover animation did not complete immediately");
        radio->setFocus(Qt::OtherFocusReason);
        QCoreApplication::processEvents();
        QVERIFY2(radio->focusLevel() >= 0.99, "RadioButton focus animation did not complete immediately");

        slider->setValue(75);
        QCOMPARE(slider->handlePos(), 0.75);
        progress->setValue(66);
        QCOMPARE(progress->displayValue(), 66.0);
        ring->setValue(40);
        QCOMPARE(ring->displayValue(), 40.0);

        ring->setIndeterminate(true);
        const qreal stableAngle = ring->rotationAngle();
        QTest::qWait(50);
        QCOMPARE(ring->rotationAngle(), stableAngle);

        combo->showPopup();
        QCoreApplication::processEvents();

        QWidget *popup = findVisibleTopLevelByObjectName(QStringLiteral("FluentComboPopup"));
        QVERIFY2(popup && popup->isVisible(), "ComboBox popup was not visible with animations disabled");
        QCOMPARE(popup->windowOpacity(), 1.0);
        QVERIFY2(popup->mask().isEmpty(), "ComboBox popup should finish reveal mask immediately with animations disabled");
        QVERIFY2(findComboPopupView(), "ComboBox popup view was not available with animations disabled");

        combo->hidePopup();
        QCoreApplication::processEvents();
    }

    void motionTokensCanConfigureDurations()
    {
        syncTheme(false, QColor(QStringLiteral("#0066B4")));

        const bool oldAnimationsEnabled = ThemeManager::instance().animationsEnabled();
        const FluentMotionTokens oldMotionTokens = ThemeManager::instance().motionTokens();
        struct Restore {
            bool enabled = true;
            FluentMotionTokens tokens;
            ~Restore()
            {
                ThemeManager::instance().setMotionTokens(tokens);
                ThemeManager::instance().setAnimationsEnabled(enabled);
                QCoreApplication::processEvents();
            }
        } restore{oldAnimationsEnabled, oldMotionTokens};

        ThemeManager::instance().setAnimationsEnabled(true);
        QCoreApplication::processEvents();

        FluentMotionTokens custom = oldMotionTokens;
        custom.hoverDuration = 17;
        custom.popupOpenDuration = 23;
        custom.selectionDuration = 31;
        custom.layoutDuration = 37;
        custom.collapseDuration = -5;
        custom.popupSlideOffset = -2;
        ThemeManager::instance().setMotionTokens(custom);
        QCoreApplication::processEvents();

        QCOMPARE(ThemeManager::instance().motionTokens().hoverDuration, 17);
        QCOMPARE(ThemeManager::instance().motionTokens().popupOpenDuration, 23);
        QCOMPARE(ThemeManager::instance().motionTokens().selectionDuration, 31);
        QCOMPARE(ThemeManager::instance().motionTokens().layoutDuration, 37);
        QCOMPARE(ThemeManager::instance().motionTokens().collapseDuration, 0);
        QCOMPARE(ThemeManager::instance().motionTokens().popupSlideOffset, 0);
        QCOMPARE(FluentMotion::duration(FluentMotionRole::Hover), 17);
        QCOMPARE(FluentMotion::configuredDuration(FluentMotionRole::PopupOpen), 23);
        QCOMPARE(FluentMotion::duration(FluentMotionRole::Layout), 37);

        QWidget flowHost;
        auto *flow = new FluentFlowLayout(&flowHost);
        QCOMPARE(flow->animationDuration(), 37);
        flow->setAnimationDuration(52);
        QCOMPARE(flow->animationDuration(), 52);

        FluentMotion::setDuration(FluentMotionRole::Press, 44);
        QCoreApplication::processEvents();
        QCOMPARE(ThemeManager::instance().motionTokens().pressDuration, 44);
        QCOMPARE(FluentMotion::duration(FluentMotionRole::Press), 44);

        ThemeManager::instance().setThemeMode(ThemeManager::ThemeMode::Dark);
        QCoreApplication::processEvents();
        QCOMPARE(ThemeManager::instance().motionTokens().pressDuration, 44);
        QCOMPARE(FluentMotion::duration(FluentMotionRole::Press), 44);

        ThemeManager::instance().setAnimationsEnabled(false);
        QCoreApplication::processEvents();
        QCOMPARE(FluentMotion::configuredDuration(FluentMotionRole::Press), 44);
        QCOMPARE(FluentMotion::duration(FluentMotionRole::Press), 0);
    }

    void collapsibleCardContentAddedAfterEnableStaysVisible()
    {
        syncTheme(false, QColor(QStringLiteral("#0066B4")));

        QWidget window;
        auto *layout = new QVBoxLayout(&window);
        auto *card = new FluentCard(&window);
        card->setCollapsible(true);
        card->setTitle(QStringLiteral("Expandable card"));
        card->setCollapsed(false);
        card->contentLayout()->addWidget(new FluentLabel(QStringLiteral("First content row"), card));
        card->contentLayout()->addWidget(new FluentLabel(QStringLiteral("Second content row"), card));
        layout->addWidget(card);

        window.resize(360, 180);
        window.show();
        QCoreApplication::processEvents();
        QTest::qWait(20);
        QCoreApplication::processEvents();

        QWidget *contentClip = card->findChild<QWidget *>(QStringLiteral("FluentCardContentClip"));
        QVERIFY2(contentClip, "Collapsible card did not create the content clip");
        QVERIFY2(contentClip->isVisible(), "Expanded collapsible card content clip is hidden");
        QVERIFY2(contentClip->height() > 0, "Expanded collapsible card content clip kept a zero height after content was added");
        QVERIFY2(card->contentWidget()->height() > 0, "Expanded collapsible card content widget kept a zero height");
        const auto labels = card->contentWidget()->findChildren<FluentLabel *>();
        QVERIFY2(labels.size() >= 2, "Expanded collapsible card lost content labels after reparenting");
        QVERIFY2(labels.first()->isVisible(), "Expanded collapsible card content label is hidden");
        QVERIFY2(labels.first()->geometry().height() > 0, "Expanded collapsible card content label has no visible height");

        auto *collapsedCard = new FluentCard(&window);
        collapsedCard->setCollapsible(true);
        collapsedCard->setTitle(QStringLiteral("Initially collapsed card"));
        collapsedCard->setCollapsed(true);
        collapsedCard->setCollapseAnimationEnabled(false);
        collapsedCard->contentLayout()->addWidget(new FluentLabel(QStringLiteral("Collapsed card first row"), collapsedCard));
        collapsedCard->contentLayout()->addWidget(new FluentLabel(QStringLiteral("Collapsed card second row"), collapsedCard));
        layout->addWidget(collapsedCard);

        QCoreApplication::processEvents();
        QTest::qWait(20);
        collapsedCard->setCollapsed(false);
        QCoreApplication::processEvents();
        QTest::qWait(20);
        QCoreApplication::processEvents();

        QWidget *collapsedContentClip = collapsedCard->findChild<QWidget *>(QStringLiteral("FluentCardContentClip"));
        QVERIFY2(collapsedContentClip, "Initially collapsed card did not create the content clip");
        QVERIFY2(collapsedContentClip->isVisible(), "Initially collapsed card did not show the clip after expanding");
        QVERIFY2(collapsedContentClip->height() > 0, "Initially collapsed card expanded to a zero-height content clip");
        QVERIFY2(collapsedCard->contentWidget()->height() > 0, "Initially collapsed card expanded to a zero-height content widget");
    }

    void documentTabModeUsesIntegratedTabStrip()
    {
        syncTheme(false, QColor(QStringLiteral("#0066B4")));

        QWidget window;
        auto *layout = new QVBoxLayout(&window);
        layout->setContentsMargins(12, 12, 12, 12);

        auto *tabs = new FluentTabWidget(&window);
        auto *addButton = new FluentToolButton(tabs);
        tabs->setTabDisplayMode(FluentTabWidget::TabDisplayMode::Document);
        tabs->setTabsClosable(true);
        tabs->setMovable(true);
        tabs->setFixedSize(420, 128);
        QCOMPARE(tabs->contentMargin(), 5);
        tabs->setContentMargin(7);
        QCOMPARE(tabs->contentMargin(), 7);
        tabs->addTab(new FluentLabel(QStringLiteral("Desktop page"), tabs), FluentIcon::icon(FluentIconType::Folder), QStringLiteral("Desktop"));
        tabs->addTab(new FluentLabel(QStringLiteral("Documents page"), tabs), FluentIcon::icon(FluentIconType::Document), QStringLiteral("Documents"));
        QCOMPARE(tabs->widget(0)->contentsMargins(), QMargins(7, 7, 7, 7));
        QCOMPARE(tabs->widget(1)->contentsMargins(), QMargins(7, 7, 7, 7));
        addButton->setIcon(FluentIcon::icon(FluentIconType::Add));
        addButton->setFixedSize(34, 34);
        tabs->setCornerWidget(addButton, Qt::TopRightCorner);

        layout->addWidget(tabs);
        window.show();
        QTRY_VERIFY(window.isVisible());
        QTest::qWait(80);
        QCoreApplication::processEvents();

        QTabBar *bar = tabs->tabBar();
        QVERIFY(bar);
        QVERIFY2(!bar->tabButton(0, QTabBar::RightSide),
                 "Document mode should not expose Qt's native tab close button");

        const QRect barRect = bar->geometry();
        const QRect lastTab = bar->tabRect(bar->count() - 1);
        QVERIFY(lastTab.isValid());
        QVERIFY2(addButton->geometry().left() >= barRect.left() + lastTab.right(),
                 "Document add button should be positioned beside the last tab");
        QVERIFY2(addButton->geometry().left() < barRect.right(),
                 "Document add button should stay inside the tab strip");

        QSignalSpy closeSpy(tabs, &QTabWidget::tabCloseRequested);
        const QRect firstTab = bar->tabRect(0);
        QVERIFY(firstTab.isValid());
        QTest::mouseClick(bar,
                          Qt::LeftButton,
                          Qt::NoModifier,
                          QPoint(firstTab.right() - 18, firstTab.center().y()));
        QCOMPARE(closeSpy.count(), 1);
        QCOMPARE(closeSpy.takeFirst().at(0).toInt(), 0);
    }

    void commandBarMovesTrailingActionsToOverflow()
    {
        syncTheme(false, QColor(QStringLiteral("#0066B4")));

        QWidget window;
        auto *layout = new QVBoxLayout(&window);
        layout->setContentsMargins(12, 12, 12, 12);

        auto *bar = new FluentCommandBar(&window);
        bar->setFixedWidth(230);
        bar->addAction(QStringLiteral("New file"));
        bar->addAction(QStringLiteral("Rename"));
        bar->addSeparator();
        bar->addAction(QStringLiteral("Delete"));
        bar->addAction(QStringLiteral("Share"));
        bar->addAction(QStringLiteral("Properties"));
        layout->addWidget(bar);

        window.show();
        QCoreApplication::processEvents();
        QTRY_VERIFY(window.isVisible());

        auto *overflow = bar->findChild<FluentToolButton *>(QStringLiteral("FluentCommandBarOverflowButton"));
        QVERIFY2(overflow, "CommandBar did not create an overflow button");
        QTRY_VERIFY2(overflow->isVisible(), "CommandBar overflow button did not become visible when commands exceed width");

        QTest::mouseClick(overflow, Qt::LeftButton, Qt::NoModifier, overflow->rect().center());
        QCoreApplication::processEvents();
        QTest::qWait(180);
        QCoreApplication::processEvents();
        QVERIFY2(findVisibleTopLevelByObjectName(QStringLiteral("FluentMenuPopupHost")),
                 "CommandBar overflow should open a FluentMenu popup");

        if (QWidget *popup = findVisibleTopLevelByObjectName(QStringLiteral("FluentMenuPopupHost"))) {
            popup->close();
        }

        bar->setFixedWidth(720);
        QCoreApplication::processEvents();
        QTRY_VERIFY2(!overflow->isVisible(), "CommandBar overflow button should hide again when there is enough width");
    }

    void teachingTipDoesNotRequestStaleGeometry()
    {
        syncTheme(false, QColor(QStringLiteral("#0066B4")));

        QStringList warnings;
        g_teachingTipGeometryWarnings = &warnings;
        g_previousMessageHandler = qInstallMessageHandler(captureTeachingTipGeometryWarnings);

        QWidget window;
        window.resize(640, 420);
        auto *layout = new QVBoxLayout(&window);
        layout->setContentsMargins(32, 32, 32, 32);
        auto *button = new FluentButton(QStringLiteral("Tip target"), &window);
        layout->addWidget(button);
        layout->addStretch(1);

        window.show();
        QCoreApplication::processEvents();
        QTest::qWait(80);

        auto *tip = new FluentTeachingTip(&window);
        tip->setTitle(QStringLiteral("New feature"));
        tip->setSubtitle(QStringLiteral("TeachingTip explains a target control."));
        tip->setActionText(QStringLiteral("Start"));
        tip->setTarget(button);
        tip->open();
        QCoreApplication::processEvents();
        QTest::qWait(260);
        QCoreApplication::processEvents();
        tip->hide();
        tip->deleteLater();

        auto *guidedTip = new FluentTeachingTip(&window);
        auto *secondTarget = new FluentToggleSwitch(QStringLiteral("Second target"), &window);
        secondTarget->setChecked(true);
        layout->insertWidget(1, secondTarget);
        auto *customContent = new FluentLabel(QStringLiteral("Custom content widget with enough text to exercise wrapped height during a guided TeachingTip step."), guidedTip);
        customContent->setWordWrap(true);
        guidedTip->setGuideTargets({button, secondTarget, button});
        guidedTip->setGuideStyles({
            {QStringLiteral("1 / 3 Entry"), QStringLiteral("First guided node."), QStringLiteral("Next")},
            {QStringLiteral("2 / 3 Details"), QStringLiteral("Second guided node uses previous action and a custom content widget."), QStringLiteral("Next"), QStringLiteral("Back")},
            {QStringLiteral("3 / 3 Finish"), QStringLiteral("Final guided node."), QStringLiteral("Done"), QStringLiteral("Back")}
        });
        guidedTip->setGuideContentWidgets({nullptr, customContent, nullptr});
        guidedTip->setMaskEnabled(true);
        guidedTip->setMaskOpacity(0.46);

        auto tipButtonIsHitTestable = [](QAbstractButton *button) {
            if (!button || !button->isVisible()) {
                return false;
            }
            QWidget *hit = QApplication::widgetAt(button->mapToGlobal(button->rect().center()));
            return hit == button || (hit && button->isAncestorOf(hit));
        };
        auto clickThroughHitTest = [](QAbstractButton *button) {
            if (!button || !button->isVisible()) {
                return false;
            }
            const QPoint globalCenter = button->mapToGlobal(button->rect().center());
            QWidget *hit = QApplication::widgetAt(globalCenter);
            if (!hit || (hit != button && !button->isAncestorOf(hit))) {
                return false;
            }
            QTest::mouseClick(hit, Qt::LeftButton, Qt::NoModifier, hit->mapFromGlobal(globalCenter));
            return true;
        };
        guidedTip->startGuide();
        QCoreApplication::processEvents();
        QTest::qWait(220);
        auto *firstNext = findButtonByText(guidedTip, QStringLiteral("Next"));
        QVERIFY2(tipButtonIsHitTestable(firstNext), "TeachingTip first-step Next button is blocked by overlay/mask");
        QVERIFY2(clickThroughHitTest(firstNext), "TeachingTip first-step Next button cannot be clicked through real hit testing");
        QCOMPARE(guidedTip->currentGuideIndex(), 1);
        QCoreApplication::processEvents();
        QTest::qWait(220);
        auto *secondBack = findButtonByText(guidedTip, QStringLiteral("Back"));
        auto *secondNext = findButtonByText(guidedTip, QStringLiteral("Next"));
        QVERIFY2(tipButtonIsHitTestable(secondBack), "TeachingTip second-step Back button is blocked by overlay/mask");
        QVERIFY2(tipButtonIsHitTestable(secondNext), "TeachingTip second-step Next button is blocked by overlay/mask");
        QVERIFY2(clickThroughHitTest(secondNext), "TeachingTip second-step Next button cannot be clicked through real hit testing");
        QCOMPARE(guidedTip->currentGuideIndex(), 2);
        QCoreApplication::processEvents();
        QTest::qWait(220);
        guidedTip->finishGuide();
        guidedTip->deleteLater();

        qInstallMessageHandler(g_previousMessageHandler);
        g_previousMessageHandler = nullptr;
        g_teachingTipGeometryWarnings = nullptr;

        QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QLatin1Char('\n'))));
    }
};

QTEST_MAIN(QtFluentWidgetsVisualSmokeTest)
#include "QtFluentWidgetsVisualSmokeTest.moc"
