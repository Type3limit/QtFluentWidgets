#pragma once

#include <QWidget>

class QString;

namespace Fluent {

class FluentDial;
class FluentSpinBox;
class FluentLabel;

class FluentAngleSelector final : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged USER true)
    Q_PROPERTY(bool dialVisible READ dialVisible WRITE setDialVisible)
    Q_PROPERTY(bool labelVisible READ labelVisible WRITE setLabelVisible)
    Q_PROPERTY(bool spinBoxVisible READ spinBoxVisible WRITE setSpinBoxVisible)
public:
    explicit FluentAngleSelector(QWidget *parent = nullptr);

    int value() const;
    void setValue(int value);

    int minimum() const { return m_minimum; }
    int maximum() const { return m_maximum; }
    void setRange(int minimum, int maximum);

    bool wrapping() const { return m_wrapping; }
    void setWrapping(bool wrapping);

    QString labelText() const;
    void setLabelText(const QString &text);

    QString suffix() const;
    void setSuffix(const QString &suffix);

    bool dialVisible() const;
    void setDialVisible(bool visible);

    bool labelVisible() const;
    void setLabelVisible(bool visible);

    bool spinBoxVisible() const;
    void setSpinBoxVisible(bool visible);

    FluentDial *dial() const { return m_dial; }
    FluentSpinBox *spinBox() const { return m_spin; }

signals:
    void valueChanged(int value);

private:
    int normalizeToRange(int value) const;
    void setValueInternal(int value, bool emitSignal);

    int m_minimum = 0;
    int m_maximum = 359;
    bool m_wrapping = true;

    FluentLabel *m_label = nullptr;
    FluentDial *m_dial = nullptr;
    FluentSpinBox *m_spin = nullptr;
};

} // namespace Fluent

