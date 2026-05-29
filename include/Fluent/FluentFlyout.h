#pragma once

#include "Fluent/FluentBorderEffect.h"

#include <QMargins>
#include <QPointer>
#include <QWidget>

class QVariantAnimation;
class QVBoxLayout;

namespace Fluent {

class FluentFlyout : public QWidget
{
    Q_OBJECT
public:
    enum class Placement {
        Bottom,
        Top,
        Right,
        Left,
    };
    Q_ENUM(Placement)

    explicit FluentFlyout(QWidget *parent = nullptr);
    ~FluentFlyout() override;

    QWidget *contentWidget() const;
    void setContentWidget(QWidget *widget);

    QMargins contentMargins() const;
    void setContentMargins(const QMargins &margins);

public slots:
    void showAt(const QPoint &globalPos);
    void showFor(QWidget *target, Placement placement = Placement::Bottom);

protected:
    virtual QSize popupWindowSize() const;
    virtual bool popupRevealEnabled() const;
    virtual bool popupSlideEnabled() const;
    void hideEvent(QHideEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    QPoint placedPosition(const QRect &anchor, const QSize &popupSize, Placement placement) const;
    void startOpenAnimation(const QPoint &finalPos, int slideOffsetY);
    void updateLayoutMargins();

    QVBoxLayout *m_layout = nullptr;
    QMargins m_contentMargins;
    QPointer<QWidget> m_content;
    QVariantAnimation *m_openAnim = nullptr;
    FluentBorderEffect m_border;
};

} // namespace Fluent
