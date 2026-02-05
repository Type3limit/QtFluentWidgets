#include "PageOverview.h"

#include "../DemoHelpers.h"
#include "../DemoCodeEditorSettings.h"

#include <QAbstractItemView>
#include <QCursor>
#include <QDate>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QTime>
#include <QVBoxLayout>

#include "Fluent/FluentButton.h"
#include "Fluent/FluentCalendarPicker.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentCheckBox.h"
#include "Fluent/FluentColorDialog.h"
#include "Fluent/FluentColorPicker.h"
#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentDialog.h"
#include "Fluent/FluentFlowLayout.h"
#include "Fluent/FluentGroupBox.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentLineEdit.h"
#include "Fluent/FluentListView.h"
#include "Fluent/FluentMainWindow.h"
#include "Fluent/FluentMenu.h"
#include "Fluent/FluentMessageBox.h"
#include "Fluent/FluentProgressBar.h"
#include "Fluent/FluentRadioButton.h"
#include "Fluent/FluentScrollArea.h"
#include "Fluent/FluentScrollBar.h"
#include "Fluent/FluentSlider.h"
#include "Fluent/FluentSpinBox.h"
#include "Fluent/FluentTableView.h"
#include "Fluent/FluentTabWidget.h"
#include "Fluent/FluentCodeEditor.h"
#include "Fluent/FluentTextEdit.h"
#include "Fluent/FluentTheme.h"
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
        auto *t = new FluentLabel(QStringLiteral("控件画廊"));
        t->setStyleSheet("font-size: 18px; font-weight: 650;");
        auto *st = new FluentLabel(QStringLiteral("FlowLayout 自适应换行：窗口越宽，一行展示越多；新增控件也只需添加一个 tile。"));
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

        auto *btn = new FluentButton(QStringLiteral("进入该页 →"));
        QObject::connect(btn, &QPushButton::clicked, window, [jumpTo, pageIndex]() { jumpTo(pageIndex); });

        l->addWidget(lab);
        l->addStretch(1);
        l->addWidget(btn);
        flow->addWidget(card);
    };

    addGroupTitle(QStringLiteral("按钮 / 开关"));
    flow->addWidget(makeTile(QStringLiteral("Label"), QStringLiteral("主题切换不应覆盖粗体"), [&](QVBoxLayout *body) {
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

    flow->addWidget(makeTile(QStringLiteral("Toggle / Check / Radio"), QStringLiteral("常见开关与选择控件"), [&](QVBoxLayout *body) {
        auto *row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(10);

        auto *toggle = new FluentToggleSwitch(QStringLiteral("开关"));
        toggle->setChecked(true);
        auto *check = new FluentCheckBox(QStringLiteral("复选"));
        check->setChecked(true);
        auto *radio = new FluentRadioButton(QStringLiteral("单选"));
        radio->setChecked(true);

        row->addWidget(toggle);
        row->addWidget(check);
        row->addWidget(radio);
        row->addStretch(1);
        body->addLayout(row);
    }));

    flow->addWidget(makeTile(QStringLiteral("ProgressBar"), QStringLiteral("Accent 会影响进度条颜色"), [&](QVBoxLayout *body) {
        auto *p = new FluentProgressBar();
        p->setRange(0, 100);
        p->setValue(66);
        body->addWidget(p);
    }));

    addJumpCard(QStringLiteral("按钮/开关页：Button / ToolButton / ToggleSwitch / CheckBox / RadioButton / ProgressBar"), 2);

    addGroupTitle(QStringLiteral("输入"));
    flow->addWidget(makeTile(QStringLiteral("LineEdit"), QString(), [&](QVBoxLayout *body) {
        auto *le = new FluentLineEdit();
        le->setPlaceholderText(QStringLiteral("FluentLineEdit"));
        body->addWidget(le);
    }));

    flow->addWidget(makeTile(QStringLiteral("Slider"), QStringLiteral("拖动观察 Hover/Pressed"), [&](QVBoxLayout *body) {
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
        cb->addItems({QStringLiteral("选项 A"), QStringLiteral("选项 B"), QStringLiteral("选项 C")});
        cb->setCurrentIndex(1);
        body->addWidget(cb);
    }));

    flow->addWidget(makeTile(QStringLiteral("TextEdit"), QStringLiteral("包含 FluentScrollBar"), [&](QVBoxLayout *body) {
        auto *te = new FluentTextEdit();
        te->setPlaceholderText(QStringLiteral("FluentTextEdit"));
        te->setText(QStringLiteral("1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n"));
        te->setFixedHeight(120);
        body->addWidget(te);
    }));

    flow->addWidget(makeTile(QStringLiteral("CodeEditor"), QStringLiteral("语法高亮 + 格式化（Ctrl+Shift+F）"), [&](QVBoxLayout *body) {
        auto *ed = new FluentCodeEditor();
        ed->setFixedHeight(140);
        ed->setPlainText(QStringLiteral(
            "#include <vector>\n\n"
            "int main(){ std::vector<int> v{1,2,3}; return v.size(); }\n"));
        Demo::DemoCodeEditorSettings::instance().attach(ed, false);
        body->addWidget(ed);
    }));

    addJumpCard(QStringLiteral("输入页：LineEdit / TextEdit / Slider / SpinBox / ComboBox"), 1);

    addGroupTitle(QStringLiteral("选择器"));
    flow->addWidget(makeTile(QStringLiteral("CalendarPicker"), QString(), [&](QVBoxLayout *body) {
        auto *p = new FluentCalendarPicker();
        p->setDate(QDate::currentDate());
        body->addWidget(p);
    }));
    flow->addWidget(makeTile(QStringLiteral("TimePicker"), QString(), [&](QVBoxLayout *body) {
        auto *p = new FluentTimePicker();
        p->setTime(QTime::currentTime());
        body->addWidget(p);
    }));
    flow->addWidget(makeTile(QStringLiteral("ColorPicker"), QStringLiteral("内置颜色选择按钮"), [&](QVBoxLayout *body) {
        auto *p = new FluentColorPicker();
        p->setColor(ThemeManager::instance().colors().accent);
        QObject::connect(p, &FluentColorPicker::colorChanged, window, [](const QColor &c) {
            if (c.isValid()) {
                Demo::applyAccent(c);
            }
        });
        body->addWidget(p);
    }));
    flow->addWidget(makeTile(QStringLiteral("ColorDialog"), QStringLiteral("对话框类控件在示例页更完整"), [&](QVBoxLayout *body) {
        auto *btn = new FluentButton(QStringLiteral("打开 FluentColorDialog…"));
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
    addJumpCard(QStringLiteral("选择器页：CalendarPicker / TimePicker / ColorPicker / ColorDialog"), 3);

    addGroupTitle(QStringLiteral("数据视图"));
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
        model->setHorizontalHeaderLabels({QStringLiteral("列 1"), QStringLiteral("列 2"), QStringLiteral("列 3")});
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
        model->setHorizontalHeaderLabels({QStringLiteral("树"), QStringLiteral("值")});
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
    addJumpCard(QStringLiteral("数据视图页：ListView / TableView / TreeView"), 4);

    addGroupTitle(QStringLiteral("容器/布局"));
    flow->addWidget(makeTile(QStringLiteral("Card"), QStringLiteral("FluentCard 作为内容容器"), [&](QVBoxLayout *body) {
        auto *inner = new FluentCard();
        inner->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        auto *l = new QVBoxLayout(inner);
        l->setContentsMargins(12, 10, 12, 10);
        l->addWidget(new FluentLabel(QStringLiteral("这是一个嵌套的 FluentCard")));
        body->addWidget(inner);
    }));
    flow->addWidget(makeTile(QStringLiteral("Collapsible Card"), QStringLiteral("点击标题折叠/展开（仅显示标题）"), [&](QVBoxLayout *body) {
        auto *card = new FluentCard();
        card->setCollapsible(true);
        card->setTitle(QStringLiteral("高级选项"));
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        auto *cl = card->contentLayout();
        if (cl) {
            cl->setSpacing(6);
            cl->addWidget(new FluentLabel(QStringLiteral("-这里放更复杂的设置项")));
            cl->addWidget(new FluentLabel(QStringLiteral("-折叠后内容会隐藏并收缩高度")));
        }

        body->addWidget(card);
    }));
    flow->addWidget(makeTile(QStringLiteral("GroupBox"), QString(), [&](QVBoxLayout *body) {
        auto *gb = new FluentGroupBox(QStringLiteral("分组"));
        auto *l = new QVBoxLayout(gb);
        l->setContentsMargins(12, 10, 12, 10);
        l->addWidget(new FluentLabel(QStringLiteral("把相关控件组织在一起")));
        body->addWidget(gb);
    }));
    flow->addWidget(makeTile(QStringLiteral("TabWidget"), QString(), [&](QVBoxLayout *body) {
        auto *tabs = new FluentTabWidget();
        tabs->setFixedHeight(160);
        tabs->addTab(new FluentLabel(QStringLiteral("Tab A 内容")), QStringLiteral("Tab A"));
        tabs->addTab(new FluentLabel(QStringLiteral("Tab B 内容")), QStringLiteral("Tab B"));
        body->addWidget(tabs);
    }));
    flow->addWidget(makeTile(QStringLiteral("ScrollArea"), QStringLiteral("overlay 滚动条（可见滚动）"), [&](QVBoxLayout *body) {
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
            sl->addWidget(new FluentLabel(QStringLiteral("滚动内容 %1").arg(i)));
        }
        body->addWidget(area);
    }));

    flow->addWidget(makeTile(QStringLiteral("ScrollBar"), QString(), [&](QVBoxLayout *body) {
        auto *sb = new FluentScrollBar(Qt::Horizontal);
        sb->setRange(0, 100);
        sb->setValue(30);
        body->addWidget(sb);
    }));

    flow->addWidget(makeTile(QStringLiteral("FlowLayout"), QStringLiteral("总览页/侧栏卡片均使用 FlowLayout"), [&](QVBoxLayout *body) {
        auto *lab = new FluentLabel(QStringLiteral(
            "-FluentFlowLayout：自适应换行\n"
            "-uniformItemWidth：统一卡片宽度\n"
            "-resize 动画：缩放时更顺滑"));
        lab->setWordWrap(true);
        body->addWidget(lab);
    }));

    addJumpCard(QStringLiteral("容器/布局页：Card / GroupBox / TabWidget / ScrollArea / ScrollBar / Splitter / FlowLayout"), 5);

    addGroupTitle(QStringLiteral("窗口 / 对话框"));
    flow->addWidget(makeTile(QStringLiteral("Dialog"), QStringLiteral("支持可选蒙版 overlay"), [&](QVBoxLayout *body) {
        auto *btn = new FluentButton(QStringLiteral("打开 FluentDialog…"));
        QObject::connect(btn, &QPushButton::clicked, window, [window]() {
            FluentDialog dlg(window);
            dlg.setMaskEnabled(true);
            auto *l = new QVBoxLayout(&dlg);
            l->setContentsMargins(16, 16, 16, 16);
            l->addWidget(new FluentLabel(QStringLiteral("这是一个 FluentDialog（带蒙版）")));
            dlg.resize(520, 260);
            dlg.exec();
        });
        body->addWidget(btn);
    }));
    flow->addWidget(makeTile(QStringLiteral("MessageBox"), QString(), [&](QVBoxLayout *body) {
        auto *btn = new FluentButton(QStringLiteral("弹出 FluentMessageBox…"));
        QObject::connect(btn, &QPushButton::clicked, window, [window]() {
            FluentMessageBox::information(window,
                                         QStringLiteral("MessageBox"),
                                         QStringLiteral("这是一条信息提示（带蒙版 overlay）"),
                                         QString(),
                                         true);
        });
        body->addWidget(btn);
    }));
    flow->addWidget(makeTile(QStringLiteral("Menu"), QStringLiteral("右键/按钮触发弹出"), [&](QVBoxLayout *body) {
        auto *btn = new FluentButton(QStringLiteral("弹出 FluentMenu…"));
        QObject::connect(btn, &QPushButton::clicked, window, [window]() {
            auto *menu = new FluentMenu(window);
            menu->addAction(QStringLiteral("操作 A"));
            menu->addAction(QStringLiteral("操作 B"));
            auto *sub = menu->addFluentMenu(QStringLiteral("更多"));
            sub->addAction(QStringLiteral("子菜单项"));
            menu->exec(QCursor::pos());
            menu->deleteLater();
        });
        body->addWidget(btn);
    }));
    flow->addWidget(makeTile(QStringLiteral("Toast"), QStringLiteral("位置/动画在示例页更完整"), [&](QVBoxLayout *body) {
        auto *btn = new FluentButton(QStringLiteral("发一条 Toast"));
        QObject::connect(btn, &QPushButton::clicked, window, [window]() {
            FluentToast::showToast(window,
                                  QStringLiteral("Toast"),
                                  QStringLiteral("这是一条 Toast（点击可关闭）"),
                                  FluentToast::Position::BottomRight,
                                  2400);
        });
        body->addWidget(btn);
    }));
    flow->addWidget(makeTile(QStringLiteral("Window Chrome"),
                             QStringLiteral("这些控件更适合在“窗口/对话框”页集中演示"),
                             [&](QVBoxLayout *body) {
                                 auto *lab = new FluentLabel(QStringLiteral(
                                     "-FluentMainWindow\n"
                                     "-FluentMenuBar / FluentToolBar / FluentStatusBar\n"
                                     "-FluentResizeHelper\n"
                                     "-FluentStyle / FluentTheme"));
                                 lab->setWordWrap(true);
                                 body->addWidget(lab);
                             }));
    addJumpCard(QStringLiteral("窗口/对话框页：Dialog / MessageBox / Menu / MenuBar / ToolBar / StatusBar / Toast"), 6);

    return overviewArea;
}

} // namespace Demo::Pages
