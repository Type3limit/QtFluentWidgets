#pragma once

#include "Fluent/FluentBorderEffect.h"

#include <QDate>
#include <QString>
#include <QWidget>

class QPainter;
class QColor;
class QHideEvent;
class QVariantAnimation;

namespace Fluent {

class FluentCalendarPopup final : public QWidget
{
	Q_OBJECT
public:
	enum class SelectionMode {
		Single, ///< Pick a single date (default)
		Range,  ///< Pick a start and end date
	};
	Q_ENUM(SelectionMode)

	explicit FluentCalendarPopup(QWidget *anchor = nullptr);

	void setAnchor(QWidget *anchor);
	QWidget *anchor() const;

	void setTodayText(const QString &text);
	QString todayText() const;

	// Single mode
	void setDate(const QDate &date);
	QDate date() const;

	// Range mode
	void setSelectionMode(SelectionMode mode);
	SelectionMode selectionMode() const;
	void setDateRange(const QDate &start, const QDate &end);
	QDate rangeStart() const;
	QDate rangeEnd() const;

	void popup();
	void dismiss();

signals:
	void datePicked(const QDate &date);
	void rangePicked(const QDate &start, const QDate &end);
	void dismissed();

protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;

private:
	enum class ViewMode { Days, Months, Years };

	enum class HitPart {
		None,
		// Left / single panel
		HeaderMonth, HeaderYear, HeaderToday, NavPrev, NavNext, Cell,
		// Right panel (range mode only)
		RHeaderMonth, RHeaderYear, RHeaderToday, RNavPrev, RNavNext, RCell,
	};

	// Panel geometry
	int  panelWidth() const;
	int  popupHeight() const;

	void positionPopupBelowOrAbove(int gap);

	// Single-mode rect helpers (left panel, panelX=0)
	QRect contentRect() const;
	QRect headerRect() const;
	QRect gridRect() const;
	QRect monthPillRect() const;
	QRect yearPillRect() const;
	QRect todayButtonRect() const;
	QRect navPrevRect() const;
	QRect navNextRect() const;

	void ensurePageFromSelected();
	void ensureRightPage();

	void setMode(ViewMode mode);
	void startModeTransition(ViewMode from, ViewMode to);

	void stepMonth(int delta);
	void stepYear(int delta);
	void stepYearPage(int deltaPages);
	void stepRightMonth(int delta);

	void setSelectedDate(const QDate &date, bool emitPicked);
	void handleRangeClick(const QDate &date);

	QDate dateFromCellIndex(int idx, int pageYear, int pageMonth) const;
	QDate effectiveRangeStart() const;
	QDate effectiveRangeEnd() const;

	// Hit testing
	HitPart hitTest(const QPoint &pos, int *outIndex = nullptr) const;
	int cellIndexAt(const QPoint &pos) const;
	int cellIndexInGrid(const QPoint &pos, const QRect &cells, int cw, int ch) const;

	// Single-mode painting
	void paintHeader(QPainter &p);
	void paintDays(QPainter &p);
	void paintMonths(QPainter &p);
	void paintYears(QPainter &p);

	// Range-mode painting
	void paintRangePanelHeader(QPainter &p, int panelX, int pageYear, int pageMonth, bool isRight);
	void paintRangePanelDays(QPainter &p, int panelX, int pageYear, int pageMonth, bool isRight);

	void drawChevronLeft(QPainter &p, const QPointF &center, const QColor &color) const;
	void drawChevronRight(QPainter &p, const QPointF &center, const QColor &color) const;

	QWidget *m_anchor = nullptr;

	FluentBorderEffect m_border{this};

	SelectionMode m_selectionMode = SelectionMode::Single;

	// Open/mode animations
	QVariantAnimation *m_openAnim = nullptr;
	QVariantAnimation *m_modeAnim = nullptr;
	QRect  m_targetGeom;
	qreal  m_openProgress = 1.0;

	ViewMode m_prevMode     = ViewMode::Days;
	qreal    m_modeProgress = 1.0;

	bool m_appFilterInstalled = false;

	// Left / single panel state
	ViewMode m_mode      = ViewMode::Days;
	QDate    m_selected;
	int      m_pageYear  = 0;
	int      m_pageMonth = 0;
	int      m_yearBase  = 0;

	// Right panel state (range mode)
	int m_rightPageYear  = 0;
	int m_rightPageMonth = 0;

	// Range selection state
	QDate m_rangeStart;
	QDate m_rangeEnd;
	QDate m_hoverDate;     ///< Live hover preview while selecting the end date
	bool  m_selectingEnd = false;

	HitPart m_hoverPart  = HitPart::None;
	HitPart m_pressPart  = HitPart::None;
	int     m_hoverIndex = -1;
	int     m_pressIndex = -1;
	QString m_todayText = QStringLiteral("今天");
};

} // namespace Fluent

