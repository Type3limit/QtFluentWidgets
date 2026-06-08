#include "Fluent/FluentCard.h"

#include "Fluent/FluentAnimatedIcon.h"
#include "Fluent/FluentFramePainter.h"
#include "Fluent/FluentIcon.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentMotion.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentToolButton.h"

#include <QAbstractAnimation>
#include <QAbstractButton>
#include <QEvent>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QVariantAnimation>
#include <QVBoxLayout>

namespace Fluent {

static constexpr const char *kFlowDisableAnimationProperty = "fluentFlowDisableAnimation";

class FluentCardContentClip final : public QWidget
{
public:
    explicit FluentCardContentClip(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setObjectName("FluentCardContentClip");
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_StyledBackground, false);
        setAutoFillBackground(false);
        setContentsMargins(0, 0, 0, 0);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }

    void setContent(QWidget *content)
    {
        if (m_content == content) {
            return;
        }
        m_content = content;
        if (m_content && m_content->parentWidget() != this) {
            m_content->setParent(this);
        }
        updateContentGeometry();
    }

    int contentNaturalHeight() const
    {
        if (!m_content) {
            return 0;
        }
        const int availableWidth = qMax(0, width());
        if (m_content->layout()) {
            auto *layout = m_content->layout();
            if (layout->hasHeightForWidth()) {
                return qMax(0, layout->heightForWidth(availableWidth));
            }
            return qMax(0, layout->sizeHint().height());
        }
        if (m_content->hasHeightForWidth()) {
            return qMax(0, m_content->heightForWidth(availableWidth));
        }
        return qMax(0, m_content->sizeHint().height());
    }

    int revealHeight() const
    {
        return m_revealHeight;
    }

    void setRevealHeight(int height)
    {
        const int next = qMax(0, height);
        m_revealHeight = next;
        setMinimumHeight(next);
        setMaximumHeight(next);
        if (next > 0 && !isVisible()) {
            setVisible(true);
        }
        updateContentGeometry();
        updateGeometry();
    }

    void releaseRevealHeight()
    {
        m_revealHeight = -1;
        setMinimumHeight(0);
        setMaximumHeight(QWIDGETSIZE_MAX);
        setVisible(true);
        refreshContentGeometry();
    }

    void refreshContentGeometry()
    {
        updateContentGeometry();
        if (m_content) {
            m_content->updateGeometry();
        }
        updateGeometry();
    }

    QSize sizeHint() const override
    {
        const QSize contentSize = m_content ? m_content->sizeHint() : QSize();
        const int height = (m_revealHeight >= 0) ? m_revealHeight : contentNaturalHeight();
        return QSize(contentSize.width(), height);
    }

    QSize minimumSizeHint() const override
    {
        const int height = (m_revealHeight >= 0) ? m_revealHeight : contentNaturalHeight();
        return QSize(0, qMax(0, height));
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        updateContentGeometry();
    }

private:
    void updateContentGeometry()
    {
        if (!m_content) {
            return;
        }
        if (auto *layout = m_content->layout()) {
            layout->activate();
        }
        m_content->setVisible(true);
        m_content->setGeometry(0, 0, width(), contentNaturalHeight());
    }

    QWidget *m_content = nullptr;
    int m_revealHeight = -1;
};

static QIcon makeChevronIcon(bool collapsed, const QColor &color)
{
    FluentIconOptions options;
    options.color = color;
    return FluentIcon::icon(collapsed ? FluentIconType::ChevronRight : FluentIconType::ChevronDown, options);
}

FluentCard::FluentCard(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("FluentCard");
    setAttribute(Qt::WA_StyledBackground, false);
    setAutoFillBackground(false);

    m_hoverAnim = new QVariantAnimation(this);
    FluentMotion::configure(m_hoverAnim, FluentMotionRole::Hover);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverLevel = qBound<qreal>(0.0, value.toReal(), 1.0);
        update();
    });

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentCard::applyTheme);
}

