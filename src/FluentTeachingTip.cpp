#include "Fluent/FluentTeachingTip.h"

#include "Fluent/FluentButton.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentMotion.h"
#include "Fluent/FluentPopupSurface.h"
#include "Fluent/FluentToolButton.h"

#include <QCursor>
#include <QEvent>
#include <QHideEvent>
#include <QHBoxLayout>
#include <QLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPropertyAnimation>
#include <QPointer>
#include <QRegion>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>

namespace Fluent {

namespace {

constexpr int kTeachingTipPanelWidth = 320;
constexpr int kTeachingTipMinPanelWidth = 260;
constexpr int kTeachingTipPanelHorizontalPadding = 8;
constexpr int kTeachingTipBodyWidth = kTeachingTipPanelWidth - kTeachingTipPanelHorizontalPadding;
constexpr int kTeachingTipTitleWidth = kTeachingTipBodyWidth - 8 - 34;
constexpr int kTeachingTipGeometrySafetyHeight = 40;

class TeachingTipMaskOverlay final : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal overlayOpacity READ overlayOpacity WRITE setOverlayOpacity)

public:
    explicit TeachingTipMaskOverlay(QWidget *window, qreal opacity)
        : QWidget(window)
        , m_window(window)
        , m_opacity(qBound<qreal>(0.0, opacity, 1.0))
    {
        setObjectName(QStringLiteral("FluentTeachingTipMaskOverlay"));
        setAttribute(Qt::WA_NoSystemBackground, false);
        setAttribute(Qt::WA_StyledBackground, false);
        setAttribute(Qt::WA_TransparentForMouseEvents, false);

        if (m_window) {
            m_window->installEventFilter(this);
            setGeometry(m_window->rect());
            updateOverlayMask();
        }
    }

    qreal overlayOpacity() const
    {
        return m_opacity;
    }

    void setOverlayOpacity(qreal opacity)
    {
        m_opacity = qBound<qreal>(0.0, opacity, 1.0);
        update();
    }

    void setOpacity(qreal opacity)
    {
        setOverlayOpacity(opacity);
    }

    void animateOpacityTo(qreal opacity, FluentMotionRole role)
    {
        const qreal target = qBound<qreal>(0.0, opacity, 1.0);
        if (m_opacityAnim) {
            m_opacityAnim->stop();
            m_opacityAnim->deleteLater();
            m_opacityAnim = nullptr;
        }
        const int duration = FluentMotion::duration(role);
        if (duration <= 0 || qFuzzyCompare(m_opacity, target)) {
            setOverlayOpacity(target);
            return;
        }

        auto *anim = new QPropertyAnimation(this, "overlayOpacity", this);
        m_opacityAnim = anim;
        FluentMotion::configure(anim, role);
        anim->setStartValue(m_opacity);
        anim->setEndValue(target);
        connect(anim, &QPropertyAnimation::finished, this, [this, anim]() {
            if (m_opacityAnim == anim) {
                m_opacityAnim = nullptr;
            }
            anim->deleteLater();
        });
        anim->start();
    }

    void setTarget(QWidget *target)
    {
        if (m_target == target) {
            updateOverlayMask();
            update();
            return;
        }
        if (m_target) {
            m_target->removeEventFilter(this);
        }
        m_target = target;
        if (m_target) {
            m_target->installEventFilter(this);
        }
        updateOverlayMask();
        update();
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == m_window) {
            switch (event->type()) {
            case QEvent::Resize:
            case QEvent::Show:
            case QEvent::WindowStateChange:
                setGeometry(m_window->rect());
                updateOverlayMask();
                raise();
                update();
                break;
            default:
                break;
            }
        } else if (watched == m_target) {
            switch (event->type()) {
            case QEvent::Move:
            case QEvent::Resize:
            case QEvent::Show:
            case QEvent::Hide:
                updateOverlayMask();
                update();
                break;
            default:
                break;
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)

        QPainter p(this);
        if (!p.isActive()) {
            return;
        }
        p.setRenderHint(QPainter::Antialiasing, true);

        const QRectF cutout = targetCutoutRect();
        QColor dim(0, 0, 0);
        dim.setAlphaF(m_opacity);
        p.fillRect(rect(), dim);

        if (!cutout.isValid()) {
            return;
        }
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event) {
            event->accept();
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event) {
            event->accept();
        }
    }

