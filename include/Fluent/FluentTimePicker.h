#pragma once

#include <QString>
#include <QTimeEdit>

#include "Fluent/FluentQtCompat.h"

class QFocusEvent;
class QPaintEvent;
class QResizeEvent;
class QVariantAnimation;
class QLineEdit;

namespace Fluent {

namespace Detail {
class FluentWheelPickerPopup;
}

class FluentTimePicker final : public QTimeEdit
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverLevel READ hoverLevel WRITE setHoverLevel)
    Q_PROPERTY(qreal focusLevel READ focusLevel WRITE setFocusLevel)
    Q_PROPERTY(bool use24HourClock READ use24HourClock WRITE setUse24HourClock)
    Q_PROPERTY(int minuteIncrement READ minuteIncrement WRITE setMinuteIncrement)
    Q_PROPERTY(QString hourPlaceholderText READ hourPlaceholderText WRITE setHourPlaceholderText)
    Q_PROPERTY(QString minutePlaceholderText READ minutePlaceholderText WRITE setMinutePlaceholderText)
    Q_PROPERTY(QString anteMeridiemText READ anteMeridiemText WRITE setAnteMeridiemText)
    Q_PROPERTY(QString postMeridiemText READ postMeridiemText WRITE setPostMeridiemText)
public:
    explicit FluentTimePicker(QWidget *parent = nullptr);
    ~FluentTimePicker() override;

    qreal hoverLevel() const;
    void setHoverLevel(qreal value);

    qreal focusLevel() const;
    void setFocusLevel(qreal value);

    void setTime(const QTime &time);
    QTime time() const;

    void clearTime();
    bool hasTime() const;

    void setUse24HourClock(bool enabled);
    bool use24HourClock() const;

    void setMinuteIncrement(int minutes);
    int minuteIncrement() const;

    void setHourPlaceholderText(const QString &text);
    QString hourPlaceholderText() const;

    void setMinutePlaceholderText(const QString &text);
    QString minutePlaceholderText() const;

    void setAnteMeridiemText(const QString &text);
    QString anteMeridiemText() const;

    void setPostMeridiemText(const QString &text);
    QString postMeridiemText() const;

protected:
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(FluentEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    QSize sizeHint() const override;

private:
    void applyTheme();
    void startHoverAnimation(qreal endValue);
    void startFocusAnimation(qreal endValue);
    void ensureEditorHidden();
    void togglePopup();
    void showPopup();
    void hidePopup();
    bool isPopupVisible() const;
    void rebuildPopupColumns(const QTime &selectedTime);
    QTime popupSeedTime() const;
    QTime popupTimeFromColumns() const;
    QString periodText(bool postMeridiem) const;
    QString displayTextForIndex(int fieldIndex) const;

    qreal m_hoverLevel = 0.0;
    qreal m_focusLevel = 0.0;
    QVariantAnimation *m_hoverAnim = nullptr;
    QVariantAnimation *m_focusAnim = nullptr;
    Detail::FluentWheelPickerPopup *m_popup = nullptr;
    QLineEdit *m_editor = nullptr;
    bool m_hasTime = false;
    bool m_use24HourClock = false;
    int m_minuteIncrement = 1;
    bool m_blockTimeChangedMarker = false;
    QString m_hourPlaceholderText;
    QString m_minutePlaceholderText;
    QString m_anteMeridiemText;
    QString m_postMeridiemText;
};

} // namespace Fluent
