#pragma once

#include <QTabWidget>

namespace Fluent {

class FluentTabWidget final : public QTabWidget
{
    Q_OBJECT
public:
    explicit FluentTabWidget(QWidget *parent = nullptr);

protected:
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void applyTheme();

    QWidget *m_frameOverlay = nullptr;
};

} // namespace Fluent
