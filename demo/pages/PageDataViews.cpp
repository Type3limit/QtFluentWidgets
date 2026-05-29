#include "PageDataViews.h"

#include "../DemoHelpers.h"
#include "Fluent/FluentCard.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QVBoxLayout>

#include "Fluent/FluentLabel.h"
#include "Fluent/FluentListView.h"
#include "Fluent/FluentMainWindow.h"
#include "Fluent/FluentScrollArea.h"
#include "Fluent/FluentTableView.h"
#include "Fluent/FluentTableWidget.h"
#include "Fluent/FluentTreeView.h"
#include "Fluent/FluentButton.h"
#include "Fluent/FluentCheckBox.h"
#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentSpinBox.h"
#include "Fluent/FluentToggleSwitch.h"

#include <QTableWidgetItem>

namespace Demo::Pages {

using namespace Fluent;

QWidget *createDataViewsPage(FluentMainWindow *window)
{
    return Demo::makePage([&](QVBoxLayout *page) {
        auto s = Demo::makeSection(DEMO_TEXT("数据视图", "Data Views"),
                                   DEMO_TEXT("ListView / TableView / TreeView（选择变化联动到详情区）", "ListView / TableView / TreeView with selection-driven detail updates"));

        page->addWidget(s.card);

        // State preview
        {
            QString code;
#define DATAVIEWS_STATE_PREVIEW(X) \
    X(auto *listRow = new QHBoxLayout();) \
    X(listRow->setContentsMargins(0, 0, 0, 0);) \
    X(listRow->setSpacing(10);) \
    X(auto *activeList = new FluentListView();) \
    X(auto *activeListModel = new QStringListModel({QStringLiteral("Active item"), QStringLiteral("Selected item"), QStringLiteral("Hover target")}, activeList);) \
    X(activeList->setModel(activeListModel);) \
    X(activeList->setFixedHeight(118);) \
    X(activeList->setCurrentIndex(activeListModel->index(1, 0));) \
    X(activeList->selectionModel()->select(activeListModel->index(1, 0), QItemSelectionModel::ClearAndSelect);) \
    X(auto *disabledList = new FluentListView();) \
    X(auto *disabledListModel = new QStringListModel({QStringLiteral("Disabled item"), QStringLiteral("Disabled selection"), QStringLiteral("Disabled hover")}, disabledList);) \
    X(disabledList->setModel(disabledListModel);) \
    X(disabledList->setFixedHeight(118);) \
    X(disabledList->setCurrentIndex(disabledListModel->index(1, 0));) \
    X(disabledList->selectionModel()->select(disabledListModel->index(1, 0), QItemSelectionModel::ClearAndSelect);) \
    X(disabledList->setEnabled(false);) \
    X(listRow->addWidget(activeList, 1);) \
    X(listRow->addWidget(disabledList, 1);) \
    X(body->addLayout(listRow);) \
    X(auto *table = new FluentTableView();) \
    X(auto *tableModel = new QStandardItemModel(3, 2, table);) \
    X(tableModel->setHorizontalHeaderLabels({DEMO_TEXT("状态", "State"), DEMO_TEXT("Token", "Token")} );) \
    X(tableModel->setItem(0, 0, new QStandardItem(QStringLiteral("Normal"))); tableModel->setItem(0, 1, new QStandardItem(QStringLiteral("surface/text")));) \
    X(tableModel->setItem(1, 0, new QStandardItem(QStringLiteral("Selected"))); tableModel->setItem(1, 1, new QStandardItem(QStringLiteral("accent + neutral")));) \
    X(tableModel->setItem(2, 0, new QStandardItem(QStringLiteral("Focus"))); tableModel->setItem(2, 1, new QStandardItem(QStringLiteral("accent border")));) \
    X(table->setModel(tableModel);) \
    X(table->setFixedHeight(128);) \
    X(table->setCurrentIndex(tableModel->index(1, 0));) \
    X(table->selectionModel()->select(tableModel->index(1, 0), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);) \
    X(body->addWidget(table);)

#define X(line) code += QStringLiteral(#line "\n");
            DATAVIEWS_STATE_PREVIEW(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                DEMO_TEXT("状态与主题联动", "State and theme coupling"),
                DEMO_TEXT("快速检查 active / selected / disabled / focus 在浅色、深色与自定义 accent 下的观感。",
                          "Quickly inspect active / selected / disabled / focus states under light, dark, and custom accent colors."),
                DEMO_TEXT("要点：\n"
                          "-ListView / TableView 的边框、文字、header、disabled 状态都由 theme token 派生\n"
                          "-关闭全局动效后 hover 直接切换到最终状态\n"
                          "-这个示例适合配合右侧设置面板切换主题后肉眼检查",
                          "Highlights:\n"
                          "-ListView / TableView borders, text, headers, and disabled states are derived from theme tokens\n"
                          "-When global animations are disabled, hover jumps directly to its final state\n"
                          "-Use this sample with the theme settings panel for quick visual checks"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    DATAVIEWS_STATE_PREVIEW(X)
#undef X
                },
                false,
                250));

#undef DATAVIEWS_STATE_PREVIEW
        }

        // ListView
        {
            QString code;
#define DATAVIEWS_LIST(X) \
    X(auto *detail = new FluentLabel(DEMO_TEXT("选择任意项查看详情", "Select any item to view details"));) \
    X(detail->setStyleSheet("font-size: 12px; opacity: 0.9;");) \
    X(body->addWidget(detail);) \
    X(auto *list = new FluentListView();) \
    X(auto *listModel = new QStringListModel(window);) \
    X(listModel->setStringList({QStringLiteral("Alpha"), QStringLiteral("Beta"), QStringLiteral("Gamma"), QStringLiteral("Delta"), QStringLiteral("Epsilon"), QStringLiteral("Zeta"), QStringLiteral("Eta"), QStringLiteral("Theta"), QStringLiteral("Iota")});) \
    X(list->setModel(listModel);) \
    X(list->setFixedHeight(220);) \
    X(QObject::connect(list->selectionModel(), &QItemSelectionModel::selectionChanged, window, [=]() { const QModelIndex idx = list->currentIndex(); if (idx.isValid()) { detail->setText(QStringLiteral("ListView: %1").arg(idx.data().toString())); } });) \
    X(body->addWidget(list);)

#define X(line) code += QStringLiteral(#line "\n");
            DATAVIEWS_LIST(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentListView"),
                DEMO_TEXT("列表视图（QAbstractItemView 风格）", "List view in a QAbstractItemView style"),
                DEMO_TEXT("要点：\n"
                          "-setModel() 绑定 QStringListModel/QStandardItemModel\n"
                          "-selectionModel()->selectionChanged 监听选择变化\n"
                          "-双击可编辑文本项\n"
                          "-适合轻量列表/侧栏",
                          "Highlights:\n"
                          "-Use setModel() with QStringListModel or QStandardItemModel\n"
                          "-Listen to selectionModel()->selectionChanged for updates\n"
                          "-Double-click text items to edit them\n"
                          "-A good fit for lightweight lists and side panels"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    DATAVIEWS_LIST(X)
#undef X
                },
                false,
                250));

#undef DATAVIEWS_LIST
        }

        // TableView
        {
            QString code;
#define DATAVIEWS_TABLE(X) \
    X(auto *detail = new FluentLabel(DEMO_TEXT("选择任意项查看详情", "Select any item to view details"));) \
    X(detail->setStyleSheet("font-size: 12px; opacity: 0.9;");) \
    X(body->addWidget(detail);) \
    X(auto *table = new FluentTableView();) \
    X(auto *tableModel = new QStandardItemModel(8, 2, window);) \
    X(tableModel->setHorizontalHeaderLabels({DEMO_TEXT("名称", "Name"), DEMO_TEXT("值", "Value")});) \
    X(for (int r = 0; r < tableModel->rowCount(); ++r) { tableModel->setItem(r, 0, new QStandardItem(QStringLiteral("Item %1").arg(r + 1))); tableModel->setItem(r, 1, new QStandardItem(QStringLiteral("%1").arg((r + 1) * 7))); }) \
    X(table->setModel(tableModel);) \
    X(table->horizontalHeader()->setStretchLastSection(true);) \
    X(table->setFixedHeight(260);) \
    X(QObject::connect(table->selectionModel(), &QItemSelectionModel::selectionChanged, window, [=]() { const QModelIndex idx = table->currentIndex(); if (idx.isValid()) { detail->setText(QStringLiteral("TableView: row=%1 col=%2").arg(idx.row()).arg(idx.column())); } });) \
    X(body->addWidget(table);)

#define X(line) code += QStringLiteral(#line "\n");
            DATAVIEWS_TABLE(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentTableView"),
                DEMO_TEXT("表格视图（Header/列伸缩/选择行为）", "Table view with header stretching and row selection"),
                DEMO_TEXT("要点：\n"
                          "-horizontalHeader()->setStretchLastSection(true)\n"
                          "-setSelectionBehavior(SelectRows) 常用于表格\n"
                          "-双击单元格可编辑，文本编辑器使用 FluentLineEdit 风格",
                          "Highlights:\n"
                          "-Use horizontalHeader()->setStretchLastSection(true)\n"
                          "-setSelectionBehavior(SelectRows) is common for tables\n"
                          "-Double-click cells to edit them with a FluentLineEdit-style text editor"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    DATAVIEWS_TABLE(X)
#undef X
                },
                true,
                280));

#undef DATAVIEWS_TABLE
        }

