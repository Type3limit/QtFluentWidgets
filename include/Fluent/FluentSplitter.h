#pragma once

#include <QSplitter>

namespace Fluent {

class FluentSplitter final : public QSplitter
{
    Q_OBJECT
public:
    explicit FluentSplitter(Qt::Orientation orientation, QWidget *parent = nullptr);
    explicit FluentSplitter(QWidget *parent = nullptr);

protected:
    QSplitterHandle *createHandle() override;

private:
    void applyTheme();
};

} // namespace Fluent