void FluentCard::setCollapsible(bool on)
{
    if (m_collapsible == on) {
        return;
    }

    m_collapsible = on;
    if (m_collapsible) {
        ensureCollapsibleUi();
        if (m_header) {
            m_header->removeEventFilter(this);
            m_header->installEventFilter(this);
            m_header->show();
        }
        applyCollapsedState(false);
        updateHeaderUi();
        updateCollapseIndicatorState(false);
    } else {
        setProperty(kFlowDisableAnimationProperty, false);
        if (m_header) {
            m_header->hide();
            m_header->removeEventFilter(this);
        }
        if (m_content) {
            releaseContentHeightLock();
            m_content->setVisible(true);
            m_content->setMaximumHeight(QWIDGETSIZE_MAX);
        }
        if (m_contentClip) {
            m_contentClip->setVisible(true);
            m_contentClip->setMaximumHeight(QWIDGETSIZE_MAX);
        }
        updateGeometry();
    }
}

bool FluentCard::isCollapsible() const
{
    return m_collapsible;
}

void FluentCard::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    updateHeaderUi();
}

QString FluentCard::title() const
{
    return m_title;
}

void FluentCard::setCollapsed(bool collapsed)
{
    if (m_collapsed == collapsed) {
        return;
    }

    m_collapsed = collapsed;
    emit collapsedChanged(m_collapsed);

    if (m_collapsible) {
        applyCollapsedState(m_animateCollapse);
        updateHeaderUi();
        updateCollapseIndicatorState(m_animateCollapse);
    }
}

bool FluentCard::isCollapsed() const
{
    return m_collapsed;
}

void FluentCard::setCollapseAnimationEnabled(bool enabled)
{
    m_animateCollapse = enabled;
}

bool FluentCard::isCollapseAnimationEnabled() const
{
    return m_animateCollapse;
}

bool FluentCard::loadCollapseIndicatorAnimation(const QString &path)
{
    if (!m_collapsible) {
        return false;
    }

    ensureCollapsibleUi();
    if (!m_chevronAnimation) {
        return false;
    }

    const bool ok = m_chevronAnimation->load(path);
    m_hasChevronAnimation = ok;
    updateHeaderUi();
    updateCollapseIndicatorState(false);
    return ok;
}

bool FluentCard::loadCollapseIndicatorAnimationData(const QByteArray &json,
                                                    const QString &cacheKey,
                                                    const QString &resourcePath)
{
    if (!m_collapsible) {
        return false;
    }

    ensureCollapsibleUi();
    if (!m_chevronAnimation) {
        return false;
    }

    const bool ok = m_chevronAnimation->loadData(json, cacheKey, resourcePath);
    m_hasChevronAnimation = ok;
    updateHeaderUi();
    updateCollapseIndicatorState(false);
    return ok;
}

void FluentCard::clearCollapseIndicatorAnimation()
{
    m_hasChevronAnimation = false;
    if (m_chevronAnimation) {
        m_chevronAnimation->hide();
    }
    updateHeaderUi();
}

QWidget *FluentCard::contentWidget() const
{
    return m_content;
}

QVBoxLayout *FluentCard::contentLayout() const
{
    auto *self = const_cast<FluentCard *>(this);
    self->scheduleContentClipGeometryRefresh();
    return m_contentLayout;
}

bool FluentCard::event(QEvent *event)
{
    const bool shouldRefresh = event->type() == QEvent::LayoutRequest
        || event->type() == QEvent::ChildAdded
        || event->type() == QEvent::ChildRemoved
        || event->type() == QEvent::Resize
        || event->type() == QEvent::Show;
    if (shouldRefresh) {
        scheduleContentClipGeometryRefresh();
    }

    return QWidget::event(event);
}

void FluentCard::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentCard::enterEvent(FluentEnterEvent *event)
{
    QWidget::enterEvent(event);
    if (!m_hovered) {
        m_hovered = true;
        startHoverAnimation(1.0);
    }
}