private:
    QRectF targetHighlightRect() const
    {
        if (!m_window || !m_target || !m_target->isVisible()) {
            return QRectF();
        }

        const QPoint topLeft = m_window->mapFromGlobal(m_target->mapToGlobal(QPoint(0, 0)));
        QRect rect(topLeft, m_target->size());
        rect = rect.adjusted(-7, -7, 7, 7);
        return QRectF(rect).intersected(QRectF(this->rect()));
    }

    QRectF targetCutoutRect() const
    {
        const QRectF highlight = targetHighlightRect();
        if (!highlight.isValid()) {
            return QRectF();
        }

        return deviceAlignedRect(highlight.adjusted(3.0, 3.0, -3.0, -3.0)).intersected(QRectF(rect()));
    }

    QRectF deviceAlignedRect(const QRectF &value) const
    {
        qreal dpr = devicePixelRatioF();
        if (dpr <= 0.0) {
            dpr = 1.0;
        }

        return QRectF(
            qRound(value.x() * dpr) / dpr,
            qRound(value.y() * dpr) / dpr,
            qRound(value.width() * dpr) / dpr,
            qRound(value.height() * dpr) / dpr);
    }

    void updateOverlayMask()
    {
        if (rect().isEmpty()) {
            clearMask();
            return;
        }

        const QRectF targetHole = targetCutoutRect();
        if (!targetHole.isValid()) {
            setMask(QRegion(rect()));
            return;
        }

        QRegion region(rect());
        if (targetHole.isValid()) {
            region = region.subtracted(QRegion(targetHole.toAlignedRect()));
        }
        setMask(region);
    }

    QPointer<QWidget> m_window;
    QPointer<QWidget> m_target;
    QPointer<QPropertyAnimation> m_opacityAnim;
    qreal m_opacity = 0.42;
};

} // namespace

FluentTeachingTip::FluentTeachingTip(QWidget *parent)
    : FluentFlyout(parent)
{
    m_panel = new QWidget(this);
    m_panel->setAttribute(Qt::WA_StyledBackground, false);
    m_panel->setFixedWidth(kTeachingTipPanelWidth);

    m_rootLayout = new QVBoxLayout(m_panel);
    m_rootLayout->setContentsMargins(4, 2, 4, 4);
    m_rootLayout->setSpacing(8);

    auto *header = new QHBoxLayout();
    header->setContentsMargins(0, 0, 0, 0);
    header->setSpacing(8);

    m_titleLabel = new FluentLabel(m_panel);
    m_titleLabel->setStyleSheet(QStringLiteral("font-size: 14px; font-weight: 650;"));
    m_titleLabel->setWordWrap(true);
    m_titleLabel->setFixedWidth(kTeachingTipTitleWidth);

    m_closeButton = new FluentToolButton(m_panel);
    m_closeButton->setProperty("fluentWindowGlyph", 3);
    m_closeButton->setProperty("fluentNeutralCloseGlyph", true);
    m_closeButton->setFixedSize(34, 28);
    connect(m_closeButton, &QToolButton::clicked, this, [this]() {
        if (m_guideRunning) {
            finishGuide();
        } else {
            hide();
        }
    });

    header->addWidget(m_titleLabel, 1);
    header->addWidget(m_closeButton);

    m_subtitleLabel = new FluentLabel(m_panel);
    m_subtitleLabel->setWordWrap(true);
    m_subtitleLabel->setFixedWidth(kTeachingTipBodyWidth);
    m_subtitleLabel->setStyleSheet(QStringLiteral("font-size: 12px; opacity: 0.88;"));

    m_actionButton = new FluentButton(m_panel);
    m_actionButton->setVisible(false);
    m_actionButton->setPrimary(true);
    connect(m_actionButton, &QPushButton::clicked, this, [this]() {
        emit actionTriggered();
        if (m_guideRunning) {
            nextGuideStep();
        } else {
            hide();
        }
    });

    m_previousButton = new FluentButton(m_panel);
    m_previousButton->setVisible(false);
    connect(m_previousButton, &QPushButton::clicked, this, [this]() {
        emit previousActionTriggered();
        if (m_guideRunning) {
            previousGuideStep();
        }
    });

    m_actionsHost = new QWidget(m_panel);
    m_actionsHost->setFixedWidth(kTeachingTipBodyWidth);
    auto *actionsLayout = new QHBoxLayout(m_actionsHost);
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->setSpacing(8);
    actionsLayout->addWidget(m_previousButton);
    actionsLayout->addWidget(m_actionButton);
    actionsLayout->addStretch(1);

    m_rootLayout->addLayout(header);
    m_rootLayout->addWidget(m_subtitleLabel);
    m_rootLayout->addWidget(m_actionsHost);

    FluentFlyout::setContentWidget(m_panel);
    setMinimumWidth(PopupSurface::withShadowMargins(QSize(kTeachingTipMinPanelWidth, 0)).width());
    updateContent();
}

