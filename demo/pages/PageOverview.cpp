#include "PageOverview.h"

#include "../DemoHelpers.h"
#include "../DemoCodeEditorSettings.h"

#include <QAction>
#include <QAbstractItemView>
#include <QCursor>
#include <QDate>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStringList>
#include <QStringListModel>
#include <QTime>
#include <QVBoxLayout>

#include "Fluent/FluentAnimatedButton.h"
#include "Fluent/FluentAutoSuggestBox.h"
#include "Fluent/FluentButton.h"
#include "Fluent/FluentAnnotatedScrollBar.h"
#include "Fluent/FluentCalendarPicker.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentCheckBox.h"
#include "Fluent/FluentColorDialog.h"
#include "Fluent/FluentColorPicker.h"
#include "Fluent/FluentCommandBar.h"
#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentDatePicker.h"
#include "Fluent/FluentDialog.h"
#include "Fluent/FluentDropDownButton.h"
#include "Fluent/FluentFlowLayout.h"
#include "Fluent/FluentFlyout.h"
#include "Fluent/FluentGroupBox.h"
#include "Fluent/FluentInfoBar.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentLineEdit.h"
#include "Fluent/FluentListView.h"
#include "Fluent/FluentLottieWidget.h"
#include "Fluent/FluentMainWindow.h"
#include "Fluent/FluentMenu.h"
#include "Fluent/FluentMessageBox.h"
#include "Fluent/FluentProgressBar.h"
#include "Fluent/FluentProgressRing.h"
#include "Fluent/FluentRadioButton.h"
#include "Fluent/FluentScrollArea.h"
#include "Fluent/FluentScrollBar.h"
#include "Fluent/FluentSlider.h"
#include "Fluent/FluentSpinBox.h"
#include "Fluent/FluentSplitButton.h"
#include "Fluent/FluentTableView.h"
#include "Fluent/FluentTabWidget.h"
#include "Fluent/FluentCodeEditor.h"
#include "Fluent/FluentTextEdit.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentTeachingTip.h"
#include "Fluent/FluentTimePicker.h"
#include "Fluent/FluentToast.h"
#include "Fluent/FluentToggleSwitch.h"
#include "Fluent/FluentToolButton.h"
#include "Fluent/FluentTreeView.h"
#include "Fluent/FluentWidget.h"

