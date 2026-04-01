#pragma once

#include <QComboBox>
#include "Fluent/FluentQtCompat.h"

class QPropertyAnimation;

namespace Fluent {

class FluentComboPopup;

class FluentComboBox final : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverLevel READ hoverLevel WRITE setHoverLevel)
    Q_PROPERTY(int popupScrollThreshold READ popupScrollThreshold WRITE setPopupScrollThreshold)
public:
    explicit FluentComboBox(QWidget *parent = nullptr);

    QSize sizeHint() const override;

    qreal hoverLevel() const;
    void setHoverLevel(qreal value);

    int popupScrollThreshold() const;
    void setPopupScrollThreshold(int threshold);

    void showPopup() override;
    void hidePopup() override;

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(FluentEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    friend class FluentComboPopup;

    void applyTheme();
    void commitPopupIndex(int row);
    bool isPopupVisible() const;
    int effectivePopupScrollThreshold() const;

    qreal m_hoverLevel = 0.0;
    QPropertyAnimation *m_hoverAnim = nullptr;
    FluentComboPopup *m_popup = nullptr;
    int m_popupScrollThreshold = 0;
};

} // namespace Fluent