void FluentCard::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    if (m_hovered) {
        m_hovered = false;
        startHoverAnimation(0.0);
    }
}

bool FluentCard::eventFilter(QObject *watched, QEvent *event)
{
    if ((watched == m_content || watched == m_contentLayout)
        && (event->type() == QEvent::LayoutRequest
            || event->type() == QEvent::ChildAdded
            || event->type() == QEvent::ChildRemoved
            || event->type() == QEvent::Show
            || event->type() == QEvent::Resize)) {
        scheduleContentClipGeometryRefresh();
    }

    if (watched == m_header && m_collapsible && event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            setCollapsed(!m_collapsed);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void FluentCard::applyTheme()
{
    const bool hoverRunning = m_hoverAnim && m_hoverAnim->state() == QAbstractAnimation::Running;
    const QVariant hoverEnd = m_hoverAnim ? m_hoverAnim->endValue() : QVariant();
    const bool collapseRunning = m_collapseAnim
        && m_collapseAnim->state() == QAbstractAnimation::Running;

    FluentMotion::configure(m_hoverAnim, FluentMotionRole::Hover);
    if (m_collapseAnim) {
        FluentMotion::configure(m_collapseAnim, FluentMotionRole::Collapse);
    }
    if (hoverRunning && m_hoverAnim->duration() <= 0) {
        m_hoverAnim->stop();
        m_hoverLevel = qBound<qreal>(0.0, hoverEnd.toReal(), 1.0);
    }
    if (collapseRunning && m_collapseAnim && m_collapseAnim->duration() <= 0) {
        finishCollapseAnimationImmediately();
    }
    update();
    updateHeaderUi();
}

void FluentCard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter p(this);
    if (!p.isActive()) {
        return;
    }

    FluentSurfaceSpec surface;
    surface.level = FluentSurfaceLevel::Card;
    surface.elevation = FluentElevationLevel::None;
    surface.radius = ThemeManager::instance().tokens().radius.card;
    surface.enabled = isEnabled();
    surface.hovered = (m_hoverLevel > 0.001) && isEnabled();
    surface.hoverLevel = m_hoverLevel;
    surface.borderWidth = 1.0;
    surface.borderInset = 0.5;

    paintFluentSurface(p, QRectF(rect()), ThemeManager::instance().colors(), surface);
}

void FluentCard::ensureCollapsibleUi()
{
    if (m_header && m_content && m_contentLayout) {
        return;
    }

    auto *root = qobject_cast<QVBoxLayout *>(layout());
    if (!root) {
        root = new QVBoxLayout(this);
        root->setContentsMargins(16, 14, 16, 14);
        root->setSpacing(10);
        setLayout(root);
    }

    if (!m_contentClip) {
        m_contentClip = new FluentCardContentClip(this);
    }

    // If the user already placed widgets/layouts on this card, move them into the collapsible content container.
    if (!m_content && root->count() > 0) {
        m_content = new QWidget(m_contentClip);
        m_content->setObjectName("FluentCardContent");
        m_content->installEventFilter(this);

        m_contentLayout = new QVBoxLayout(m_content);
        m_contentLayout->setContentsMargins(0, 0, 0, 0);
        m_contentLayout->setSpacing(8);
        m_contentLayout->installEventFilter(this);

        QList<QLayoutItem *> items;
        items.reserve(root->count());
        while (root->count() > 0) {
            items.append(root->takeAt(0));
        }

        for (QLayoutItem *it : items) {
            if (!it) {
                continue;
            }
            if (auto *w = it->widget()) {
                w->setParent(m_content);
                m_contentLayout->addWidget(w);
                delete it;
                continue;
            }
            if (auto *l = it->layout()) {
                m_contentLayout->addLayout(l);
                delete it;
                continue;
            }
            m_contentLayout->addItem(it);
        }
    }

    if (!m_header) {
        m_header = new QWidget(this);
        m_header->setObjectName("FluentCardHeader");
        m_header->setCursor(Qt::PointingHandCursor);
        m_header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_header->installEventFilter(this);

        auto *hl = new QHBoxLayout(m_header);
        hl->setContentsMargins(0, 0, 0, 0);
        hl->setSpacing(8);

        m_titleLabel = new FluentLabel(m_header);
        m_titleLabel->setObjectName("FluentCardTitle");
        m_titleLabel->setStyleSheet("font-size: 13px; font-weight: 600;");

        m_chevronButton = new FluentToolButton(m_header);
        m_chevronButton->setObjectName("FluentCardChevron");
        m_chevronButton->setFixedSize(28, 28);

        m_chevronAnimation = new FluentAnimatedIcon(m_chevronButton);
        m_chevronAnimation->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        m_chevronAnimation->setInteractive(false);
        m_chevronAnimation->setFixedSize(20, 20);
        m_chevronAnimation->setFallbackIconSize(QSize(16, 16));
        m_chevronAnimation->hide();
        connect(m_chevronAnimation, &FluentAnimatedIcon::finished, this, [this] {
            updateHeaderUi();
        });

        connect(m_chevronButton, &QAbstractButton::clicked, this, [this] {
            if (m_collapsible) {
                setCollapsed(!m_collapsed);
            }
        });

        hl->addWidget(m_titleLabel, 1);
        hl->addWidget(m_chevronButton, 0, Qt::AlignRight | Qt::AlignVCenter);

        root->insertWidget(0, m_header);
    }

    if (!m_content) {
        m_content = new QWidget(m_contentClip);
        m_content->setObjectName("FluentCardContent");
        m_content->installEventFilter(this);

        m_contentLayout = new QVBoxLayout(m_content);
        m_contentLayout->setContentsMargins(0, 0, 0, 0);
        m_contentLayout->setSpacing(8);
        m_contentLayout->installEventFilter(this);
    }

    if (m_content && m_contentClip && m_content->parentWidget() != m_contentClip) {
        m_content->setParent(m_contentClip);
    }
    if (m_contentClip && m_content) {
        m_contentClip->setContent(m_content);
    }

    if (m_contentClip && root->indexOf(m_contentClip) < 0) {
        root->addWidget(m_contentClip);
    }

    refreshContentClipGeometry();

    if (!m_collapseAnim) {
        m_collapseAnim = new QVariantAnimation(this);
        FluentMotion::configure(m_collapseAnim, FluentMotionRole::Collapse);
        connect(m_collapseAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            if (m_contentClip) {
                m_contentClip->setRevealHeight(value.toInt());
            }
        });
        connect(m_collapseAnim, &QVariantAnimation::finished, this, [this] {
            if (!m_content || !m_contentClip) {
                return;
            }
            setProperty(kFlowDisableAnimationProperty, false);
            releaseContentHeightLock();
            if (m_collapsed) {
                m_contentClip->setRevealHeight(0);
                m_contentClip->setVisible(false);
            } else {
                m_contentClip->releaseRevealHeight();
                m_content->setVisible(true);
            }
            updateGeometry();
        });
    }
}