namespace Demo::Pages {

using namespace Fluent;

QWidget *createOverviewPage(FluentMainWindow *window, const std::function<void(int)> &jumpTo)
{
    auto *overviewArea = new FluentScrollArea();
    overviewArea->setOverlayScrollBarsEnabled(true);
    overviewArea->setFrameShape(QFrame::NoFrame);

    auto *overviewContent = new FluentWidget();
    overviewContent->setBackgroundRole(FluentWidget::BackgroundRole::WindowBackground);
    overviewArea->setWidget(overviewContent);

    auto *overviewLayout = new QVBoxLayout(overviewContent);
    overviewLayout->setContentsMargins(24, 24, 24, 24);
    overviewLayout->setSpacing(14);

    {
        auto *hero = new FluentCard();
        hero->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        auto *heroLayout = new QHBoxLayout(hero);
        heroLayout->setContentsMargins(24, 22, 24, 22);
        heroLayout->setSpacing(24);

        auto *copy = new FluentWidget();
        copy->setBackgroundRole(FluentWidget::BackgroundRole::Transparent);
        auto *copyLayout = new QVBoxLayout(copy);
        copyLayout->setContentsMargins(0, 0, 0, 0);
        copyLayout->setSpacing(10);

        auto *eyebrow = new FluentLabel(DEMO_TEXT("Qt Widgets Fluent Design 组件库", "Qt Widgets Fluent Design component library"));
        eyebrow->setStyleSheet("font-size: 12px; font-weight: 600; opacity: 0.82;");
        auto *title = new FluentLabel(QStringLiteral("Qt Fluent"));
        title->setStyleSheet("font-size: 28px; font-weight: 700;");
        auto *subtitle = new FluentLabel(DEMO_TEXT("统一的主题 token、图标、动效、弹层和控件状态，组成一套可直接落地的桌面 UI 组件库。",
                                                   "A desktop UI component library with unified theme tokens, icons, motion, popups, and control states."));
        subtitle->setWordWrap(true);
        subtitle->setStyleSheet("font-size: 13px; opacity: 0.88;");

        auto *quickRow = new QHBoxLayout();
        quickRow->setContentsMargins(0, 4, 0, 0);
        quickRow->setSpacing(8);
        auto *openBasic = new FluentButton(DEMO_TEXT("控件画廊", "Control gallery"));
        openBasic->setPrimary(true);
        auto *openMotion = new FluentButton(DEMO_TEXT("主题与动效", "Theme & motion"));
        auto *openIcons = new FluentButton(DEMO_TEXT("图标", "Icons"));
        auto *openPopups = new FluentButton(DEMO_TEXT("弹层/窗口", "Popups & windows"));
        QObject::connect(openBasic, &QPushButton::clicked, window, [jumpTo]() { jumpTo(1); });
        QObject::connect(openMotion, &QPushButton::clicked, window, [jumpTo]() { jumpTo(5); });
        QObject::connect(openIcons, &QPushButton::clicked, window, [jumpTo]() { jumpTo(4); });
        QObject::connect(openPopups, &QPushButton::clicked, window, [jumpTo]() { jumpTo(10); });
        quickRow->addWidget(openBasic);
        quickRow->addWidget(openMotion);
        quickRow->addWidget(openIcons);
        quickRow->addWidget(openPopups);
        quickRow->addStretch(1);

        copyLayout->addWidget(eyebrow);
        copyLayout->addWidget(title);
        copyLayout->addWidget(subtitle);
        copyLayout->addLayout(quickRow);
        copyLayout->addStretch(1);

        auto *preview = new FluentWidget();
        preview->setBackgroundRole(FluentWidget::BackgroundRole::Transparent);
        preview->setMinimumWidth(360);
        auto *previewLayout = new QVBoxLayout(preview);
        previewLayout->setContentsMargins(0, 0, 0, 0);
        previewLayout->setSpacing(10);

        auto *inputRow = new QHBoxLayout();
        inputRow->setContentsMargins(0, 0, 0, 0);
        inputRow->setSpacing(8);
        auto *searchPreview = new FluentSearchBox();
        searchPreview->setPlaceholderText(DEMO_TEXT("搜索控件", "Search controls"));
        searchPreview->setSuggestions({QStringLiteral("FluentButton"),
                                        QStringLiteral("FluentTabWidget"),
                                        QStringLiteral("FluentTeachingTip")});
        auto *accentButton = new FluentButton(DEMO_TEXT("Primary", "Primary"));
        accentButton->setPrimary(true);
        inputRow->addWidget(searchPreview, 1);
        inputRow->addWidget(accentButton);

        auto *command = new FluentCommandBar();
        command->addAction(DEMO_TEXT("新建", "New"));
        command->addAction(DEMO_TEXT("编辑", "Edit"));
        command->addSeparator();
        auto *previewMenu = new FluentMenu(DEMO_TEXT("导出", "Export"), command);
        previewMenu->addAction(QStringLiteral("PDF"));
        previewMenu->addAction(QStringLiteral("PNG"));
        auto *previewAction = new QAction(DEMO_TEXT("导出", "Export"), command);
        previewAction->setMenu(previewMenu);
        command->addCommand(previewAction);
        auto *readOnly = new FluentToggleSwitch(DEMO_TEXT("只读", "Read only"));
        command->addWidget(readOnly);

        auto *progressRow = new QHBoxLayout();
        progressRow->setContentsMargins(0, 0, 0, 0);
        progressRow->setSpacing(12);
        auto *bar = new FluentProgressBar();
        bar->setRange(0, 100);
        bar->setValue(72);
        auto *ring = new FluentProgressRing();
        ring->setFixedSize(36, 36);
        ring->setValue(72);
        progressRow->addWidget(bar, 1);
        progressRow->addWidget(ring);

        auto *tip = new FluentInfoBar(FluentInfoBar::Severity::Info,
                                      DEMO_TEXT("主题联动", "Theme linked"),
                                      DEMO_TEXT("切换深浅色、accent 与 motion token 后，示例会同步更新。",
                                                "Examples update with light/dark, accent, and motion tokens."),
                                      preview);
        tip->setActionText(DEMO_TEXT("查看", "View"));
        tip->setClosable(false);

        previewLayout->addLayout(inputRow);
        previewLayout->addWidget(command);
        previewLayout->addLayout(progressRow);
        previewLayout->addWidget(tip);

        heroLayout->addWidget(copy, 1);
        heroLayout->addWidget(preview, 1);
        overviewLayout->addWidget(hero);
    }

    {
        auto *t = new FluentLabel(DEMO_TEXT("控件画廊", "Control gallery"));
        t->setStyleSheet("font-size: 18px; font-weight: 650;");
        auto *st = new FluentLabel(DEMO_TEXT("FlowLayout 自适应换行：窗口越宽，一行展示越多；新增控件也只需添加一个 tile。",
                                              "FlowLayout wraps adaptively: the wider the window, the more tiles fit on each row; adding a new control only requires one more tile."));
        st->setStyleSheet("font-size: 12px; opacity: 0.85;");
        st->setWordWrap(true);
        overviewLayout->addWidget(t);
        overviewLayout->addWidget(st);
    }

    auto *tilesHost = new QWidget();
    tilesHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *flow = new FluentFlowLayout(tilesHost, 0, 12, 12);
    flow->setUniformItemWidthEnabled(true);
    flow->setMinimumItemWidth(340);
    flow->setColumnHysteresis(18);
    flow->setAnimationEnabled(true);
    flow->setAnimationDuration(140);
    flow->setAnimateWhileResizing(true);
    flow->setAnimationThrottle(50);
    tilesHost->setLayout(flow);
    overviewLayout->addWidget(tilesHost);
    overviewLayout->addStretch(1);

    auto makeTile = [&](const QString &title,
                        const QString &subtitle,
                        const std::function<void(QVBoxLayout *)> &fill) -> FluentCard * {
        auto *card = new FluentCard();
        card->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        card->setMinimumWidth(320);

        auto *l = new QVBoxLayout(card);
        l->setContentsMargins(16, 16, 16, 16);
        l->setSpacing(10);

        auto *tt = new FluentLabel(title);
        tt->setStyleSheet("font-size: 14px; font-weight: 600;");
        l->addWidget(tt);

        if (!subtitle.isEmpty()) {
            auto *ss = new FluentLabel(subtitle);
            ss->setStyleSheet("font-size: 12px; opacity: 0.85;");
            ss->setWordWrap(true);
            l->addWidget(ss);
        }

        auto *body = new QVBoxLayout();
        body->setContentsMargins(0, 0, 0, 0);
        body->setSpacing(8);
        l->addLayout(body);

        fill(body);

        return card;
    };

    auto addGroupTitle = [&](const QString &title) {
        auto *lbl = new FluentLabel(title);
        lbl->setStyleSheet("font-size: 13px; font-weight: 650; opacity: 0.95;");
        lbl->setProperty(FluentFlowLayout::kFullRowProperty, true);
        flow->addWidget(lbl);
    };

    auto addJumpCard = [&](const QString &hint, int pageIndex) {
        auto *card = new FluentCard();
        card->setProperty(FluentFlowLayout::kFullRowProperty, true);
        card->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

        auto *l = new QHBoxLayout(card);
        l->setContentsMargins(16, 12, 16, 12);
        l->setSpacing(10);

        auto *lab = new FluentLabel(hint);
        lab->setStyleSheet("font-size: 12px; opacity: 0.88;");

        auto *btn = new FluentButton(DEMO_TEXT("进入该页 →", "Open page ->"));
        QObject::connect(btn, &QPushButton::clicked, window, [jumpTo, pageIndex]() { jumpTo(pageIndex); });

        l->addWidget(lab);
        l->addStretch(1);
        l->addWidget(btn);
        flow->addWidget(card);
    };

    addGroupTitle(DEMO_TEXT("按钮 / 开关", "Buttons / Toggles"));
    flow->addWidget(makeTile(QStringLiteral("Label"), DEMO_TEXT("主题切换不应覆盖粗体", "Theme switching should not override bold text"), [&](QVBoxLayout *body) {
        auto *a = new FluentLabel(QStringLiteral("FluentLabel Normal"));
        auto *b = new FluentLabel(QStringLiteral("FluentLabel Bold"));
        b->setStyleSheet("font-weight: 650;");
        body->addWidget(a);
        body->addWidget(b);
    }));

    flow->addWidget(makeTile(QStringLiteral("Button / ToolButton"), QString(), [&](QVBoxLayout *body) {
        auto *row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        auto *p = new FluentButton(QStringLiteral("Primary"));
        p->setPrimary(true);
        auto *s = new FluentButton(QStringLiteral("Button"));
        auto *tb = new FluentToolButton(QStringLiteral("Tool"));
        tb->setCheckable(true);
        tb->setChecked(true);
        row->addWidget(p);
        row->addWidget(s);
        row->addWidget(tb);
        row->addStretch(1);
        body->addLayout(row);
    }));

    flow->addWidget(makeTile(QStringLiteral("AnimatedButton"), DEMO_TEXT("Lottie marker 状态按钮", "Lottie marker state button"), [&](QVBoxLayout *body) {
        auto *row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(10);

        auto *search = Demo::makeAnimatedSearchButton(DEMO_TEXT("搜索控件", "Search controls"));
        auto *primary = Demo::makeAnimatedSearchButton(DEMO_TEXT("Primary", "Primary"));
        primary->setPrimary(true);

        row->addWidget(search);
        row->addWidget(primary);
        row->addStretch(1);
        body->addLayout(row);
    }));

    flow->addWidget(makeTile(QStringLiteral("Commanding"), DEMO_TEXT("DropDownButton / SplitButton / CommandBar", "DropDownButton / SplitButton / CommandBar"), [&](QVBoxLayout *body) {
        auto *row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);

        auto *drop = new FluentDropDownButton(DEMO_TEXT("更多", "More"));
        drop->addAction(DEMO_TEXT("复制", "Copy"));
        drop->addAction(DEMO_TEXT("移动", "Move"));

        auto *split = new FluentSplitButton(DEMO_TEXT("保存", "Save"));
        split->setDefaultAction(new QAction(DEMO_TEXT("保存", "Save"), split));
        auto *menu = new FluentMenu(split);
        menu->addAction(DEMO_TEXT("另存为", "Save as"));
        split->setMenu(menu);

        row->addWidget(drop);
        row->addWidget(split);
        row->addStretch(1);
        body->addLayout(row);

        auto *bar = new FluentCommandBar();
        auto *newAction = new QAction(DEMO_TEXT("新建", "New"), bar);
        auto *editAction = new QAction(DEMO_TEXT("编辑", "Edit"), bar);
        auto *exportMenu = new FluentMenu(bar);
        exportMenu->setTitle(DEMO_TEXT("导出", "Export"));
        exportMenu->addAction(QStringLiteral("PDF"));
        exportMenu->addAction(QStringLiteral("PNG"));
        auto *readOnly = new FluentToggleSwitch(DEMO_TEXT("只读", "Read only"));
        bar->addCommand(newAction);
        bar->addCommand(editAction);
        bar->addSeparator();
        bar->addCommand(exportMenu->menuAction());
        bar->addWidget(readOnly);
        body->addWidget(bar);
        auto *hint = new FluentLabel(DEMO_TEXT("CommandBar 组织一组可同步状态的 QAction。", "CommandBar organizes QAction commands with shared state."));
        hint->setWordWrap(true);
        body->addWidget(hint);
        QObject::connect(readOnly, &FluentToggleSwitch::toggled, editAction, [editAction](bool checked) {
            editAction->setEnabled(!checked);
        });
    }));

