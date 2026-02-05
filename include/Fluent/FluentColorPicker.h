#pragma once

#include <QWidget>
#include <QColor>

class QLineEdit;
class QPushButton;

namespace Fluent {

class FluentColorPicker final : public QWidget
{
    Q_OBJECT
public:
    explicit FluentColorPicker(QWidget *parent = nullptr);

    QColor color() const;
    void setColor(const QColor &color);

signals:
    void colorChanged(const QColor &color);

private slots:
    void openDialog();

private:
    void applyTheme();
    void updatePreview();

    QColor m_color;
    QLineEdit *m_preview;
    QPushButton *m_button;
};

} // namespace Fluent
