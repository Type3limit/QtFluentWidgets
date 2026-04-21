#pragma once

#include <QIcon>

#include "Fluent/FluentButton.h"

namespace Fluent {

class FluentIconButton final : public FluentButton
{
    Q_OBJECT
    Q_PROPERTY(int buttonExtent READ buttonExtent WRITE setButtonExtent)
public:
    explicit FluentIconButton(QWidget *parent = nullptr);
    explicit FluentIconButton(const QIcon &icon, QWidget *parent = nullptr);

    int buttonExtent() const;
    void setButtonExtent(int extent);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private:
    int effectiveExtent() const;

    int m_buttonExtent = 0;
};

} // namespace Fluent