void FluentCard::updateHeaderUi()
{
    if (!m_collapsible || !m_header) {
        return;
    }

    if (m_titleLabel) {
        m_titleLabel->setText(m_title);
    }

    if (m_chevronButton) {
        const auto colors = ThemeManager::instance().colors();
        const QIcon staticIcon = makeChevronIcon(m_collapsed, colors.subText);
        if (m_hasChevronAnimation && m_chevronAnimation && m_chevronAnimation->isLoaded()) {
            m_chevronAnimation->setTintColor(colors.subText);
            m_chevronAnimation->setFallbackIcon(staticIcon);
            updateCollapseIndicatorGeometry();
            if (m_chevronAnimation->isPlaying()) {
                m_chevronButton->setIcon(QIcon());
                m_chevronAnimation->show();
            } else {
                m_chevronAnimation->hide();
                m_chevronButton->setIcon(staticIcon);
                m_chevronButton->setIconSize(QSize(16, 16));
            }
        } else {
            if (m_chevronAnimation) {
                m_chevronAnimation->hide();
            }
            m_chevronButton->setIcon(staticIcon);
            m_chevronButton->setIconSize(QSize(16, 16));
        }
    }
}

void FluentCard::updateCollapseIndicatorState(bool animated)
{
    if (!m_hasChevronAnimation || !m_chevronAnimation || !m_chevronAnimation->isLoaded()) {
        return;
    }

    const QString targetState = m_collapsed ? QStringLiteral("Collapsed") : QStringLiteral("Expanded");
    if (m_chevronAnimation->hasMarker(targetState)) {
        if (animated) {
            updateCollapseIndicatorGeometry();
            if (m_chevronButton) {
                m_chevronButton->setIcon(QIcon());
            }
            m_chevronAnimation->show();
            m_chevronAnimation->raise();

            const QString transition = m_chevronAnimation->state() + QStringLiteral("To") + targetState;
            const QString transitionStart = transition + QStringLiteral("_Start");
            const QString transitionEnd = transition + QStringLiteral("_End");
            if (m_chevronAnimation->hasMarker(transitionStart) && m_chevronAnimation->hasMarker(transitionEnd)) {
                syncCollapseIndicatorSpeed(m_chevronAnimation->markerFrame(transitionStart),
                                           m_chevronAnimation->markerFrame(transitionEnd));
            } else if (m_chevronAnimation->hasMarker(transition)) {
                syncCollapseIndicatorSpeed(m_chevronAnimation->markerFrame(transition),
                                           m_chevronAnimation->markerEndFrame(transition));
            }
        }
        m_chevronAnimation->setState(targetState, animated);
        updateHeaderUi();
        return;
    }

    const int lastFrame = qMax(0, m_chevronAnimation->totalFrames() - 1);
    const int middleFrame = qMax(0, lastFrame / 2);
    if (animated) {
        const int startFrame = m_collapsed ? middleFrame : 0;
        const int endFrame = m_collapsed ? lastFrame : middleFrame;
        syncCollapseIndicatorSpeed(startFrame, endFrame);
        updateCollapseIndicatorGeometry();
        if (m_chevronButton) {
            m_chevronButton->setIcon(QIcon());
        }
        m_chevronAnimation->show();
        m_chevronAnimation->raise();
        m_chevronAnimation->playSegment(startFrame, endFrame);
        updateHeaderUi();
        return;
    }

    m_chevronAnimation->pause();
    m_chevronAnimation->setCurrentFrame(m_collapsed ? lastFrame : middleFrame);
    updateHeaderUi();
}