QString FluentTeachingTip::title() const
{
    return m_title;
}

void FluentTeachingTip::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    updateContent();
}

QString FluentTeachingTip::subtitle() const
{
    return m_subtitle;
}

void FluentTeachingTip::setSubtitle(const QString &subtitle)
{
    if (m_subtitle == subtitle) {
        return;
    }
    m_subtitle = subtitle;
    updateContent();
}

QString FluentTeachingTip::actionText() const
{
    return m_actionText;
}

void FluentTeachingTip::setActionText(const QString &text)
{
    if (m_actionText == text) {
        return;
    }
    m_actionText = text;
    updateContent();
}

bool FluentTeachingTip::isCloseButtonVisible() const
{
    return m_closeButtonVisible;
}

void FluentTeachingTip::setCloseButtonVisible(bool visible)
{
    if (m_closeButtonVisible == visible) {
        return;
    }
    m_closeButtonVisible = visible;
    updateContent();
}

void FluentTeachingTip::setMaskEnabled(bool enabled)
{
    if (m_maskEnabled == enabled) {
        return;
    }
    m_maskEnabled = enabled;
    if (isVisible()) {
        if (m_maskEnabled) {
            ensureMaskOverlay();
        } else {
            teardownMaskOverlay();
        }
    }
}

bool FluentTeachingTip::maskEnabled() const
{
    return m_maskEnabled;
}

void FluentTeachingTip::setMaskOpacity(qreal opacity)
{
    m_maskOpacity = qBound<qreal>(0.0, opacity, 1.0);
    updateMaskOverlay();
}

qreal FluentTeachingTip::maskOpacity() const
{
    return m_maskOpacity;
}

QWidget *FluentTeachingTip::target() const
{
    return m_target.data();
}

void FluentTeachingTip::setTarget(QWidget *target)
{
    if (m_target) {
        m_target->removeEventFilter(this);
    }
    m_target = target;
    if (m_target) {
        m_target->installEventFilter(this);
    }
    updateMaskOverlay();
}

QWidget *FluentTeachingTip::contentWidget() const
{
    return m_contentWidget.data();
}

