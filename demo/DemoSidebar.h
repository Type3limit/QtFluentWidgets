#pragma once

#include "Fluent/FluentScrollArea.h"
#include "Fluent/FluentToast.h"

#include <QPointer>

class QStringListModel;

namespace Fluent {
class FluentListView;
}

namespace Demo {

class DemoSidebar final : public Fluent::FluentScrollArea
{
    Q_OBJECT
public:
    explicit DemoSidebar(QWidget *hostWindow, QWidget *parent = nullptr, bool showNavigation = true);

    void setCurrentPage(int index);
    int currentPage() const;

    Fluent::FluentToast::Position toastPosition() const;

signals:
    void currentPageChanged(int index);
    void toastPositionChanged(Fluent::FluentToast::Position position);

private:
    QPointer<QWidget> m_hostWindow;
    QStringListModel *m_navModel = nullptr;
    Fluent::FluentListView *m_navView = nullptr;

    Fluent::FluentToast::Position m_toastPosition = Fluent::FluentToast::Position::BottomRight;
};

} // namespace Demo