void FluentCard::syncCollapseIndicatorSpeed(int startFrame, int endFrame)
{
    if (!m_chevronAnimation || !m_chevronAnimation->isLoaded()) {
        return;
    }

    const int frameDistance = qAbs(endFrame - startFrame);
    const qreal frameRate = m_chevronAnimation->frameRate();
    const int durationMs = qMax(1, m_collapseAnim ? m_collapseAnim->duration() : 160);
    if (frameDistance <= 0 || frameRate <= 0.0) {
        m_chevronAnimation->setSpeed(1.0);
        return;
    }

    m_chevronAnimation->setSpeed((frameDistance * 1000.0) / (frameRate * durationMs));
}

void FluentCard::updateCollapseIndicatorGeometry()
{
    if (!m_chevronButton || !m_chevronAnimation) {
        return;
    }

    const QSize size = m_chevronAnimation->size();
    const QPoint topLeft((m_chevronButton->width() - size.width()) / 2,
                         (m_chevronButton->height() - size.height()) / 2);
    m_chevronAnimation->setGeometry(QRect(topLeft, size));
}

void FluentCard::applyCollapsedState(bool animated)
{
    if (!m_collapsible) {
        return;
    }
    ensureCollapsibleUi();
    if (!m_content || !m_contentClip) {
        return;
    }

    if (m_collapseAnim) {
        FluentMotion::configure(m_collapseAnim, FluentMotionRole::Collapse);
    }
    if (!animated || !m_collapseAnim || m_collapseAnim->duration() <= 0) {
        setProperty(kFlowDisableAnimationProperty, false);
        if (m_collapseAnim) {
            m_collapseAnim->stop();
        }
        releaseContentHeightLock();
        if (m_collapsed) {
            m_contentClip->setRevealHeight(0);
            m_contentClip->setVisible(false);
        } else {
            m_contentClip->releaseRevealHeight();
            m_content->setVisible(true);
        }
        updateGeometry();
        return;
    }

    m_collapseAnim->stop();
    const int naturalHeight = m_contentClip->contentNaturalHeight();
    lockContentHeightForAnimation(naturalHeight);
    m_contentClip->setVisible(true);
    m_content->setVisible(true);
    setProperty(kFlowDisableAnimationProperty, true);

    const int start = m_contentClip->isVisible()
        ? (m_contentClip->revealHeight() >= 0 ? m_contentClip->revealHeight() : m_contentClip->height())
        : 0;
    const int end = m_collapsed ? 0 : naturalHeight;

    m_collapseAnim->setStartValue(start);
    m_collapseAnim->setEndValue(end);
    m_collapseAnim->start();
}

