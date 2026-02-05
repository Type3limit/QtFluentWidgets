#pragma once

#include <QWidget>

class QPaintEvent;

namespace Fluent {

class FluentWidget : public QWidget
{
    Q_OBJECT
public:
    enum class BackgroundRole {
        WindowBackground,
        Surface,
        Transparent,
    };
    Q_ENUM(BackgroundRole)

    explicit FluentWidget(QWidget *parent = nullptr);

    BackgroundRole backgroundRole() const;
    void setBackgroundRole(BackgroundRole role);

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void applyTheme();

    BackgroundRole m_backgroundRole = BackgroundRole::WindowBackground;
};

} // namespace Fluent
