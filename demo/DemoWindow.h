#pragma once

#include "Fluent/FluentMainWindow.h"
#include "Fluent/FluentToast.h"

namespace Demo {

class DemoWindow final : public Fluent::FluentMainWindow
{
    Q_OBJECT
public:
    explicit DemoWindow(QWidget *parent = nullptr);

private:
    Fluent::FluentToast::Position m_toastPosition = Fluent::FluentToast::Position::BottomRight;
};

} // namespace Demo
