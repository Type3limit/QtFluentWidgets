#pragma once

#include "Fluent/FluentExport.h"

#include <QPointer>
#include <QWidget>

class QAction;
class QHBoxLayout;
class QMenu;

namespace Fluent {

class FluentButton;

class FLUENT_EXPORT FluentSplitButton final : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(bool primary READ isPrimary WRITE setPrimary)

public:
    explicit FluentSplitButton(QWidget *parent = nullptr);
    explicit FluentSplitButton(const QString &text, QWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);

    bool isPrimary() const;
    void setPrimary(bool primary);

    QAction *defaultAction() const;
    void setDefaultAction(QAction *action);

    QMenu *menu() const;
    void setMenu(QMenu *menu);

    FluentButton *mainButton() const;
    FluentButton *dropDownButton() const;

signals:
    void clicked();
    void triggered(QAction *action);

private slots:
    void showMenu();

private:
    void syncDefaultAction();

    QHBoxLayout *m_layout = nullptr;
    FluentButton *m_mainButton = nullptr;
    FluentButton *m_dropDownButton = nullptr;
    QPointer<QAction> m_defaultAction;
    QPointer<QMenu> m_menu;
    bool m_primary = false;
};

} // namespace Fluent