void FluentCard::finishCollapseAnimationImmediately()
{
    if (!m_collapseAnim || !m_content || !m_contentClip) {
        return;
    }

    m_collapseAnim->stop();
    setProperty(kFlowDisableAnimationProperty, false);
    releaseContentHeightLock();
    if (m_collapsed) {
        m_contentClip->setRevealHeight(0);
        m_contentClip->setVisible(false);
    } else {
        m_contentClip->releaseRevealHeight();
        m_content->setVisible(true);
    }
    updateCollapseIndicatorState(false);
    updateGeometry();
    update();
}

void FluentCard::startHoverAnimation(qreal endValue)
{
    if (!m_hoverAnim) {
        m_hoverLevel = qBound<qreal>(0.0, endValue, 1.0);
        update();
        return;
    }

    FluentMotion::configure(m_hoverAnim, FluentMotionRole::Hover);
    m_hoverAnim->stop();
    if (m_hoverAnim->duration() <= 0) {
        m_hoverLevel = qBound<qreal>(0.0, endValue, 1.0);
        update();
        return;
    }
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(qBound<qreal>(0.0, endValue, 1.0));
    m_hoverAnim->start();
}

void FluentCard::lockContentHeightForAnimation(int height)
{
    if (!m_content) {
        return;
    }
    const int clamped = qMax(0, height);
    m_content->setMinimumHeight(clamped);
    m_content->setMaximumHeight(clamped);
}

void FluentCard::releaseContentHeightLock()
{
    if (!m_content) {
        return;
    }
    m_content->setMinimumHeight(0);
    m_content->setMaximumHeight(QWIDGETSIZE_MAX);
}

void FluentCard::scheduleContentClipGeometryRefresh()
{
    if (!m_collapsible || !m_contentClip || !m_content || m_contentRefreshPending) {
        return;
    }

    m_contentRefreshPending = true;
    QTimer::singleShot(0, this, [this] {
        m_contentRefreshPending = false;
        refreshContentClipGeometry();
    });
}

void FluentCard::refreshContentClipGeometry()
{
    if (!m_contentClip || !m_content) {
        return;
    }

    if (!m_collapsed && (!m_collapseAnim || m_collapseAnim->state() != QAbstractAnimation::Running)) {
        releaseContentHeightLock();
        m_contentClip->releaseRevealHeight();
    } else {
        m_contentClip->refreshContentGeometry();
    }
}

} // namespace Fluent
