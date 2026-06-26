#pragma once

#include "Fluent/FluentExport.h"

#include <QBasicTimer>
#include <QProgressBar>

class QPropertyAnimation;

namespace Fluent {

class FLUENT_EXPORT FluentProgressRing final : public QProgressBar
{
    Q_OBJECT
    Q_PROPERTY(qreal displayValue READ displayValue WRITE setDisplayValue)
    Q_PROPERTY(qreal rotationAngle READ rotationAngle WRITE setRotationAngle)
    Q_PROPERTY(qreal ringWidth READ ringWidth WRITE setRingWidth)
    Q_PROPERTY(bool indeterminate READ isIndeterminate WRITE setIndeterminate)

public:
    explicit FluentProgressRing(QWidget *parent = nullptr);

    qreal displayValue() const;
    void setDisplayValue(qreal value);

    qreal rotationAngle() const;
    void setRotationAngle(qreal angle);

    qreal ringWidth() const;
    void setRingWidth(qreal width);

    bool isIndeterminate() const;
    void setIndeterminate(bool indeterminate);

protected:
    void changeEvent(QEvent *event) override;
    bool event(QEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

private:
    void applyTheme();
    void syncSpinTimer();

    qreal m_displayValue = 0.0;
    qreal m_rotationAngle = 0.0;
    qreal m_ringWidth = 4.0;
    bool m_indeterminate = false;
    QBasicTimer m_spinTimer;
    int m_spinTimerIntervalMs = 0;
    QPropertyAnimation *m_valueAnim = nullptr;
};

} // namespace Fluent