    flow->addWidget(makeTile(QStringLiteral("Toggle / Check / Radio"), DEMO_TEXT("常见开关与选择控件", "Common selection and toggle controls"), [&](QVBoxLayout *body) {
        auto *row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(10);

        auto *toggle = new FluentToggleSwitch(DEMO_TEXT("开关", "Toggle"));
        toggle->setChecked(true);
        auto *check = new FluentCheckBox(DEMO_TEXT("复选", "Check"));
        check->setChecked(true);
        auto *radio = new FluentRadioButton(DEMO_TEXT("单选", "Radio"));
        radio->setChecked(true);

        row->addWidget(toggle);
        row->addWidget(check);
        row->addWidget(radio);
        row->addStretch(1);
        body->addLayout(row);
    }));

    flow->addWidget(makeTile(QStringLiteral("ProgressBar"), DEMO_TEXT("Accent 会影响进度条颜色", "Accent affects the progress-bar color"), [&](QVBoxLayout *body) {
        auto *row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(12);

        auto *p = new FluentProgressBar();
        p->setRange(0, 100);
        p->setValue(66);
        auto *ring = new FluentProgressRing();
        ring->setFixedSize(42, 42);
        ring->setValue(66);
        auto *busy = new FluentProgressRing();
        busy->setFixedSize(42, 42);
        busy->setIndeterminate(true);

        row->addWidget(p, 1);
        row->addWidget(ring);
        row->addWidget(busy);
        body->addLayout(row);
    }));