void FluentTeachingTip::setContentWidget(QWidget *widget)
{
    if (m_contentWidget == widget) {
        return;
    }

    if (m_contentWidget) {
        m_rootLayout->removeWidget(m_contentWidget);
        m_contentWidget->setParent(nullptr);
    }

    m_contentWidget = widget;
    if (m_contentWidget) {
        m_contentWidget->setParent(m_panel);
        m_contentWidget->setFixedWidth(kTeachingTipBodyWidth);
        const int actionIndex = m_rootLayout->indexOf(m_actionsHost);
        m_rootLayout->insertWidget(actionIndex >= 0 ? actionIndex : m_rootLayout->count(), m_contentWidget);
    }
    updateGeometry();
    setFixedSize(preferredPopupSize());
}

void FluentTeachingTip::setGuideTargets(const QList<QWidget *> &targets)
{
    m_guideTargets.clear();
    m_guideTargets.reserve(targets.size());
    for (QWidget *target : targets) {
        m_guideTargets.append(target);
    }

    if (m_guideRunning && guideCount() <= 0) {
        finishGuide();
    }
}

QList<QWidget *> FluentTeachingTip::guideTargets() const
{
    QList<QWidget *> targets;
    targets.reserve(m_guideTargets.size());
    for (const auto &target : m_guideTargets) {
        targets.append(target.data());
    }
    return targets;
}

void FluentTeachingTip::setGuideStyles(const QList<GuideStyle> &styles)
{
    m_guideStyles = styles;
}

QList<FluentTeachingTip::GuideStyle> FluentTeachingTip::guideStyles() const
{
    return m_guideStyles;
}

void FluentTeachingTip::setGuideContentWidgets(const QList<QWidget *> &widgets)
{
    m_guideContentWidgets.clear();
    m_guideContentWidgets.reserve(widgets.size());
    for (QWidget *widget : widgets) {
        m_guideContentWidgets.append(widget);
    }
}

QList<QWidget *> FluentTeachingTip::guideContentWidgets() const
{
    QList<QWidget *> widgets;
    widgets.reserve(m_guideContentWidgets.size());
    for (const auto &widget : m_guideContentWidgets) {
        widgets.append(widget.data());
    }
    return widgets;
}

void FluentTeachingTip::setGuideSteps(const QList<QWidget *> &targets, const QList<GuideStyle> &styles)
{
    m_guideTargets.clear();
    m_guideTargets.reserve(targets.size());
    for (QWidget *target : targets) {
        m_guideTargets.append(target);
    }
    m_guideStyles = styles;

    if (m_guideRunning && guideCount() <= 0) {
        finishGuide();
    }
}

int FluentTeachingTip::guideCount() const
{
    return m_guideTargets.size();
}

int FluentTeachingTip::currentGuideIndex() const
{
    return m_currentGuideIndex;
}

bool FluentTeachingTip::isGuideRunning() const
{
    return m_guideRunning;
}

void FluentTeachingTip::open()
{
    if (m_target) {
        showFor(m_target, FluentFlyout::Placement::Bottom);
    } else {
        showAt(QCursor::pos());
    }
}

void FluentTeachingTip::startGuide(int index)
{
    if (guideCount() <= 0) {
        finishGuide();
        return;
    }

    const bool wasRunning = m_guideRunning;
    m_guideRunning = true;
    if (!wasRunning) {
        emit guideStarted();
    }
    showGuideStep(qBound(0, index, guideCount() - 1));
}

void FluentTeachingTip::showGuideStep(int index)
{
    const int count = guideCount();
    if (count <= 0 || index < 0 || index >= count) {
        finishGuide();
        return;
    }

    for (int stepIndex = index; stepIndex < count; ++stepIndex) {
        QWidget *stepTarget = m_guideTargets.at(stepIndex).data();
        if (!stepTarget) {
            continue;
        }

        const bool wasRunning = m_guideRunning;
        m_guideRunning = true;
        if (!wasRunning) {
            emit guideStarted();
        }

        const GuideStyle style = guideStyleAt(stepIndex);
        m_currentGuideIndex = stepIndex;
        setTarget(stepTarget);
        setTitle(style.title);
        setSubtitle(style.subtitle);
        setActionText(style.actionText.isEmpty() ? defaultGuideActionText(stepIndex) : style.actionText);
        m_previousButton->setText(style.previousActionText.isEmpty() ? defaultGuidePreviousActionText(stepIndex) : style.previousActionText);
        setCloseButtonVisible(style.closeButtonVisible);
        setContentWidget(style.contentWidget ? style.contentWidget.data() : guideContentWidgetAt(stepIndex));
        updateContent();
        showFor(stepTarget, style.placement);
        updateMaskOverlay();
        emit guideStepChanged(stepIndex);
        return;
    }

    finishGuide();
}

