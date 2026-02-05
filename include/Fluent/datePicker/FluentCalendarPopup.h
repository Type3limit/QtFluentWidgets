#pragma once

#include <QDate>
#include <QWidget>

class QPainter;
class QColor;
class QVariantAnimation;

namespace Fluent {

class FluentCalendarPopup final : public QWidget
{
	Q_OBJECT
public:
	explicit FluentCalendarPopup(QWidget *anchor = nullptr);

	void setAnchor(QWidget *anchor);
	QWidget *anchor() const;

	void setDate(const QDate &date);
	QDate date() const;

	void popup();
	void dismiss();

signals:
	void datePicked(const QDate &date);
	void dismissed();

protected:
	bool event(QEvent *event) override;
	bool eventFilter(QObject *watched, QEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;

private:
	enum class ViewMode {
		Days,
		Months,
		Years,
	};

	enum class HitPart {
		None,
		HeaderMonth,
		HeaderYear,
			HeaderToday,
		NavPrev,
		NavNext,
		Cell,
	};

	void applyRoundedMask();
	void positionPopupBelowOrAbove(int gap);

	QRect contentRect() const;
	QRect headerRect() const;
	QRect gridRect() const;

	QRect monthPillRect() const;
	QRect yearPillRect() const;
		QRect todayButtonRect() const;
	QRect navPrevRect() const;
	QRect navNextRect() const;

	void ensurePageFromSelected();
	void setMode(ViewMode mode);

	void startModeTransition(ViewMode from, ViewMode to);

	void stepMonth(int delta);
	void stepYear(int delta);
	void stepYearPage(int deltaPages);

	void setSelectedDate(const QDate &date, bool emitPicked);

	HitPart hitTest(const QPoint &pos, int *outIndex = nullptr) const;
	int cellIndexAt(const QPoint &pos) const;

	void paintHeader(QPainter &p);
	void paintDays(QPainter &p);
	void paintMonths(QPainter &p);
	void paintYears(QPainter &p);

	void drawChevronLeft(QPainter &p, const QPointF &center, const QColor &color) const;
	void drawChevronRight(QPainter &p, const QPointF &center, const QColor &color) const;

	QWidget *m_anchor = nullptr;

	// popup animation
	QVariantAnimation *m_openAnim = nullptr;
	QVariantAnimation *m_modeAnim = nullptr;
	QRect m_targetGeom;
	qreal m_openProgress = 1.0;

	// mode transition animation
	ViewMode m_prevMode = ViewMode::Days;
	qreal m_modeProgress = 1.0;

	bool m_appFilterInstalled = false;

	ViewMode m_mode = ViewMode::Days;
	QDate m_selected;
	int m_pageYear = 0;
	int m_pageMonth = 0;

	int m_yearBase = 0;

	HitPart m_hoverPart = HitPart::None;
	HitPart m_pressPart = HitPart::None;
	int m_hoverIndex = -1;
	int m_pressIndex = -1;
};

} // namespace Fluent