    addJumpCard(DEMO_TEXT("按钮/开关页：Button / ToolButton / ToggleSwitch / CheckBox / RadioButton / ProgressBar", "Buttons page: Button / ToolButton / ToggleSwitch / CheckBox / RadioButton / ProgressBar"), 3);

    addGroupTitle(DEMO_TEXT("动态", "Motion"));
    flow->addWidget(makeTile(QStringLiteral("FluentLottieWidget"), DEMO_TEXT("加载内嵌 JSON 资源", "Loads embedded JSON assets"), [&](QVBoxLayout *body) {
        auto *row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(12);

        auto *settings = new FluentLottieWidget();
        settings->setFixedSize(72, 72);
        if (Demo::loadDemoLottieResource(settings, QStringLiteral("setting.json"))) {
            settings->play();
        }

        auto *dropdown = new FluentLottieWidget();
        dropdown->setFixedSize(72, 72);
        if (Demo::loadDemoLottieResource(dropdown, QStringLiteral("dropdown.json"))) {
            dropdown->play();
        }

        auto *text = new FluentLabel(DEMO_TEXT("设置、展开、导航等图标型 Lottie 效果集中放在动态页。", "Icon-like Lottie effects such as settings, expanders, and navigation live on the Motion page."));
        text->setWordWrap(true);
        text->setStyleSheet(QStringLiteral("font-size: 12px; opacity: 0.86;"));

        row->addWidget(settings);
        row->addWidget(dropdown);
        row->addWidget(text, 1);
        body->addLayout(row);
    }));
    addJumpCard(DEMO_TEXT("图标页：FluentIcon 内置图标矩阵 / 主题色 / 枚举复制", "Icons page: FluentIcon matrix / theme color / enum copy"), 4);
    addJumpCard(DEMO_TEXT("动态页：FluentLottieWidget / 播放控制 / 主题联动 / tint", "Motion page: FluentLottieWidget / playback / theme linkage / tint"), 5);

    addGroupTitle(DEMO_TEXT("输入", "Inputs"));
    flow->addWidget(makeTile(QStringLiteral("LineEdit"), QString(), [&](QVBoxLayout *body) {
        auto *le = new FluentLineEdit();
        le->setPlaceholderText(QStringLiteral("FluentLineEdit"));
        body->addWidget(le);
    }));

    flow->addWidget(makeTile(QStringLiteral("AutoSuggest / SearchBox"), DEMO_TEXT("候选建议与搜索提交", "Suggestions and search submission"), [&](QVBoxLayout *body) {
        QStringList suggestions;
        suggestions << QStringLiteral("FluentButton") << QStringLiteral("FluentCard") << QStringLiteral("FluentProgressRing");
        suggestions << QStringLiteral("FluentTeachingTip") << QStringLiteral("FluentNavigationView");

        auto *suggest = new FluentAutoSuggestBox();
        suggest->setPlaceholderText(DEMO_TEXT("输入控件名", "Type a control name"));
        suggest->setSuggestions(suggestions);

        auto *search = new FluentSearchBox();
        search->setPlaceholderText(DEMO_TEXT("搜索", "Search"));
        search->setSuggestions(suggestions);

        body->addWidget(suggest);
        body->addWidget(search);
    }));

    flow->addWidget(makeTile(QStringLiteral("Slider"), DEMO_TEXT("拖动观察 Hover/Pressed", "Drag to inspect hover / pressed states"), [&](QVBoxLayout *body) {
        auto *row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(10);
        auto *slider = new FluentSlider(Qt::Horizontal);
        slider->setRange(0, 100);
        slider->setValue(42);
        auto *val = new FluentLabel(QStringLiteral("42"));
        val->setMinimumWidth(28);
        QObject::connect(slider, &QSlider::valueChanged, val, [val](int v) {
            val->setText(QString::number(v));
        });
        row->addWidget(slider, 1);
        row->addWidget(val);
        body->addLayout(row);
    }));

    flow->addWidget(makeTile(QStringLiteral("SpinBox"), QString(), [&](QVBoxLayout *body) {
        auto *row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        auto *spin = new FluentSpinBox();
        spin->setRange(0, 999);
        spin->setValue(12);
        auto *dspin = new FluentDoubleSpinBox();
        dspin->setRange(0.0, 99.9);
        dspin->setDecimals(1);
        dspin->setValue(3.5);
        row->addWidget(spin);
        row->addWidget(dspin);
        row->addStretch(1);
        body->addLayout(row);
    }));

