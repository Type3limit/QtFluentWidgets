#pragma once

#include <QTextEdit>

class QFocusEvent;
class QPaintEvent;
class QResizeEvent;
class QShowEvent;
class QVariantAnimation;
class QWidget;

namespace Fluent {

class FluentTextEdit final : public QTextEdit
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverLevel READ hoverLevel WRITE setHoverLevel)
    Q_PROPERTY(qreal focusLevel READ focusLevel WRITE setFocusLevel)
public:
    explicit FluentTextEdit(QWidget *parent = nullptr);

    QSize sizeHint() const override;

    qreal hoverLevel() const;
    void setHoverLevel(qreal value);

    qreal focusLevel() const;
    void setFocusLevel(qreal value);

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool viewportEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void applyTheme();
    void updateBorderOverlayGeometry();
    void startHoverAnimation(qreal endValue);
    void startFocusAnimation(qreal endValue);

    qreal m_hoverLevel = 0.0;
    qreal m_focusLevel = 0.0;
    QVariantAnimation *m_hoverAnim = nullptr;
    QVariantAnimation *m_focusAnim = nullptr;
    QWidget *m_borderOverlay = nullptr;

    bool m_themeAppliedOnce = false;
};

} // namespace Fluent
