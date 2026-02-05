#pragma once

#include <QString>
#include <QWidget>

class QVBoxLayout;
class QPropertyAnimation;

namespace Fluent {

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

    QWidget *contentWidget() const;
    QVBoxLayout *contentLayout() const;

signals:
    void collapsedChanged(bool collapsed);

protected:
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void applyTheme();
    void ensureCollapsibleUi();
    void updateHeaderUi();
    void applyCollapsedState(bool animated);

    bool m_collapsible = false;
    bool m_collapsed = false;
    bool m_animateCollapse = true;
    QString m_title;

    QWidget *m_header = nullptr;
    class FluentLabel *m_titleLabel = nullptr;
    class FluentToolButton *m_chevronButton = nullptr;
    QWidget *m_content = nullptr;
    QVBoxLayout *m_contentLayout = nullptr;
    QPropertyAnimation *m_collapseAnim = nullptr;
};

} // namespace Fluent
