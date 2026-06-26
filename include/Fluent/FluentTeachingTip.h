#pragma once

#include "Fluent/FluentExport.h"

#include "Fluent/FluentFlyout.h"

#include <QList>
#include <QPointer>
#include <QString>

class QEvent;
class QHideEvent;
class QShowEvent;
class QWidget;

namespace Fluent {

class FluentButton;
class FluentLabel;
class FluentToolButton;

class FLUENT_EXPORT FluentTeachingTip final : public FluentFlyout
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(QString subtitle READ subtitle WRITE setSubtitle)
    Q_PROPERTY(QString actionText READ actionText WRITE setActionText)
    Q_PROPERTY(bool closeButtonVisible READ isCloseButtonVisible WRITE setCloseButtonVisible)
    Q_PROPERTY(bool maskEnabled READ maskEnabled WRITE setMaskEnabled)
    Q_PROPERTY(qreal maskOpacity READ maskOpacity WRITE setMaskOpacity)

public:
    struct GuideStyle {
        GuideStyle() = default;
        GuideStyle(const QString &title,
                   const QString &subtitle,
                   const QString &actionText = QString(),
                   FluentFlyout::Placement placement = FluentFlyout::Placement::Bottom,
                   bool closeButtonVisible = true)
            : title(title)
            , subtitle(subtitle)
            , actionText(actionText)
            , placement(placement)
            , closeButtonVisible(closeButtonVisible)
        {
        }

        GuideStyle(const QString &title,
                   const QString &subtitle,
                   const QString &actionText,
                   const QString &previousActionText,
                   FluentFlyout::Placement placement = FluentFlyout::Placement::Bottom,
                   bool closeButtonVisible = true,
                   QWidget *contentWidget = nullptr)
            : title(title)
            , subtitle(subtitle)
            , actionText(actionText)
            , previousActionText(previousActionText)
            , placement(placement)
            , closeButtonVisible(closeButtonVisible)
            , contentWidget(contentWidget)
        {
        }

        QString title;
        QString subtitle;
        QString actionText;
        QString previousActionText;
        FluentFlyout::Placement placement = FluentFlyout::Placement::Bottom;
        bool closeButtonVisible = true;
        QPointer<QWidget> contentWidget;
    };

    explicit FluentTeachingTip(QWidget *parent = nullptr);

    QString title() const;
    void setTitle(const QString &title);

    QString subtitle() const;
    void setSubtitle(const QString &subtitle);

    QString actionText() const;
    void setActionText(const QString &text);

    bool isCloseButtonVisible() const;
    void setCloseButtonVisible(bool visible);

    void setMaskEnabled(bool enabled);
    bool maskEnabled() const;

    void setMaskOpacity(qreal opacity);
    qreal maskOpacity() const;

    QWidget *target() const;
    void setTarget(QWidget *target);

    QWidget *contentWidget() const;
    void setContentWidget(QWidget *widget);

    void setGuideTargets(const QList<QWidget *> &targets);
    QList<QWidget *> guideTargets() const;

    void setGuideStyles(const QList<GuideStyle> &styles);
    QList<GuideStyle> guideStyles() const;

    void setGuideContentWidgets(const QList<QWidget *> &widgets);
    QList<QWidget *> guideContentWidgets() const;

    void setGuideSteps(const QList<QWidget *> &targets, const QList<GuideStyle> &styles);
    int guideCount() const;
    int currentGuideIndex() const;
    bool isGuideRunning() const;

signals:
    void actionTriggered();
    void previousActionTriggered();
    void guideStarted();
    void guideStepChanged(int index);
    void guideFinished();

public slots:
    void open();
    void startGuide(int index = 0);
    void showGuideStep(int index);
    void nextGuideStep();
    void previousGuideStep();
    void finishGuide();

protected:
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    QSize popupWindowSize() const override;
    bool popupRevealEnabled() const override;
    bool popupSlideEnabled() const override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    QSize preferredPopupSize() const;
    void updateContent();
    void ensureMaskOverlay();
    void teardownMaskOverlay();
    void updateMaskOverlay();
    QWidget *maskHostWindow() const;
    GuideStyle guideStyleAt(int index) const;
    QWidget *guideContentWidgetAt(int index) const;
    QString defaultGuideActionText(int index) const;
    QString defaultGuidePreviousActionText(int index) const;

    QPointer<QWidget> m_target;
    QList<QPointer<QWidget>> m_guideTargets;
    QList<GuideStyle> m_guideStyles;
    QList<QPointer<QWidget>> m_guideContentWidgets;
    int m_currentGuideIndex = -1;
    bool m_guideRunning = false;

    QString m_title;
    QString m_subtitle;
    QString m_actionText;
    bool m_closeButtonVisible = true;
    bool m_maskEnabled = false;
    qreal m_maskOpacity = 0.42;
    QPointer<QWidget> m_maskOverlay;
    QPointer<QWidget> m_maskHost;
    QPointer<QWidget> m_contentWidget;

    QWidget *m_panel = nullptr;
    QVBoxLayout *m_rootLayout = nullptr;
    QWidget *m_actionsHost = nullptr;
    FluentLabel *m_titleLabel = nullptr;
    FluentLabel *m_subtitleLabel = nullptr;
    FluentButton *m_previousButton = nullptr;
    FluentButton *m_actionButton = nullptr;
    FluentToolButton *m_closeButton = nullptr;
};

} // namespace Fluent