void FluentTeachingTip::nextGuideStep()
{
    showGuideStep(m_currentGuideIndex + 1);
}

void FluentTeachingTip::previousGuideStep()
{
    if (!m_guideRunning) {
        return;
    }
    if (m_currentGuideIndex <= 0) {
        showGuideStep(0);
        return;
    }
    showGuideStep(m_currentGuideIndex - 1);
}

void FluentTeachingTip::finishGuide()
{
    const bool wasRunning = m_guideRunning;
    m_guideRunning = false;
    m_currentGuideIndex = -1;
    hide();
    if (wasRunning) {
        emit guideFinished();
    }
}

bool FluentTeachingTip::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_target && event) {
        switch (event->type()) {
        case QEvent::Move:
        case QEvent::Resize:
        case QEvent::Show:
        case QEvent::Hide:
            updateMaskOverlay();
            break;
        default:
            break;
        }
    }
    return FluentFlyout::eventFilter(watched, event);
}

QSize FluentTeachingTip::sizeHint() const
{
    return preferredPopupSize();
}

QSize FluentTeachingTip::minimumSizeHint() const
{
    return preferredPopupSize();
}

QSize FluentTeachingTip::popupWindowSize() const
{
    return preferredPopupSize();
}

bool FluentTeachingTip::popupRevealEnabled() const
{
    return false;
}

bool FluentTeachingTip::popupSlideEnabled() const
{
    return false;
}

void FluentTeachingTip::hideEvent(QHideEvent *event)
{
    const bool wasGuideRunning = m_guideRunning;
    m_guideRunning = false;
    m_currentGuideIndex = -1;
    teardownMaskOverlay();
    FluentFlyout::hideEvent(event);
    if (wasGuideRunning) {
        emit guideFinished();
    }
}

void FluentTeachingTip::showEvent(QShowEvent *event)
{
    if (m_maskEnabled) {
        ensureMaskOverlay();
    }
    FluentFlyout::showEvent(event);
    raise();
}

QSize FluentTeachingTip::preferredPopupSize() const
{
    if (!m_panel) {
        return FluentFlyout::sizeHint();
    }

    int panelHeight = 0;
    if (QLayout *panelLayout = m_panel->layout()) {
        panelLayout->activate();
        panelHeight = panelLayout->hasHeightForWidth()
            ? panelLayout->heightForWidth(kTeachingTipPanelWidth)
            : panelLayout->sizeHint().height();
    } else {
        panelHeight = m_panel->sizeHint().height();
    }

    panelHeight = qMax(panelHeight, m_panel->minimumSizeHint().height());
    panelHeight = qMax(panelHeight,
                       QLayout::closestAcceptableSize(const_cast<QWidget *>(m_panel),
                                                      QSize(kTeachingTipPanelWidth, panelHeight)).height());
    panelHeight += kTeachingTipGeometrySafetyHeight;
    const QMargins margins = contentMargins();
    const QSize contentSize(kTeachingTipPanelWidth + margins.left() + margins.right(),
                            panelHeight + margins.top() + margins.bottom());
    return PopupSurface::withShadowMargins(contentSize);
}