        // TableWidget (item-based, supports per-cell widgets)
        {
            QString code;
#define DATAVIEWS_TABLEWIDGET(X) \
    X(auto *detail = new FluentLabel(DEMO_TEXT("使用 setCellWidget() 为单元格嵌入控件，适合少量数据 + 控件交互的场景", "Use setCellWidget() to embed controls per cell — ideal for small datasets that need rich interactions"));) \
    X(detail->setStyleSheet("font-size: 12px; opacity: 0.9;");) \
    X(body->addWidget(detail);) \
    X(auto wrap = [](QWidget *w, bool center) {) \
    X(    auto *host = new QWidget(); host->setAttribute(Qt::WA_TranslucentBackground);) \
    X(    auto *lay = new QHBoxLayout(host); lay->setContentsMargins(10, 4, 10, 4); lay->setSpacing(0);) \
    X(    if (center) { lay->addStretch(1); lay->addWidget(w); lay->addStretch(1); } else { lay->addWidget(w); }) \
    X(    return host;) \
    X(};) \
    X(auto *table = new FluentTableWidget(5, 4);) \
    X(table->setHorizontalHeaderLabels({DEMO_TEXT("名称", "Name"), DEMO_TEXT("启用", "Enabled"), DEMO_TEXT("数量", "Qty"), DEMO_TEXT("等级", "Level")});) \
    X(table->horizontalHeader()->setStretchLastSection(true);) \
    X(table->setColumnWidth(0, 140);) \
    X(table->setColumnWidth(1, 90);) \
    X(table->setColumnWidth(2, 140);) \
    X(table->verticalHeader()->setDefaultSectionSize(46);) \
    X(table->setFixedHeight(290);) \
    X(for (int r = 0; r < 5; ++r) {) \
    X(    auto *nameItem = new QTableWidgetItem(QStringLiteral("Row %1").arg(r + 1));) \
    X(    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);) \
    X(    table->setItem(r, 0, nameItem);) \
    X(    auto *cb = new FluentCheckBox(); cb->setChecked(r % 2 == 0); table->setCellWidget(r, 1, wrap(cb, true));) \
    X(    auto *sb = new FluentSpinBox(); sb->setRange(0, 999); sb->setValue((r + 1) * 10); sb->setFixedWidth(110); table->setCellWidget(r, 2, wrap(sb, false));) \
    X(    auto *combo = new FluentComboBox(); combo->addItems({QStringLiteral("Low"), QStringLiteral("Mid"), QStringLiteral("High")}); combo->setCurrentIndex(r % 3); table->setCellWidget(r, 3, wrap(combo, false));) \
    X(}) \
    X(body->addWidget(table);)

#define X(line) code += QStringLiteral(#line "\n");
            DATAVIEWS_TABLEWIDGET(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentTableWidget"),
                DEMO_TEXT("基于项的表格（QTableWidget 风格，支持 setCellWidget）", "Item-based table (QTableWidget style) with per-cell widget support"),
                DEMO_TEXT("要点：\n"
                          "-FluentTableWidget(rows, cols) 直接构造（不需要外部 model）\n"
                          "-setItem(r, c, new QTableWidgetItem(text)) 设置文本单元格\n"
                          "-setCellWidget(r, c, widget) 嵌入任意控件：FluentCheckBox / FluentSpinBox / FluentComboBox / FluentToggleSwitch …\n"
                          "-与 FluentTableView 风格一致：行 hover/选中描边、Fluent 滚动条\n"
                          "-适用场景：少量数据 + 行内交互（属性表、编辑面板）；海量数据请用 FluentTableView + 自定义 delegate",
                          "Highlights:\n"
                          "-FluentTableWidget(rows, cols) constructs directly (no external model needed)\n"
                          "-Use setItem(r, c, new QTableWidgetItem(text)) for plain cells\n"
                          "-Use setCellWidget(r, c, widget) to embed any control: FluentCheckBox / FluentSpinBox / FluentComboBox / FluentToggleSwitch …\n"
                          "-Shares FluentTableView's row hover/selection visuals and Fluent scroll bars\n"
                          "-Best for small datasets with in-row interaction (property sheets, editor panes). For large datasets use FluentTableView with custom delegates."),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    DATAVIEWS_TABLEWIDGET(X)
#undef X
                },
                true,
                300));

