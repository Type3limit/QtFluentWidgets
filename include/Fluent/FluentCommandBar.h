#pragma once

#include "Fluent/FluentExport.h"

#include <QHash>
#include <QPointer>
#include <QSet>
#include <QWidget>

class QAction;
class QHBoxLayout;
class QIcon;
class QResizeEvent;

namespace Fluent {

class FluentMenu;
class FluentToolButton;

class FLUENT_EXPORT FluentCommandBar final : public QWidget
{
    Q_OBJECT
public:
    explicit FluentCommandBar(QWidget *parent = nullptr);

    QAction *addAction(const QString &text);
    QAction *addAction(const QIcon &icon, const QString &text);
    void addCommand(QAction *action);
    void addSeparator();
    void addWidget(QWidget *widget);
    void clear();

    QSize iconSize() const;
    void setIconSize(const QSize &size);

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QWidget *createButtonForAction(QAction *action);
    void syncActionWidget(QAction *action);
    void ensureOverflowButton();
    void updateOverflow();
    int preferredWidthFor(const QSet<QAction *> &overflowed, bool overflowVisible) const;
    QAction *actionForWidget(QWidget *widget) const;
    bool hasVisibleCommandAround(int index, int direction) const;

    QHBoxLayout *m_layout = nullptr;
    QSize m_iconSize = QSize(16, 16);
    QList<QPointer<QAction>> m_ownedActions;
    QList<QPointer<QAction>> m_actionOrder;
    QHash<QAction *, QPointer<QWidget>> m_actionWidgets;
    FluentToolButton *m_overflowButton = nullptr;
    FluentMenu *m_overflowMenu = nullptr;
    bool m_updatingOverflow = false;
};

} // namespace Fluent
