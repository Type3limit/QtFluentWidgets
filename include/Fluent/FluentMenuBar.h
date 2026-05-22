#pragma once

#include <QMenuBar>

#include <QHash>
#include <QPointer>
#include <QRect>

namespace Fluent { class FluentMenu; }
class QVariantAnimation;
class QMouseEvent;
class QPaintEvent;
class QActionEvent;
class QFontMetrics;
class QMenu;
class QPoint;
class QWidget;

namespace Fluent {

class FluentMenuBar final : public QMenuBar
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverLevel READ hoverLevel WRITE setHoverLevel)
public:
    explicit FluentMenuBar(QWidget *parent = nullptr);

    FluentMenu *addFluentMenu(const QString &title);

    qreal hoverLevel() const;
    void setHoverLevel(qreal value);
    QSize minimumSizeHint() const override;
protected:
    void changeEvent(QEvent *event) override;
    void actionEvent(QActionEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;

private:
    void applyTheme();
    void startHoverAnimation(qreal endValue);

    void openMenuForAction(QAction *action);
    void onOpenMenuAboutToHide();

    void ensureMenusFluent();
    QMenu *menuForAction(QAction *action) const;
    void updateHighlightForAction(QAction *action, bool animate);
    QRect highlightTargetRect(QAction *action) const;
    void invalidateActionLayout();
    void rebuildActionLayout() const;
    QRect actionRect(QAction *action) const;
    QAction *actionAtPosition(const QPoint &pos) const;
    int actionItemWidth(QAction *action, const QFontMetrics &fm) const;

    QAction *m_hoverAction = nullptr;
    QAction *m_pressedAction = nullptr;
    qreal m_hoverLevel = 0.0;
    QVariantAnimation *m_hoverAnim = nullptr;

    QAction *m_highlightAction = nullptr;
    QRect m_highlightRect;
    QVariantAnimation *m_highlightAnim = nullptr;

    QHash<QAction *, QPointer<QMenu>> m_actionMenus;
    mutable QHash<QAction *, QRect> m_actionRects;
    mutable QSize m_cachedSizeHint;
    mutable bool m_layoutDirty = true;

    QPointer<QMenu> m_openMenu;
    QPointer<QWidget> m_openPopup;
    QAction *m_openAction = nullptr;
};

} // namespace Fluent
