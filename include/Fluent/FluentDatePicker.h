#pragma once

#include <QDate>
#include <QFlags>
#include <QString>
#include <QVector>
#include <QWidget>

#include "Fluent/FluentQtCompat.h"

class QVariantAnimation;

namespace Fluent {

namespace Detail {
class FluentWheelPickerPopup;
}

class FluentDatePicker final : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverLevel READ hoverLevel WRITE setHoverLevel)
    Q_PROPERTY(qreal focusLevel READ focusLevel WRITE setFocusLevel)
    Q_PROPERTY(QDate date READ date WRITE setDate NOTIFY dateChanged USER true)
    Q_PROPERTY(QString monthPlaceholderText READ monthPlaceholderText WRITE setMonthPlaceholderText)
    Q_PROPERTY(QString dayPlaceholderText READ dayPlaceholderText WRITE setDayPlaceholderText)
    Q_PROPERTY(QString yearPlaceholderText READ yearPlaceholderText WRITE setYearPlaceholderText)
public:
    enum DatePart {
        MonthPart = 0x1,
        DayPart = 0x2,
        YearPart = 0x4,
    };
    Q_DECLARE_FLAGS(DateParts, DatePart)
    Q_FLAG(DateParts)

    explicit FluentDatePicker(QWidget *parent = nullptr);
    ~FluentDatePicker() override;

    qreal hoverLevel() const;
    void setHoverLevel(qreal value);

    qreal focusLevel() const;
    void setFocusLevel(qreal value);

    void setDate(const QDate &date);
    QDate date() const;

    void clearDate();
    bool hasDate() const;

    void setMinimumDate(const QDate &date);
    QDate minimumDate() const;

    void setMaximumDate(const QDate &date);
    QDate maximumDate() const;

    void setDateRange(const QDate &minimum, const QDate &maximum);

    void setVisibleParts(DateParts parts);
    DateParts visibleParts() const;

    void setMonthDisplayFormat(const QString &format);
    QString monthDisplayFormat() const;

    void setDayDisplayFormat(const QString &format);
    QString dayDisplayFormat() const;

    void setYearDisplayFormat(const QString &format);
    QString yearDisplayFormat() const;

    void setMonthPlaceholderText(const QString &text);
    QString monthPlaceholderText() const;

    void setDayPlaceholderText(const QString &text);
    QString dayPlaceholderText() const;

    void setYearPlaceholderText(const QString &text);
    QString yearPlaceholderText() const;

signals:
    void dateChanged(const QDate &date);

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(FluentEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    QSize sizeHint() const override;

private:
    void applyTheme();
    void startHoverAnimation(qreal endValue);
    void startFocusAnimation(qreal endValue);
    void togglePopup();
    void showPopup();
    void hidePopup();
    bool isPopupVisible() const;

    QVector<DatePart> orderedParts() const;
    int popupColumnIndex(DatePart part) const;
    QDate boundedDate(const QDate &date) const;
    QDate popupSeedDate() const;
    QDate popupDateFromColumns() const;
    void rebuildPopupColumns(const QDate &selectedDate);
    QString displayTextForPart(DatePart part, const QDate &value, bool placeholder) const;

    qreal m_hoverLevel = 0.0;
    qreal m_focusLevel = 0.0;
    QVariantAnimation *m_hoverAnim = nullptr;
    QVariantAnimation *m_focusAnim = nullptr;
    Detail::FluentWheelPickerPopup *m_popup = nullptr;
    QDate m_date;
    QDate m_minimumDate;
    QDate m_maximumDate;
    DateParts m_visibleParts = MonthPart | DayPart | YearPart;
    QString m_monthFormat;
    QString m_dayFormat;
    QString m_yearFormat;
    QString m_monthPlaceholderText;
    QString m_dayPlaceholderText;
    QString m_yearPlaceholderText;
    bool m_syncingPopupColumns = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FluentDatePicker::DateParts)

} // namespace Fluent