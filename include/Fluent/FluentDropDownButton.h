#pragma once

#include "Fluent/FluentExport.h"

#include "Fluent/FluentButton.h"

#include <QPointer>

class QAction;
class QIcon;
class QMenu;

namespace Fluent {

class FLUENT_EXPORT FluentDropDownButton : public FluentButton
{
    Q_OBJECT
public:
    explicit FluentDropDownButton(QWidget *parent = nullptr);
    explicit FluentDropDownButton(const QString &text, QWidget *parent = nullptr);

    QMenu *menu() const;
    void setMenu(QMenu *menu);

    QAction *addAction(const QString &text);
    QAction *addAction(const QIcon &icon, const QString &text);

    QSize sizeHint() const override;

public slots:
    void showMenu();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPointer<QMenu> m_menu;
};

} // namespace Fluent
