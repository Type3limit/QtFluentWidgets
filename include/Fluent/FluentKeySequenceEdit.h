#pragma once

#include <QKeySequenceEdit>
#include <QPointer>

#include "Fluent/FluentQtCompat.h"

class QFocusEvent;
class QLineEdit;
class QVariantAnimation;

namespace Fluent {

class FluentKeySequenceEdit final : public QKeySequenceEdit
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverLevel READ hoverLevel WRITE setHoverLevel)
    Q_PROPERTY(qreal focusLevel READ focusLevel WRITE setFocusLevel)
public:
    explicit FluentKeySequenceEdit(QWidget *parent = nullptr);
    explicit FluentKeySequenceEdit(const QKeySequence &keySequence, QWidget *parent = nullptr);

    qreal hoverLevel() const;
    void setHoverLevel(qreal value);

    qreal focusLevel() const;
    void setFocusLevel(qreal value);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    void initializeControl();
    void ensureEditor();
    void applyTheme();
    void startHoverAnimation(qreal endValue);
    void startFocusAnimation(qreal endValue);
    void updateFocusState();

    qreal m_hoverLevel = 0.0;
    qreal m_focusLevel = 0.0;
    QPointer<QLineEdit> m_editor;
    QVariantAnimation *m_hoverAnim = nullptr;
    QVariantAnimation *m_focusAnim = nullptr;
};

} // namespace Fluent