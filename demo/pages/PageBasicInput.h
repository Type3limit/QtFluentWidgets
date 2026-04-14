#pragma once

#include <functional>

class QWidget;

namespace Fluent { class FluentMainWindow; }

namespace Demo::Pages {

QWidget *createBasicInputPage(Fluent::FluentMainWindow *window, const std::function<void(int)> &jumpTo);

} // namespace Demo::Pages