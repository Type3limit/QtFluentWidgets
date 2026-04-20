#pragma once

#include "Fluent/FluentBorderEffect.h"
#include "Fluent/FluentPopupSurface.h"

#include <QListWidget>
#include <QPointer>
#include <QVariant>
#include <QVector>

#include <functional>

class QTimer;
class QVariantAnimation;

namespace Fluent::Detail {

struct PickerOption {
    QString text;
    QVariant value;
};

struct PickerColumnConfig {
    QVector<PickerOption> options;
    QVariant currentValue;
    int width = 96;
    Qt::Alignment alignment = Qt::AlignCenter;
};

class FluentWheelPickerColumn final : public QListWidget
{
public:
    explicit FluentWheelPickerColumn(QWidget *parent = nullptr);

    void setOptions(const QVector<PickerOption> &options, const QVariant &selectedValue = QVariant());

    QVariant currentValue() const;
    QString currentText() const;

    void setCurrentValue(const QVariant &value);

    int itemHeight() const;

    void setTextAlignment(Qt::Alignment alignment);
    Qt::Alignment textAlignment() const;

    void snapToCurrent(bool animated);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    int spacerRowCount() const;
    int firstDataRow() const;
    int lastDataRow() const;
    int actualIndexForRow(int row) const;
    int rowForActualIndex(int index) const;
    int rowForValue(const QVariant &value) const;
    int nearestDataRow() const;
    QRectF selectionRect() const;

    void scheduleSnap();
    void syncCurrentFromScroll();
    int stepsFromWheelEvent(QWheelEvent *event);
    void moveCurrentBySteps(int steps, bool animated);
    void scrollToRowCentered(int row, bool animated);

    QVector<PickerOption> m_options;
    QVariantAnimation *m_scrollAnim = nullptr;
    QTimer *m_snapTimer = nullptr;
    Qt::Alignment m_alignment = Qt::AlignCenter;
    int m_itemHeight = 54;
    bool m_syncingFromScroll = false;
    bool m_settingOptions = false;
    int m_wheelAngleRemainder = 0;
};

class FluentWheelPickerPopup final : public QWidget
{
public:
    explicit FluentWheelPickerPopup(QWidget *anchor = nullptr);

    void setAnchor(QWidget *anchor);
    QWidget *anchor() const;

    void setColumnConfigs(const QVector<PickerColumnConfig> &configs);

    int columnCount() const;
    FluentWheelPickerColumn *columnAt(int index) const;

    QVariant columnValue(int index) const;
    void setColumnValue(int index, const QVariant &value);

    void popup();
    void dismiss(bool accepted = false, bool returnFocus = true);

    std::function<void()> onAccepted;
    std::function<void(bool accepted)> onDismissed;

protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    enum class ActionButton {
        None,
        Accept,
        Cancel
    };

    QSize popupSizeHint() const;
    QRect columnAreaRect() const;
    QRect actionBarRect() const;
    QRect acceptRect() const;
    QRect cancelRect() const;
    ActionButton hitTestAction(const QPoint &pos) const;

    void ensureColumns(int count);
    void relayoutColumns();
    void positionPopupBelowOrAbove();
    void focusNextColumn(int delta);
    void triggerAccept();

    QPointer<QWidget> m_anchor;
    QVector<FluentWheelPickerColumn *> m_columns;
    QVector<int> m_columnWidths;
    FluentBorderEffect m_border;
    QVariantAnimation *m_openAnim = nullptr;
    QRect m_targetGeom;
    int m_openSlideOffsetY = -PopupSurface::kOpenSlideOffsetPx;
    bool m_appFilterInstalled = false;
    bool m_dismissing = false;
    ActionButton m_hoverAction = ActionButton::None;
    ActionButton m_pressAction = ActionButton::None;
};

} // namespace Fluent::Detail