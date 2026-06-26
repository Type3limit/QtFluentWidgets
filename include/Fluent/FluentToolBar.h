#pragma once

#include "Fluent/FluentExport.h"

#include <QHash>
#include <QMetaObject>
#include <QPointer>
#include <QToolBar>

class QAction;
class QActionEvent;
class QWidgetAction;

namespace Fluent {

class FLUENT_EXPORT FluentToolBar final : public QToolBar
{
    Q_OBJECT
public:
    explicit FluentToolBar(const QString &title = QString(), QWidget *parent = nullptr);
    ~FluentToolBar() override;

    void insertAction(QAction *before, QAction *action);
    void removeAction(QAction *action);

protected:
    void changeEvent(QEvent *event) override;
    void actionEvent(QActionEvent *event) override;

private:
    void applyTheme();
    QWidgetAction *wrapperForAction(QAction *action) const;
    void unregisterWrapper(QWidgetAction *wrapper);
    void wrapPlainAction(QAction *action, QAction *before);

    QHash<QAction *, QPointer<QWidgetAction>> m_wrappedActions;
    QHash<QWidgetAction *, QPointer<QAction>> m_wrapperActions;
    QHash<QAction *, QMetaObject::Connection> m_actionDestroyConnections;
    bool m_wrappingAction = false;
};

} // namespace Fluent