void FluentTeachingTip::updateContent()
{
    m_titleLabel->setText(m_title);
    m_titleLabel->setVisible(!m_title.isEmpty());
    m_subtitleLabel->setText(m_subtitle);
    m_subtitleLabel->setVisible(!m_subtitle.isEmpty());

    const bool showAction = !m_actionText.isEmpty();
    const bool showPrevious = m_guideRunning && m_currentGuideIndex > 0;

    m_actionButton->setText(m_actionText);
    m_actionButton->setVisible(showAction);
    m_previousButton->setVisible(showPrevious);
    m_actionsHost->setVisible(showPrevious || showAction);
    m_closeButton->setVisible(m_closeButtonVisible);
    updateGeometry();
    setFixedSize(preferredPopupSize());
}

void FluentTeachingTip::ensureMaskOverlay()
{
    if (!m_maskEnabled) {
        return;
    }

    QWidget *host = maskHostWindow();
    if (!host) {
        return;
    }

    if (m_maskOverlay && m_maskOverlay->parentWidget() == host) {
        updateMaskOverlay();
        return;
    }

    teardownMaskOverlay();

    auto *overlay = new TeachingTipMaskOverlay(host, 0.0);
    overlay->setGeometry(host->rect());
    overlay->setTarget(m_target);
    overlay->show();
    overlay->raise();
    overlay->animateOpacityTo(m_maskOpacity, FluentMotionRole::PopupOpen);
    raise();
    m_maskOverlay = overlay;
    m_maskHost = host;
}

void FluentTeachingTip::teardownMaskOverlay()
{
    if (m_maskOverlay) {
        QWidget *overlay = m_maskOverlay;
        m_maskOverlay = nullptr;
        if (auto *typed = qobject_cast<TeachingTipMaskOverlay *>(overlay)) {
            typed->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            typed->animateOpacityTo(0.0, FluentMotionRole::PopupClose);
            const int duration = FluentMotion::duration(FluentMotionRole::PopupClose);
            if (duration > 0) {
                QTimer::singleShot(duration + 16, typed, [typed]() {
                    typed->hide();
                    typed->deleteLater();
                });
            } else {
                typed->hide();
                typed->deleteLater();
            }
        } else {
            overlay->hide();
            overlay->deleteLater();
        }
    }
    m_maskHost = nullptr;
}

void FluentTeachingTip::updateMaskOverlay()
{
    if (!m_maskEnabled || !isVisible()) {
        return;
    }

    QWidget *host = maskHostWindow();
    if (!host) {
        teardownMaskOverlay();
        return;
    }

    if (!m_maskOverlay || m_maskOverlay->parentWidget() != host) {
        ensureMaskOverlay();
        return;
    }

    if (auto *overlay = dynamic_cast<TeachingTipMaskOverlay *>(m_maskOverlay.data())) {
        overlay->setOpacity(m_maskOpacity);
        overlay->setTarget(m_target);
    }
    m_maskOverlay->setGeometry(host->rect());
    m_maskOverlay->show();
    m_maskOverlay->raise();
    m_maskOverlay->update();
    raise();
}

QWidget *FluentTeachingTip::maskHostWindow() const
{
    if (m_target) {
        return m_target->window();
    }
    if (parentWidget()) {
        return parentWidget()->window();
    }
    return nullptr;
}

FluentTeachingTip::GuideStyle FluentTeachingTip::guideStyleAt(int index) const
{
    if (index >= 0 && index < m_guideStyles.size()) {
        return m_guideStyles.at(index);
    }
    return GuideStyle();
}

QWidget *FluentTeachingTip::guideContentWidgetAt(int index) const
{
    if (index >= 0 && index < m_guideContentWidgets.size()) {
        return m_guideContentWidgets.at(index).data();
    }
    return nullptr;
}

QString FluentTeachingTip::defaultGuideActionText(int index) const
{
    if (index >= guideCount() - 1) {
        return tr("Done");
    }
    return tr("Next");
}

QString FluentTeachingTip::defaultGuidePreviousActionText(int index) const
{
    Q_UNUSED(index)
    return tr("Back");
}

} // namespace Fluent

#include "FluentTeachingTip.moc"
