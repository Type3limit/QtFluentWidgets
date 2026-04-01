#include "Fluent/FluentAngleSelector.h"

#include "Fluent/FluentDial.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentSpinBox.h"

#include <QHBoxLayout>

namespace Fluent {

FluentAngleSelector::FluentAngleSelector(QWidget *parent)
    : QWidget(parent)
{
    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(6);

    auto *label = new FluentLabel(tr(u8"角度："), this);
    label->setStyleSheet(QStringLiteral("font-size: 12px;"));
    m_label = label;

    auto *dial = new FluentDial(this);
    dial->setValue(m_minimum);
    m_dial = dial;

    auto *spin = new FluentSpinBox(this);
    spin->setRange(m_minimum, m_maximum);
    spin->setWrapping(m_wrapping);
    spin->setSuffix(QStringLiteral("°"));
    spin->setFixedWidth(84);
    m_spin = spin;

    row->addWidget(label);
    row->addWidget(dial);
    row->addWidget(spin);
    row->addStretch(1);

    connect(dial, &FluentDial::valueChanged, this, [this](int value) {
        setValueInternal(value, true);
    });
    connect(spin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        setValueInternal(value, true);
    });

    setValueInternal(m_minimum, false);
}

int FluentAngleSelector::value() const
{
    return m_spin ? m_spin->value() : m_minimum;
}

void FluentAngleSelector::setValue(int value)
{
    setValueInternal(value, false);
}

void FluentAngleSelector::setRange(int minimum, int maximum)
{
    if (minimum > maximum)
        qSwap(minimum, maximum);

    m_minimum = minimum;
    m_maximum = maximum;

    if (m_spin) {
        m_spin->setRange(m_minimum, m_maximum);
        m_spin->setWrapping(m_wrapping && (m_maximum > m_minimum));
    }

    setValueInternal(value(), false);
}

void FluentAngleSelector::setWrapping(bool wrapping)
{
    if (m_wrapping == wrapping)
        return;
    m_wrapping = wrapping;
    if (m_spin)
        m_spin->setWrapping(m_wrapping && (m_maximum > m_minimum));
}

QString FluentAngleSelector::labelText() const
{
    return m_label ? m_label->text() : QString();
}

void FluentAngleSelector::setLabelText(const QString &text)
{
    if (m_label)
        m_label->setText(text);
}

QString FluentAngleSelector::suffix() const
{
    return m_spin ? m_spin->suffix() : QString();
}

void FluentAngleSelector::setSuffix(const QString &suffix)
{
    if (m_spin)
        m_spin->setSuffix(suffix);
}

bool FluentAngleSelector::dialVisible() const
{
    return m_dial && m_dial->isVisible();
}

void FluentAngleSelector::setDialVisible(bool visible)
{
    if (m_dial)
        m_dial->setVisible(visible);
}

bool FluentAngleSelector::labelVisible() const
{
    return m_label && m_label->isVisible();
}

void FluentAngleSelector::setLabelVisible(bool visible)
{
    if (m_label)
        m_label->setVisible(visible);
}

bool FluentAngleSelector::spinBoxVisible() const
{
    return m_spin && m_spin->isVisible();
}

void FluentAngleSelector::setSpinBoxVisible(bool visible)
{
    if (m_spin)
        m_spin->setVisible(visible);
}

int FluentAngleSelector::normalizeToRange(int value) const
{
    if (m_minimum >= m_maximum)
        return m_minimum;

    if (!m_wrapping)
        return qBound(m_minimum, value, m_maximum);

    const int span = (m_maximum - m_minimum) + 1;
    int normalized = (value - m_minimum) % span;
    if (normalized < 0)
        normalized += span;
    return m_minimum + normalized;
}

void FluentAngleSelector::setValueInternal(int value, bool emitSignal)
{
    const int normalized = normalizeToRange(value);
    const int current = this->value();
    if (current == normalized)
        return;

    if (m_spin) {
        const bool blocked = m_spin->blockSignals(true);
        m_spin->setValue(normalized);
        m_spin->blockSignals(blocked);
    }
    if (m_dial) {
        const bool blocked = m_dial->blockSignals(true);
        m_dial->setValue(normalized);
        m_dial->blockSignals(blocked);
    }

    if (emitSignal)
        emit valueChanged(normalized);
}

} // namespace Fluent

