#pragma once

#include "Fluent/FluentLineEdit.h"

#include <QAbstractItemModel>
#include <QLineEdit>
#include <QStyleOptionViewItem>

namespace Fluent::Detail {

inline void prepareFluentItemEditor(FluentLineEdit *editor)
{
    if (!editor) {
        return;
    }
    editor->setAutoFillBackground(false);
    editor->setMinimumHeight(0);
    editor->setFocusLevel(1.0);
}

inline QWidget *fluentizeTextEditor(QWidget *editor, QWidget *parent)
{
    if (!editor || !qobject_cast<QLineEdit *>(editor)) {
        return editor;
    }

    delete editor;
    auto *fluentEditor = new FluentLineEdit(parent);
    prepareFluentItemEditor(fluentEditor);
    return fluentEditor;
}

inline bool isFluentTextEditor(QWidget *editor)
{
    return qobject_cast<FluentLineEdit *>(editor) != nullptr;
}

inline bool setFluentEditorData(QWidget *editor, const QModelIndex &index)
{
    auto *lineEdit = qobject_cast<FluentLineEdit *>(editor);
    if (!lineEdit) {
        return false;
    }

    lineEdit->setText(index.data(Qt::EditRole).toString());
    lineEdit->selectAll();
    return true;
}

inline bool setFluentModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index)
{
    auto *lineEdit = qobject_cast<FluentLineEdit *>(editor);
    if (!lineEdit || !model) {
        return false;
    }

    model->setData(index, lineEdit->text(), Qt::EditRole);
    return true;
}

inline QRect fluentEditorRect(const QStyleOptionViewItem &option, int horizontalInset, int verticalInset)
{
    const QRect rect = option.rect.adjusted(horizontalInset, verticalInset, -horizontalInset, -verticalInset);
    return rect.isValid() ? rect : option.rect;
}

} // namespace Fluent::Detail
