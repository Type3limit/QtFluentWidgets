#pragma once

#include "Fluent/FluentExport.h"

#include <QLabel>

namespace Fluent {

class FLUENT_EXPORT FluentLabel final : public QLabel
{
    Q_OBJECT
public:
    explicit FluentLabel(QWidget *parent = nullptr);
    explicit FluentLabel(const QString &text, QWidget *parent = nullptr);
    void setStyleSheet(const QString &styleSheet);

protected:
    void changeEvent(QEvent *event) override;

private:
    void applyTheme();
};

} // namespace Fluent