#undef DATAVIEWS_TABLEWIDGET
        }

        // TreeView
        {
            QString code;
#define DATAVIEWS_TREE(X) \
    X(auto *detail = new FluentLabel(DEMO_TEXT("选择任意项查看详情", "Select any item to view details"));) \
    X(detail->setStyleSheet("font-size: 12px; opacity: 0.9;");) \
    X(body->addWidget(detail);) \
    X(auto *tree = new FluentTreeView();) \
    X(auto *treeModel = new QStandardItemModel(window);) \
    X(treeModel->setHorizontalHeaderLabels({DEMO_TEXT("层级", "Hierarchy"), DEMO_TEXT("说明", "Description")});) \
    X(auto *rootA = new QStandardItem(QStringLiteral("Root A"));) \
    X(rootA->appendRow({new QStandardItem(QStringLiteral("A-1")), new QStandardItem(QStringLiteral("Leaf"))});) \
    X(auto *a2 = new QStandardItem(QStringLiteral("A-2"));) \
    X(a2->appendRow({new QStandardItem(QStringLiteral("A-2-1")), new QStandardItem(QStringLiteral("Leaf"))});) \
    X(rootA->appendRow({a2, new QStandardItem(QStringLiteral("Branch"))});) \
    X(auto *rootB = new QStandardItem(QStringLiteral("Root B"));) \
    X(rootB->appendRow({new QStandardItem(QStringLiteral("B-1")), new QStandardItem(QStringLiteral("Leaf"))});) \
    X(treeModel->appendRow({rootA, new QStandardItem(QStringLiteral("Group"))});) \
    X(treeModel->appendRow({rootB, new QStandardItem(QStringLiteral("Group"))});) \
    X(tree->setModel(treeModel);) \
    X(tree->expandAll();)

#define DATAVIEWS_TREE_TAIL(X) \
    X(tree->setFixedHeight(280);) \
    X(QObject::connect(tree->selectionModel(), &QItemSelectionModel::selectionChanged, window, [=]() { const QModelIndex idx = tree->currentIndex(); if (idx.isValid()) { detail->setText(QStringLiteral("TreeView: %1").arg(idx.data().toString())); } });) \
    X(body->addWidget(tree);)

#define X(line) code += QStringLiteral(#line "\n");
            DATAVIEWS_TREE(X)
            DATAVIEWS_TREE_TAIL(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentTreeView"),
                DEMO_TEXT("树视图（层级数据、展开/折叠）", "Tree view for hierarchical data with expand/collapse"),
                DEMO_TEXT("要点：\n"
                          "-QStandardItemModel 构建树\n"
                          "-expandAll()/collapseAll()\n"
                          "-selectionModel 监听当前项\n"
                          "-双击文本项可编辑",
                          "Highlights:\n"
                          "-Build the tree with QStandardItemModel\n"
                          "-Use expandAll() and collapseAll()\n"
                          "-Use selectionModel() to track the current item\n"
                          "-Double-click text items to edit them"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    DATAVIEWS_TREE(X)
                    DATAVIEWS_TREE_TAIL(X)
#undef X
                },
                true,
                320));

#undef DATAVIEWS_TREE
#undef DATAVIEWS_TREE_TAIL
        }
    });
}

} // namespace Demo::Pages
