#pragma once

#include "Fluent/FluentExport.h"

#include <QComboBox>
#include "Fluent/FluentQtCompat.h"

class QPropertyAnimation;

namespace Fluent {

class FluentComboPopup;

class FLUENT_EXPORT FluentComboBox final : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverLevel READ hoverLevel WRITE setHoverLevel)
    Q_PROPERTY(int popupScrollThreshold READ popupScrollThreshold WRITE setPopupScrollThreshold)
public:
    enum SelectionMode {
        SingleSelection,
        MultiSelection,
    };
    Q_ENUM(SelectionMode)

    explicit FluentComboBox(QWidget *parent = nullptr);

    QSize sizeHint() const override;

    qreal hoverLevel() const;
    void setHoverLevel(qreal value);

    int popupScrollThreshold() const;
    void setPopupScrollThreshold(int threshold);

    SelectionMode selectionMode() const;
    void setSelectionMode(SelectionMode mode);

    // Multi-selection helpers (only meaningful when selectionMode() == MultiSelection)
    bool isItemChecked(int index) const;
    void setItemChecked(int index, bool checked);
    QList<int> checkedIndexes() const;
    void setCheckedIndexes(const QList<int> &indexes);
    QStringList checkedTexts() const;
    void clearChecked();

    void setMultiSelectionPlaceholder(const QString &text);
    QString multiSelectionPlaceholder() const;

    void showPopup() override;
    void hidePopup() override;

signals:
    void checkedIndexesChanged(const QList<int> &indexes);

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(FluentEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    friend class FluentComboPopup;

    void applyTheme();
    void commitPopupIndex(int row);
    void toggleCheckedRow(int row);
    bool isPopupVisible() const;
    int effectivePopupScrollThreshold() const;
    void applyMultiCheckableFlags();

    qreal m_hoverLevel = 0.0;
    QPropertyAnimation *m_hoverAnim = nullptr;
    FluentComboPopup *m_popup = nullptr;
    int m_popupScrollThreshold = 0;
    SelectionMode m_selectionMode = SingleSelection;
    QString m_multiPlaceholder;
};

} // namespace Fluent
