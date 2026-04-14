#pragma once

#include <QDateEdit>
#include <QString>
#include "Fluent/FluentQtCompat.h"

class QFocusEvent;
class QPaintEvent;
class QVariantAnimation;
class QKeyEvent;
class QMouseEvent;

namespace Fluent {

class FluentCalendarPopup;

class FluentCalendarPicker final : public QDateEdit
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverLevel READ hoverLevel WRITE setHoverLevel)
    Q_PROPERTY(qreal focusLevel READ focusLevel WRITE setFocusLevel)
    Q_PROPERTY(QString todayText READ todayText WRITE setTodayText)
public:
    explicit FluentCalendarPicker(QWidget *parent = nullptr);

    qreal hoverLevel() const;
    void setHoverLevel(qreal value);

    qreal focusLevel() const;
    void setFocusLevel(qreal value);

    void setTodayText(const QString &text);
    QString todayText() const;

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(FluentEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void applyTheme();
    void togglePopup();
    void showPopup();
    void hidePopup();
    bool isPopupVisible() const;
    void startHoverAnimation(qreal endValue);
    void startFocusAnimation(qreal endValue);

    qreal m_hoverLevel = 0.0;
    qreal m_focusLevel = 0.0;
    QVariantAnimation *m_hoverAnim = nullptr;
    QVariantAnimation *m_focusAnim = nullptr;
    FluentCalendarPopup *m_popup = nullptr;
    QString m_todayText;
};

} // namespace Fluent
