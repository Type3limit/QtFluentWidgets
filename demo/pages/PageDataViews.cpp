#include "PageDataViews.h"

#include "../DemoHelpers.h"
#include "Fluent/FluentCard.h"

#include <QHeaderView>
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
#include "Fluent/FluentTreeView.h"

namespace Demo::Pages {

using namespace Fluent;

QWidget *createDataViewsPage(FluentMainWindow *window)
{
    return Demo::makePage([&](QVBoxLayout *page) {
        auto s = Demo::makeSection(QStringLiteral("数据视图"),
                                   QStringLiteral("ListView / TableView / TreeView（选择变化联动到详情区）"));

        page->addWidget(s.card);

        // ListView
        {
            QString code;
#define DATAVIEWS_LIST(X) \
    X(auto *detail = new FluentLabel(QStringLiteral("选择任意项查看详情"));) \
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
                QStringLiteral("列表视图（QAbstractItemView 风格）"),
                QStringLiteral("要点：\n"
                               "-setModel() 绑定 QStringListModel/QStandardItemModel\n"
                               "-selectionModel()->selectionChanged 监听选择变化\n"
                               "-适合轻量列表/侧栏"),
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
    X(auto *detail = new FluentLabel(QStringLiteral("选择任意项查看详情"));) \
    X(detail->setStyleSheet("font-size: 12px; opacity: 0.9;");) \
    X(body->addWidget(detail);) \
    X(auto *table = new FluentTableView();) \
    X(auto *tableModel = new QStandardItemModel(8, 2, window);) \
    X(tableModel->setHorizontalHeaderLabels({QStringLiteral("名称"), QStringLiteral("值")});) \
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
                QStringLiteral("表格视图（Header/列伸缩/选择行为）"),
                QStringLiteral("要点：\n"
                               "-horizontalHeader()->setStretchLastSection(true)\n"
                               "-setSelectionBehavior(SelectRows) 常用于表格\n"
                               "-setEditTriggers(NoEditTriggers) 仅展示"),
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

        // TreeView
        {
            QString code;
#define DATAVIEWS_TREE(X) \
    X(auto *detail = new FluentLabel(QStringLiteral("选择任意项查看详情"));) \
    X(detail->setStyleSheet("font-size: 12px; opacity: 0.9;");) \
    X(body->addWidget(detail);) \
    X(auto *tree = new FluentTreeView();) \
    X(auto *treeModel = new QStandardItemModel(window);) \
    X(treeModel->setHorizontalHeaderLabels({QStringLiteral("层级"), QStringLiteral("说明")});) \
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
                QStringLiteral("树视图（层级数据、展开/折叠）"),
                QStringLiteral("要点：\n"
                               "-QStandardItemModel 构建树\n"
                               "-expandAll()/collapseAll()\n"
                               "-selectionModel 监听当前项"),
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