    flow->addWidget(makeTile(QStringLiteral("ComboBox"), QString(), [&](QVBoxLayout *body) {
        auto *cb = new FluentComboBox();
        cb->addItems({DEMO_TEXT("选项 A", "Option A"), DEMO_TEXT("选项 B", "Option B"), DEMO_TEXT("选项 C", "Option C")});
        cb->setCurrentIndex(1);
        body->addWidget(cb);
    }));

    flow->addWidget(makeTile(QStringLiteral("TextEdit"), DEMO_TEXT("包含 FluentScrollBar", "Includes FluentScrollBar"), [&](QVBoxLayout *body) {
        auto *te = new FluentTextEdit();
        te->setPlaceholderText(QStringLiteral("FluentTextEdit"));
        te->setText(QStringLiteral("1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n"));
        te->setFixedHeight(120);
        body->addWidget(te);
    }));

    flow->addWidget(makeTile(QStringLiteral("CodeEditor"), DEMO_TEXT("语法高亮 + 格式化（Ctrl+Shift+F）", "Syntax highlighting + formatting (Ctrl+Shift+F)"), [&](QVBoxLayout *body) {
        auto *ed = new FluentCodeEditor();
        ed->setFixedHeight(140);
        ed->setPlainText(QStringLiteral(
            "#include <vector>\n\n"
            "int main(){ std::vector<int> v{1,2,3}; return v.size(); }\n"));
        Demo::DemoCodeEditorSettings::instance().attach(ed, false);
        body->addWidget(ed);
    }));

    addJumpCard(DEMO_TEXT("输入页：LineEdit / TextEdit / Slider / SpinBox / ComboBox", "Inputs page: LineEdit / TextEdit / Slider / SpinBox / ComboBox"), 2);

    addGroupTitle(DEMO_TEXT("选择器", "Pickers"));
    flow->addWidget(makeTile(QStringLiteral("DatePicker"), DEMO_TEXT("滚轮式月 / 日 / 年选择", "Wheel-style month / day / year selection"), [&](QVBoxLayout *body) {
        auto *p = new FluentDatePicker();
        p->setDate(QDate::currentDate());
        body->addWidget(p);
    }));
    flow->addWidget(makeTile(QStringLiteral("CalendarPicker"), QString(), [&](QVBoxLayout *body) {
        auto *p = new FluentCalendarPicker();
        p->setDate(QDate::currentDate());
        body->addWidget(p);
    }));
    flow->addWidget(makeTile(QStringLiteral("TimePicker"), DEMO_TEXT("滚轮式时间选择", "Wheel-style time selection"), [&](QVBoxLayout *body) {
        auto *p = new FluentTimePicker();
        p->setTime(QTime::currentTime());
        body->addWidget(p);
    }));
    flow->addWidget(makeTile(QStringLiteral("ColorPicker"), DEMO_TEXT("内置颜色选择按钮", "Built-in color picker button"), [&](QVBoxLayout *body) {
        auto *p = new FluentColorPicker();
        p->setColor(ThemeManager::instance().colors().accent);
        QObject::connect(p, &FluentColorPicker::colorChanged, window, [](const QColor &c) {
            if (c.isValid()) {
                Demo::applyAccent(c);
            }
        });
        body->addWidget(p);
    }));
    flow->addWidget(makeTile(QStringLiteral("ColorDialog"), DEMO_TEXT("对话框类控件在示例页更完整", "Dialog-based control with a fuller example page"), [&](QVBoxLayout *body) {
        auto *btn = new FluentButton(DEMO_TEXT("打开 FluentColorDialog…", "Open FluentColorDialog..."));
        QObject::connect(btn, &QPushButton::clicked, window, [window]() {
            const QColor before = ThemeManager::instance().colors().accent;
            FluentColorDialog dlg(before, window);

            QObject::connect(&dlg, &FluentColorDialog::colorChanged, window, [](const QColor &c) {
                if (c.isValid()) {
                    Demo::applyAccent(c);
                }
            });

            if (dlg.exec() == QDialog::Accepted) {
                const QColor selected = dlg.selectedColor();
                if (selected.isValid()) {
                    Demo::applyAccent(selected);
                }
            } else {
                Demo::applyAccent(before);
            }
        });
        body->addWidget(btn);
    }));
    addJumpCard(DEMO_TEXT("选择器页：CalendarPicker / TimePicker / ColorPicker / ColorDialog", "Pickers page: CalendarPicker / TimePicker / ColorPicker / ColorDialog"), 6);
    addJumpCard(DEMO_TEXT("角度控件页：FluentDial / FluentAngleSelector / 可见性变体", "Angle Controls page: FluentDial / FluentAngleSelector / visibility variants"), 7);

