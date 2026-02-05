#pragma once

#include <QToolBar>

class QActionEvent;

namespace Fluent {

class FluentToolBar final : public QToolBar
{
    Q_OBJECT
public:
    explicit FluentToolBar(const QString &title = QString(), QWidget *parent = nullptr);

protected:
    void changeEvent(QEvent *event) override;
    void actionEvent(QActionEvent *event) override;

private:
    void applyTheme();
};

} // namespace Fluent
