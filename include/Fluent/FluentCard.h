#pragma once

#include "Fluent/FluentQtCompat.h"

#include <QByteArray>
#include <QString>
#include <QWidget>

class QVBoxLayout;
class QVariantAnimation;
class QPaintEvent;

namespace Fluent {

class FluentCardContentClip;

class FluentCard final : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool collapsible READ isCollapsible WRITE setCollapsible)
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(bool collapsed READ isCollapsed WRITE setCollapsed NOTIFY collapsedChanged)
public:
    explicit FluentCard(QWidget *parent = nullptr);

    void setCollapsible(bool on);
    bool isCollapsible() const;

    void setTitle(const QString &title);
    QString title() const;

    void setCollapsed(bool collapsed);
    bool isCollapsed() const;

    void setCollapseAnimationEnabled(bool enabled);
    bool isCollapseAnimationEnabled() const;

    // Opt-in animated "flow" background: a subtle, readable linear gradient
    // (from ThemeManager's shared flow palette) that slowly rotates. Off by
    // default — cards keep their solid theme surface. The animation pauses when
    // the card is hidden (e.g. another page is shown) or animations are off.
    void setFlowBackgroundEnabled(bool enabled);
    bool isFlowBackgroundEnabled() const;

    bool loadCollapseIndicatorAnimation(const QString &path);
    bool loadCollapseIndicatorAnimationData(const QByteArray &json,
                                            const QString &cacheKey = QString(),
                                            const QString &resourcePath = QString());
    void clearCollapseIndicatorAnimation();

    QWidget *contentWidget() const;
    QVBoxLayout *contentLayout() const;

signals:
    void collapsedChanged(bool collapsed);

protected:
    bool event(QEvent *event) override;
    void changeEvent(QEvent *event) override;
    void enterEvent(FluentEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void applyTheme();
    void ensureCollapsibleUi();
    void updateHeaderUi();
    void updateCollapseIndicatorState(bool animated);
    void updateCollapseIndicatorGeometry();
    void syncCollapseIndicatorSpeed(int startFrame, int endFrame);
    void applyCollapsedState(bool animated);
    void finishCollapseAnimationImmediately();
    void startHoverAnimation(qreal endValue);
    void updateFlowAnimationState();
    void lockContentHeightForAnimation(int height);
    void releaseContentHeightLock();
    void scheduleContentClipGeometryRefresh();
    void refreshContentClipGeometry();

    bool m_collapsible = false;
    bool m_collapsed = false;
    bool m_animateCollapse = true;
    QString m_title;

    QWidget *m_header = nullptr;
    class FluentLabel *m_titleLabel = nullptr;
    class FluentToolButton *m_chevronButton = nullptr;
    class FluentAnimatedIcon *m_chevronAnimation = nullptr;
    bool m_hasChevronAnimation = false;
    FluentCardContentClip *m_contentClip = nullptr;
    QWidget *m_content = nullptr;
    QVBoxLayout *m_contentLayout = nullptr;
    QVariantAnimation *m_collapseAnim = nullptr;
    QVariantAnimation *m_hoverAnim = nullptr;
    bool m_hovered = false;
    bool m_contentRefreshPending = false;
    qreal m_hoverLevel = 0.0;
    bool m_flowBackgroundEnabled = false;
    QVariantAnimation *m_flowAnim = nullptr;
    qreal m_flowPhase = 0.0;
};

} // namespace Fluent