    addGroupTitle(DEMO_TEXT("数据视图", "Data Views"));
    flow->addWidget(makeTile(QStringLiteral("ListView"), QString(), [&](QVBoxLayout *body) {
        auto *view = new FluentListView();
        view->setFixedHeight(110);
        auto *model = new QStringListModel(view);
        model->setStringList({
            QStringLiteral("Item A"),
            QStringLiteral("Item B"),
            QStringLiteral("Item C"),
            QStringLiteral("Item D"),
        });
        view->setModel(model);
        body->addWidget(view);
    }));
    flow->addWidget(makeTile(QStringLiteral("TableView"), QString(), [&](QVBoxLayout *body) {
        auto *view = new FluentTableView();
        view->setFixedHeight(130);
        auto *model = new QStandardItemModel(3, 3, view);
        model->setHorizontalHeaderLabels({DEMO_TEXT("列 1", "Column 1"), DEMO_TEXT("列 2", "Column 2"), DEMO_TEXT("列 3", "Column 3")});
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                model->setItem(r, c, new QStandardItem(QStringLiteral("%1,%2").arg(r + 1).arg(c + 1)));
            }
        }
        view->setModel(model);
        if (view->horizontalHeader()) {
            view->horizontalHeader()->setStretchLastSection(true);
        }
        view->setSelectionMode(QAbstractItemView::SingleSelection);
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->setEditTriggers(QAbstractItemView::NoEditTriggers);
        body->addWidget(view);
    }));
    flow->addWidget(makeTile(QStringLiteral("TreeView"), QString(), [&](QVBoxLayout *body) {
        auto *view = new FluentTreeView();
        view->setFixedHeight(150);
        auto *model = new QStandardItemModel(view);
        model->setHorizontalHeaderLabels({DEMO_TEXT("树", "Tree"), DEMO_TEXT("值", "Value")});
        auto *root = model->invisibleRootItem();
        auto *parentItem = new QStandardItem(QStringLiteral("Parent"));
        parentItem->appendRow({new QStandardItem(QStringLiteral("Child 1")), new QStandardItem(QStringLiteral("42"))});
        parentItem->appendRow({new QStandardItem(QStringLiteral("Child 2")), new QStandardItem(QStringLiteral("99"))});
        root->appendRow({parentItem, new QStandardItem(QString())});
        view->setModel(model);
        view->expandAll();
        view->setSelectionMode(QAbstractItemView::SingleSelection);
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->setEditTriggers(QAbstractItemView::NoEditTriggers);
        body->addWidget(view);
    }));
    addJumpCard(DEMO_TEXT("数据视图页：ListView / TableView / TreeView", "Data Views page: ListView / TableView / TreeView"), 8);

    addGroupTitle(DEMO_TEXT("容器/布局", "Containers / Layout"));
    flow->addWidget(makeTile(QStringLiteral("Card"), DEMO_TEXT("FluentCard 作为内容容器", "FluentCard used as a content container"), [&](QVBoxLayout *body) {
        auto *inner = new FluentCard();
        inner->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        auto *l = new QVBoxLayout(inner);
        l->setContentsMargins(12, 10, 12, 10);
        l->addWidget(new FluentLabel(DEMO_TEXT("这是一个嵌套的 FluentCard", "This is a nested FluentCard")));
        body->addWidget(inner);
    }));
    flow->addWidget(makeTile(QStringLiteral("Collapsible Card"), DEMO_TEXT("点击标题折叠/展开（仅显示标题）", "Click the title to collapse or expand (title-only when collapsed)"), [&](QVBoxLayout *body) {
        auto *card = new FluentCard();
        card->setCollapsible(true);
        card->setTitle(DEMO_TEXT("高级选项", "Advanced options"));
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        auto *cl = card->contentLayout();
        if (cl) {
            cl->setSpacing(6);
            cl->addWidget(new FluentLabel(DEMO_TEXT("-这里放更复杂的设置项", "-Place more advanced settings here")));
            cl->addWidget(new FluentLabel(DEMO_TEXT("-折叠后内容会隐藏并收缩高度", "-When collapsed, the content hides and the height shrinks")));
        }

        body->addWidget(card);
    }));
    flow->addWidget(makeTile(QStringLiteral("GroupBox"), QString(), [&](QVBoxLayout *body) {
        auto *gb = new FluentGroupBox(DEMO_TEXT("分组", "Group"));
        auto *l = new QVBoxLayout(gb);
        l->setContentsMargins(12, 10, 12, 10);
        l->addWidget(new FluentLabel(DEMO_TEXT("把相关控件组织在一起", "Organize related controls together")));
        body->addWidget(gb);
    }));
    flow->addWidget(makeTile(QStringLiteral("TabWidget"), QString(), [&](QVBoxLayout *body) {
        auto *tabs = new FluentTabWidget();
        tabs->setFixedHeight(160);
        tabs->addTab(new FluentLabel(QStringLiteral("Tab A 内容")), QStringLiteral("Tab A"));
        tabs->addTab(new FluentLabel(QStringLiteral("Tab B 内容")), QStringLiteral("Tab B"));
        body->addWidget(tabs);
    }));
    flow->addWidget(makeTile(QStringLiteral("ScrollArea"), DEMO_TEXT("overlay 滚动条（可见滚动）", "Overlay scrollbars with visible scrolling"), [&](QVBoxLayout *body) {
        auto *area = new FluentScrollArea();
        area->setOverlayScrollBarsEnabled(true);
        area->setWidgetResizable(true);
        area->setFixedHeight(140);
        auto *scrollContent = new FluentWidget();
        scrollContent->setBackgroundRole(FluentWidget::BackgroundRole::Transparent);
        area->setWidget(scrollContent);
        auto *sl = new QVBoxLayout(scrollContent);
        sl->setContentsMargins(12, 12, 12, 12);
        sl->setSpacing(6);
        for (int i = 1; i <= 18; ++i) {
            sl->addWidget(new FluentLabel(DEMO_TEXT("滚动内容 %1", "Scrollable content %1").arg(i)));
        }
        body->addWidget(area);
    }));

    flow->addWidget(makeTile(QStringLiteral("ScrollBar"), QString(), [&](QVBoxLayout *body) {
        auto *sb = new FluentScrollBar(Qt::Horizontal);
        sb->setRange(0, 100);
        sb->setValue(30);
        body->addWidget(sb);
    }));

    flow->addWidget(makeTile(QStringLiteral("AnnotatedScrollBar"), DEMO_TEXT("滚动时右侧显示当前分段", "Shows the current section on the right while scrolling"), [&](QVBoxLayout *body) {
        auto *shell = new FluentWidget();
        shell->setBackgroundRole(FluentWidget::BackgroundRole::Transparent);

        auto *row = new QHBoxLayout(shell);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);

        auto *area = new FluentScrollArea();
        area->setWidgetResizable(true);
        area->setOverlayScrollBarsEnabled(false);
        area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        area->setFrameShape(QFrame::NoFrame);
        area->setFixedHeight(152);

        auto *content = new FluentWidget();
        content->setBackgroundRole(FluentWidget::BackgroundRole::Transparent);
        area->setWidget(content);

        auto *contentLayout = new QVBoxLayout(content);
        contentLayout->setContentsMargins(8, 8, 8, 8);
        contentLayout->setSpacing(8);

        const QVector<FluentAnnotatedScrollBarSource> sources = {
            {DEMO_TEXT("首页", "Home"), DEMO_TEXT("欢迎概览", "Welcome overview"), 8, 83},
            {DEMO_TEXT("首页", "Home"), DEMO_TEXT("快捷入口", "Quick entry"), 92, 167},
            {DEMO_TEXT("库存", "Inventory"), DEMO_TEXT("仓储列表", "Warehouse list"), 176, 251},
            {DEMO_TEXT("设置", "Settings"), DEMO_TEXT("参数设置", "Parameter settings"), 260, 335}
        };

        for (const FluentAnnotatedScrollBarSource &source : sources) {
            auto *card = new FluentCard();
            card->setFixedHeight(76);
            auto *cardLayout = new QVBoxLayout(card);
            cardLayout->setContentsMargins(12, 10, 12, 10);
            cardLayout->setSpacing(4);
            auto *title = new FluentLabel(source.text);
            title->setStyleSheet("font-size: 12px; font-weight: 650;");
            auto *summary = new FluentLabel(DEMO_TEXT("所属分组：%1", "Group: %1").arg(source.group));
            summary->setStyleSheet("font-size: 11px; opacity: 0.84;");
            summary->setWordWrap(true);
            cardLayout->addWidget(title);
            cardLayout->addWidget(summary);
            contentLayout->addWidget(card);
        }

        auto *annotated = new FluentAnnotatedScrollBar();
        annotated->setFixedWidth(94);
        annotated->setToolTipDuration(1000);
        annotated->setScrollArea(area);
        annotated->setSources(sources);

        row->addWidget(area, 1);
        row->addWidget(annotated);
        body->addWidget(shell);
    }));

    flow->addWidget(makeTile(QStringLiteral("FlowLayout"), DEMO_TEXT("总览页/侧栏卡片均使用 FlowLayout", "The overview and sidebar cards both use FlowLayout"), [&](QVBoxLayout *body) {
        auto *lab = new FluentLabel(DEMO_TEXT(
            "-FluentFlowLayout：自适应换行\n"
            "-uniformItemWidth：统一卡片宽度\n"
            "-resize 动画：缩放时更顺滑",
            "-FluentFlowLayout: adaptive wrapping\n"
            "-uniformItemWidth: aligned card widths\n"
            "-resize animation: smoother transitions while resizing"));
        lab->setWordWrap(true);
        body->addWidget(lab);
    }));

    addJumpCard(DEMO_TEXT("容器/布局页：Card / GroupBox / TabWidget / ScrollArea / ScrollBar / AnnotatedScrollBar / Splitter / FlowLayout", "Containers page: Card / GroupBox / TabWidget / ScrollArea / ScrollBar / AnnotatedScrollBar / Splitter / FlowLayout"), 9);

    addGroupTitle(DEMO_TEXT("窗口 / 对话框", "Windows / Dialogs"));
    flow->addWidget(makeTile(QStringLiteral("InfoBar"), DEMO_TEXT("页面内状态提示", "Inline status message"), [&](QVBoxLayout *body) {
        auto *info = new FluentInfoBar(FluentInfoBar::Severity::Success,
                                       DEMO_TEXT("已保存", "Saved"),
                                       DEMO_TEXT("更改已写入本地配置。", "Changes were written to local settings."));
        info->setActionText(DEMO_TEXT("撤销", "Undo"));
        body->addWidget(info);
    }));
    flow->addWidget(makeTile(QStringLiteral("Popup / Flyout / TeachingTip"), DEMO_TEXT("命令弹层、上下文面板与引导", "Command popup, contextual panel, and guidance"), [&](QVBoxLayout *body) {
        auto *row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);

        auto *popupBtn = new FluentButton(QStringLiteral("Popup"));
        auto *flyoutBtn = new FluentButton(QStringLiteral("Flyout"));
        auto *tipBtn = new FluentButton(QStringLiteral("Tip"));
        row->addWidget(popupBtn);
        row->addWidget(flyoutBtn);
        row->addWidget(tipBtn);
        row->addStretch(1);
        body->addLayout(row);

        QObject::connect(popupBtn, &QPushButton::clicked, window, [window, popupBtn]() {
            auto *menu = new FluentMenu(window);
            menu->addAction(DEMO_TEXT("剪切", "Cut"));
            menu->addAction(DEMO_TEXT("复制", "Copy"));
            QObject::connect(menu, &QMenu::aboutToHide, menu, &QObject::deleteLater);
            menu->popup(popupBtn->mapToGlobal(QPoint(0, popupBtn->height() + 4)));
        });
        QObject::connect(flyoutBtn, &QPushButton::clicked, window, [window, flyoutBtn]() {
            auto *flyout = new FluentFlyout(window);
            auto *panel = new FluentWidget();
            panel->setBackgroundRole(FluentWidget::BackgroundRole::Transparent);
            auto *layout = new QVBoxLayout(panel);
            layout->setContentsMargins(4, 4, 4, 4);
            layout->setSpacing(8);
            layout->addWidget(new FluentLabel(DEMO_TEXT("视图设置", "View settings")));
            layout->addWidget(new FluentToggleSwitch(DEMO_TEXT("显示预览", "Show preview")));
            flyout->setContentWidget(panel);
            flyout->showFor(flyoutBtn);
        });
        QObject::connect(tipBtn, &QPushButton::clicked, window, [window, tipBtn]() {
            auto *tip = new FluentTeachingTip(window);
            tip->setTitle(DEMO_TEXT("新功能", "New feature"));
            tip->setSubtitle(DEMO_TEXT("TeachingTip 解释一个目标控件。", "TeachingTip explains a target control."));
            tip->setActionText(DEMO_TEXT("开始", "Start"));
            tip->setTarget(tipBtn);
            tip->open();
        });
    }));
    flow->addWidget(makeTile(QStringLiteral("Dialog"), DEMO_TEXT("支持可选蒙版 overlay", "Supports an optional overlay mask"), [&](QVBoxLayout *body) {
        auto *btn = new FluentButton(DEMO_TEXT("打开 FluentDialog…", "Open FluentDialog..."));
        QObject::connect(btn, &QPushButton::clicked, window, [window]() {
            FluentDialog dlg(window);
            dlg.setMaskEnabled(true);
            auto *l = new QVBoxLayout(&dlg);
            l->setContentsMargins(16, 16, 16, 16);
            l->addWidget(new FluentLabel(DEMO_TEXT("这是一个 FluentDialog（带蒙版）", "This is a FluentDialog with an overlay mask")));
            dlg.resize(520, 260);
            dlg.exec();
        });
        body->addWidget(btn);
    }));
    flow->addWidget(makeTile(QStringLiteral("MessageBox"), QString(), [&](QVBoxLayout *body) {
        auto *btn = new FluentButton(DEMO_TEXT("弹出 FluentMessageBox…", "Show FluentMessageBox..."));
        QObject::connect(btn, &QPushButton::clicked, window, [window]() {
            FluentMessageBox::information(window,
                                         QStringLiteral("MessageBox"),
                                         DEMO_TEXT("这是一条信息提示（带蒙版 overlay）", "This is an informational prompt with an overlay mask"),
                                         QString(),
                                         true);
        });
        body->addWidget(btn);
    }));
    flow->addWidget(makeTile(QStringLiteral("Menu"), DEMO_TEXT("右键/按钮触发弹出", "Triggered from a button or right-click"), [&](QVBoxLayout *body) {
        auto *btn = new FluentButton(DEMO_TEXT("弹出 FluentMenu…", "Show FluentMenu..."));
        QObject::connect(btn, &QPushButton::clicked, window, [window]() {
            auto *menu = new FluentMenu(window);
            menu->addAction(DEMO_TEXT("操作 A", "Action A"));
            menu->addAction(DEMO_TEXT("操作 B", "Action B"));
            auto *sub = menu->addFluentMenu(DEMO_TEXT("更多", "More"));
            sub->addAction(DEMO_TEXT("子菜单项", "Submenu item"));
            menu->exec(QCursor::pos());
            menu->deleteLater();
        });
        body->addWidget(btn);
    }));
    flow->addWidget(makeTile(QStringLiteral("Toast"), DEMO_TEXT("位置/动画在示例页更完整", "Position and animation are demonstrated more fully on the dedicated page"), [&](QVBoxLayout *body) {
        auto *btn = new FluentButton(DEMO_TEXT("发一条 Toast", "Send a Toast"));
        QObject::connect(btn, &QPushButton::clicked, window, [window]() {
            FluentToast::showToast(window,
                                  QStringLiteral("Toast"),
                                  DEMO_TEXT("这是一条 Toast（点击可关闭）", "This is a Toast notification (click to dismiss)"),
                                  FluentToast::Position::BottomRight,
                                  2400);
        });
        body->addWidget(btn);
    }));
    flow->addWidget(makeTile(QStringLiteral("Window Chrome"),
                             DEMO_TEXT("这些控件更适合在“窗口/对话框”页集中演示", "These controls are demonstrated more appropriately on the Windows / Dialogs page"),
                             [&](QVBoxLayout *body) {
                                 auto *lab = new FluentLabel(DEMO_TEXT(
                                     "-FluentMainWindow\n"
                                     "-FluentMenuBar / FluentToolBar / FluentStatusBar\n"
                                     "-FluentResizeHelper\n"
                                     "-FluentStyle / FluentTheme",
                                     "-FluentMainWindow\n"
                                     "-FluentMenuBar / FluentToolBar / FluentStatusBar\n"
                                     "-FluentResizeHelper\n"
                                     "-FluentStyle / FluentTheme"));
                                 lab->setWordWrap(true);
                                 body->addWidget(lab);
                             }));
    addJumpCard(DEMO_TEXT("窗口/对话框页：Dialog / MessageBox / Menu / MenuBar / ToolBar / StatusBar / Toast", "Windows / Dialogs page: Dialog / MessageBox / Menu / MenuBar / ToolBar / StatusBar / Toast"), 10);

    return overviewArea;
}

} // namespace Demo::Pages
