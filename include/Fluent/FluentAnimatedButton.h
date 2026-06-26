#pragma once

#include "Fluent/FluentExport.h"

#include "Fluent/FluentAnimatedIcon.h"
#include "Fluent/FluentQtCompat.h"

#include <QByteArray>
#include <QColor>
#include <QPushButton>
#include <QSize>

class QMouseEvent;
class QVariantAnimation;

namespace Fluent {

class FLUENT_EXPORT FluentAnimatedButton final : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(bool primary READ isPrimary WRITE setPrimary)
    Q_PROPERTY(qreal hoverLevel READ hoverLevel WRITE setHoverLevel)
    Q_PROPERTY(qreal pressLevel READ pressLevel WRITE setPressLevel)
    Q_PROPERTY(QSize animatedIconSize READ animatedIconSize WRITE setAnimatedIconSize)
    Q_PROPERTY(QString animationState READ animationState WRITE setAnimationState NOTIFY animationStateChanged)
public:
    explicit FluentAnimatedButton(QWidget *parent = nullptr);
    explicit FluentAnimatedButton(const QString &text, QWidget *parent = nullptr);

    bool isPrimary() const;
    void setPrimary(bool primary);

    qreal hoverLevel() const;
    void setHoverLevel(qreal value);

    qreal pressLevel() const;
    void setPressLevel(qreal value);

    QSize animatedIconSize() const;
    void setAnimatedIconSize(const QSize &size);

    FluentAnimatedIcon *animatedIcon() const;

    bool loadAnimation(const QString &path);
    bool loadAnimationData(const QByteArray &json,
                           const QString &cacheKey = QString(),
                           const QString &resourcePath = QString());

    QString animationState() const;
    void setAnimationState(const QString &state);
    Q_INVOKABLE void setAnimationState(const QString &state, bool animated);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void animationStateChanged(const QString &state);

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void enterEvent(FluentEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void initialize();
    void applyTheme();
    void startHoverAnimation(qreal endValue);
    void startPressAnimation(qreal endValue);
    void updateAnimatedIconGeometry();
    void updateAnimatedIconTint();
    QColor currentForegroundColor() const;
    void syncRestingAnimationState(bool animated);
    bool hasAnimationState(const QString &state) const;

    bool m_primary = false;
    qreal m_hoverLevel = 0.0;
    qreal m_pressLevel = 0.0;
    QSize m_animatedIconSize = QSize(24, 24);
    QString m_animationState = QStringLiteral("Normal");
    QVariantAnimation *m_hoverAnim = nullptr;
    QVariantAnimation *m_pressAnim = nullptr;
    FluentAnimatedIcon *m_animatedIcon = nullptr;
};

} // namespace Fluent
