#pragma once

#include <QWidget>

class QPropertyAnimation;

namespace Fluent {

class FluentToggleSwitch final : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)
    Q_PROPERTY(qreal hoverLevel READ hoverLevel WRITE setHoverLevel)
    Q_PROPERTY(qreal focusLevel READ focusLevel WRITE setFocusLevel)
public:
    explicit FluentToggleSwitch(QWidget *parent = nullptr);
    explicit FluentToggleSwitch(const QString &text, QWidget *parent = nullptr);

    bool isChecked() const;
    void setChecked(bool checked);
    void setText(const QString &text);

    qreal progress() const;
    void setProgress(qreal value);

    qreal hoverLevel() const;
    void setHoverLevel(qreal value);

    qreal focusLevel() const;
    void setFocusLevel(qreal value);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void toggled(bool checked);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void changeEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    void applyTheme();
    void startFocusAnimation(qreal endValue);

    QString m_text;
    bool m_checked = false;
    qreal m_progress = 0.0;
    qreal m_hoverLevel = 0.0;
    qreal m_focusLevel = 0.0;
    QPropertyAnimation *m_progressAnim = nullptr;
    QPropertyAnimation *m_hoverAnim = nullptr;
    QPropertyAnimation *m_focusAnim = nullptr;
};

} // namespace Fluent
