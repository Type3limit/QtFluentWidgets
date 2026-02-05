#pragma once

#include <QLabel>

namespace Fluent {

class FluentLabel final : public QLabel
{
    Q_OBJECT
public:
    explicit FluentLabel(QWidget *parent = nullptr);
    explicit FluentLabel(const QString &text, QWidget *parent = nullptr);

protected:
    void changeEvent(QEvent *event) override;

private:
    void applyTheme();
};

} // namespace Fluent
