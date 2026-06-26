#pragma once

#include "Fluent/FluentExport.h"

#include <QStatusBar>

namespace Fluent {

class FLUENT_EXPORT FluentStatusBar final : public QStatusBar
{
    Q_OBJECT
public:
    explicit FluentStatusBar(QWidget *parent = nullptr);

protected:
    void changeEvent(QEvent *event) override;

private:
    void applyTheme();
};

} // namespace Fluent
