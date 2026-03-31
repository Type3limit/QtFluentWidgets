#pragma once

#include <QDate>
#include "Fluent/FluentQtCompat.h"
#include <QString>
#include <QWidget>

class QVariantAnimation;
class QMouseEvent;

namespace Fluent {

class FluentCalendarPopup;

///
/// \brief A Fluent-styled date-range picker widget.
///
/// Layout:  [startPrefix][startDate][startSuffix][separator][endPrefix][endDate][endSuffix]  [v]
///
/// All text pieces are configurable; defaults produce:   yyyy-MM-dd  →  yyyy-MM-dd
///
class FluentDateRangePicker final : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverLevel READ hoverLevel WRITE setHoverLevel)

public:
    explicit FluentDateRangePicker(QWidget *parent = nullptr);

    // ── Date range ─────────────────────────────────────────────────────────
    QDate startDate() const;
    QDate endDate()   const;
    void  setDateRange(const QDate &start, const QDate &end);

    // ── Date display format ────────────────────────────────────────────────
    /// Date-format string passed to QDate::toString() (default: "yyyy-MM-dd")
    void    setDisplayFormat(const QString &fmt);
    QString displayFormat() const;

    // ── Label pieces ───────────────────────────────────────────────────────
    /// Text prepended/appended to the formatted start date (default: empty)
    void    setStartPrefix(const QString &text);
    QString startPrefix() const;
    void    setStartSuffix(const QString &text);
    QString startSuffix() const;

    /// Text prepended/appended to the formatted end date (default: empty)
    void    setEndPrefix(const QString &text);
    QString endPrefix() const;
    void    setEndSuffix(const QString &text);
    QString endSuffix() const;

    /// Text shown between the start and end fields (default: "  →  ")
    void    setSeparator(const QString &text);
    QString separator() const;

    /// Text shown inside the start/end field when that date is not set
    /// (default: "–")
    void    setStartPlaceholder(const QString &text);
    QString startPlaceholder() const;
    void    setEndPlaceholder(const QString &text);
    QString endPlaceholder() const;

    // ── Animation property ─────────────────────────────────────────────────
    qreal hoverLevel() const;
    void  setHoverLevel(qreal v);

    QSize sizeHint()        const override;
    QSize minimumSizeHint() const override;

signals:
    void dateRangeChanged(const QDate &start, const QDate &end);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(FluentEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void applyTheme();
    void showPopup();
    void hidePopup();
    bool isPopupVisible() const;
    void startHoverAnimation(qreal endValue);

    // Build the display string for one side
    QString buildSideText(const QDate &date,
                          const QString &prefix,
                          const QString &suffix,
                          const QString &placeholder) const;

    QDate   m_startDate;
    QDate   m_endDate;
    QString m_format           = QStringLiteral("yyyy-MM-dd");
    QString m_startPrefix;
    QString m_startSuffix;
    QString m_endPrefix;
    QString m_endSuffix;
    QString m_separator        = QStringLiteral("  →  ");
    QString m_startPlaceholder = QStringLiteral("–");
    QString m_endPlaceholder   = QStringLiteral("–");

    qreal              m_hoverLevel = 0.0;
    QVariantAnimation *m_hoverAnim  = nullptr;

    FluentCalendarPopup *m_popup = nullptr;
};

} // namespace Fluent

