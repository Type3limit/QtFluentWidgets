#pragma once

#include <QColor>
#include <QProgressBar>

class QPropertyAnimation;

namespace Fluent {

class FluentProgressBar final : public QProgressBar
{
    Q_OBJECT
    Q_PROPERTY(qreal displayValue READ displayValue WRITE setDisplayValue)

public:
    enum class TextPosition {
        None,
        Left,
        Center,
        Right,
    };
    Q_ENUM(TextPosition)

    Q_PROPERTY(TextPosition textPosition READ textPosition WRITE setTextPosition)
public:
    explicit FluentProgressBar(QWidget *parent = nullptr);

    qreal displayValue() const;
    void setDisplayValue(qreal value);

    TextPosition textPosition() const;
    void setTextPosition(TextPosition pos);

    QColor textColor() const;
    void setTextColor(const QColor &color);

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void applyTheme();

    qreal m_displayValue = 0.0;
    QPropertyAnimation *m_valueAnim = nullptr;

    TextPosition m_textPosition = TextPosition::Right;
    QColor m_textColor;
};

} // namespace Fluent
