#pragma once

#include "Fluent/FluentExport.h"

#include <QGroupBox>
#include <QRect>

namespace Fluent {

class FLUENT_EXPORT FluentGroupBox final : public QGroupBox
{
    Q_OBJECT
public:
    explicit FluentGroupBox(QWidget *parent = nullptr);
    explicit FluentGroupBox(const QString &title, QWidget *parent = nullptr);

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void applyTheme();
    QRect titleRect() const;
    QRect checkBoxRect() const;
};

} // namespace Fluent
