#pragma once

#include <QSpinBox>
#include <QVariantAnimation>

class QResizeEvent;
class QPaintEvent;
class QLineEdit;

namespace Fluent {

class FluentSpinBox final : public QSpinBox
{
    Q_OBJECT
public:
    explicit FluentSpinBox(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void applyTheme();
    void ensureEditor();
    void startStepperAnimation(qreal endValue);

    enum class ButtonPart {
        None,
        Up,
        Down
    };

    ButtonPart hitTestButton(const QPoint &pos) const;

    QVariantAnimation *m_hoverAnim = nullptr;
    QVariantAnimation *m_focusAnim = nullptr;
    QVariantAnimation *m_stepperAnim = nullptr;
    qreal m_hoverLevel = 0.0;
    qreal m_focusLevel = 0.0;
    qreal m_stepperLevel = 0.0;

    ButtonPart m_hoverButton = ButtonPart::None;
    ButtonPart m_pressedButton = ButtonPart::None;

    QLineEdit *m_editor = nullptr;
};

class FluentDoubleSpinBox final : public QDoubleSpinBox
{
    Q_OBJECT
public:
    explicit FluentDoubleSpinBox(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void applyTheme();
    void ensureEditor();
    void startStepperAnimation(qreal endValue);

    enum class ButtonPart {
        None,
        Up,
        Down
    };

    ButtonPart hitTestButton(const QPoint &pos) const;

    QVariantAnimation *m_hoverAnim = nullptr;
    QVariantAnimation *m_focusAnim = nullptr;
    QVariantAnimation *m_stepperAnim = nullptr;
    qreal m_hoverLevel = 0.0;
    qreal m_focusLevel = 0.0;
    qreal m_stepperLevel = 0.0;

    ButtonPart m_hoverButton = ButtonPart::None;
    ButtonPart m_pressedButton = ButtonPart::None;

    QLineEdit *m_editor = nullptr;
};

} // namespace Fluent
