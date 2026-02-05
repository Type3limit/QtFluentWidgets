#pragma once

#include <QGroupBox>

namespace Fluent {

class FluentGroupBox final : public QGroupBox
{
    Q_OBJECT
public:
    explicit FluentGroupBox(QWidget *parent = nullptr);
    explicit FluentGroupBox(const QString &title, QWidget *parent = nullptr);

protected:
    void changeEvent(QEvent *event) override;

private:
    void applyTheme();
};

} // namespace Fluent
