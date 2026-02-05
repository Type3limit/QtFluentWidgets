#pragma once

#include <QObject>
#include <QPointer>

class QDialog;
class QColor;

namespace Fluent::ColorPicker {

class EyeDropperController final : public QObject
{
    Q_OBJECT
public:
    explicit EyeDropperController(QObject *parent = nullptr);
    ~EyeDropperController() override;

signals:
    void hovered(const QColor &color);
    void picked(const QColor &color);
    void canceled();

private:
    void closeAll();

    QPointer<QDialog> m_overlay;
    bool m_finished = false;
};

} // namespace Fluent::ColorPicker
