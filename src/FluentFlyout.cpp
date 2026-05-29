#include "Fluent/FluentFlyout.h"

#include "Fluent/FluentMotion.h"
#include "Fluent/FluentPopupSurface.h"
#include "Fluent/FluentTheme.h"
#include "FluentPopupUtils.h"

#include <QApplication>
#include <QCursor>
#include <QHideEvent>
#include <QPainter>
#include <QScreen>
#include <QShowEvent>
#include <QVariantAnimation>
#include <QVBoxLayout>

namespace Fluent {

FluentFlyout::FluentFlyout(QWidget *parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
    , m_contentMargins(12, 12, 12, 12)
    , m_border(this, this)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_StyledBackground, false);
    setAutoFillBackground(false);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(8);
    updateLayoutMargins();

    m_border.setRequestUpdate([this]() { update(); });
    m_border.syncFromTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
        if (isVisible()) {
            m_border.onThemeChanged();
        } else {
            m_border.syncFromTheme();
        }
        update();
    });
}

FluentFlyout::~FluentFlyout() = default;

QWidget *FluentFlyout::contentWidget() const
{
    return m_content.data();
}

void FluentFlyout::setContentWidget(QWidget *widget)
{
    if (m_content == widget) {
        return;
    }

    if (m_content) {
        m_layout->removeWidget(m_content);
        m_content->setParent(nullptr);
    }

    m_content = widget;
    if (m_content) {
        m_content->setParent(this);
        m_layout->addWidget(m_content);
    }
    adjustSize();
}

QMargins FluentFlyout::contentMargins() const
{
    return m_contentMargins;
}

void FluentFlyout::setContentMargins(const QMargins &margins)
{
    m_contentMargins = margins;
    updateLayoutMargins();
}

void FluentFlyout::showAt(const QPoint &globalPos)
{
    adjustSize();
    resize(popupWindowSize());
    startOpenAnimation(PopupSurface::topLeftForContentTopLeft(globalPos), -FluentMotion::popupSlideOffset());
}

void FluentFlyout::showFor(QWidget *target, Placement placement)
{
    if (!target) {
        showAt(QCursor::pos());
        return;
    }

    adjustSize();
    const QSize popupSize = popupWindowSize();
    resize(popupSize);
    const QRect anchor(target->mapToGlobal(QPoint(0, 0)), target->size());
    const int slideOffsetY = placement == Placement::Top
        ? FluentMotion::popupSlideOffset()
        : (placement == Placement::Bottom ? -FluentMotion::popupSlideOffset() : 0);
    startOpenAnimation(placedPosition(anchor, popupSize, placement), slideOffsetY);
}

void FluentFlyout::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    if (m_openAnim) {
        m_openAnim->stop();
        m_openAnim->deleteLater();
        m_openAnim = nullptr;
    }
    Detail::resetPopupOpenState(this);
    m_border.resetInitial();
}

void FluentFlyout::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    const auto &colors = ThemeManager::instance().colors();
    {
        QPainter clear(this);
        if (!clear.isActive()) {
            return;
        }
        clear.setCompositionMode(QPainter::CompositionMode_Source);
        clear.fillRect(rect(), Qt::transparent);
    }

    QPainter p(this);
    if (!p.isActive()) {
        return;
    }
    p.setRenderHint(QPainter::Antialiasing, true);
    PopupSurface::paintPanelWithShadowMargins(p, rect(), colors, &m_border);
}

void FluentFlyout::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    m_border.playInitialTraceOnce(0);
}

QPoint FluentFlyout::placedPosition(const QRect &anchor, const QSize &popupSize, Placement placement) const
{
    constexpr int gap = 8;
    const QSize contentSize = PopupSurface::contentSizeFromShadowSize(popupSize);
    QPoint contentPos;
    switch (placement) {
    case Placement::Top:
        contentPos = QPoint(anchor.center().x() - contentSize.width() / 2, anchor.top() - contentSize.height() - gap);
        break;
    case Placement::Right:
        contentPos = QPoint(anchor.right() + gap, anchor.center().y() - contentSize.height() / 2);
        break;
    case Placement::Left:
        contentPos = QPoint(anchor.left() - contentSize.width() - gap, anchor.center().y() - contentSize.height() / 2);
        break;
    case Placement::Bottom:
    default:
        contentPos = QPoint(anchor.center().x() - contentSize.width() / 2, anchor.bottom() + gap);
        break;
    }

    QPoint pos = PopupSurface::topLeftForContentTopLeft(contentPos);

    QScreen *screen = QApplication::screenAt(anchor.center());
    if (!screen) {
        screen = QApplication::primaryScreen();
    }
    const QRect avail = screen ? screen->availableGeometry() : QRect();
    if (avail.isValid()) {
        pos.setX(qBound(avail.left() + 4, pos.x(), avail.right() - popupSize.width() - 4));
        pos.setY(qBound(avail.top() + 4, pos.y(), avail.bottom() - popupSize.height() - 4));
    }
    return pos;
}

QSize FluentFlyout::popupWindowSize() const
{
    QSize preferred = size()
                          .expandedTo(sizeHint())
                          .expandedTo(minimumSizeHint())
                          .expandedTo(minimumSize());
    if (!preferred.isValid()) {
        preferred = size();
    }
    if (maximumSize().isValid()) {
        preferred = preferred.boundedTo(maximumSize());
    }
    return preferred;
}

bool FluentFlyout::popupRevealEnabled() const
{
    return true;
}

bool FluentFlyout::popupSlideEnabled() const
{
    return true;
}

void FluentFlyout::updateLayoutMargins()
{
    if (!m_layout) {
        return;
    }

    const QMargins shadow = PopupSurface::shadowMargins();
    m_layout->setContentsMargins(shadow.left() + m_contentMargins.left(),
                                 shadow.top() + m_contentMargins.top(),
                                 shadow.right() + m_contentMargins.right(),
                                 shadow.bottom() + m_contentMargins.bottom());
}

void FluentFlyout::startOpenAnimation(const QPoint &finalPos, int slideOffsetY)
{
    if (m_openAnim) {
        m_openAnim->stop();
        m_openAnim->deleteLater();
        m_openAnim = nullptr;
    }

    QRect targetGeometry(finalPos, size());
    const bool revealEnabled = popupRevealEnabled();
    const bool slideEnabled = popupSlideEnabled();
    const int effectiveSlideOffsetY = slideEnabled ? slideOffsetY : 0;
    if (!slideEnabled && isVisible()) {
        Detail::finishPopupOpen(this, targetGeometry);
        raise();
        return;
    }

    Detail::preparePopupOpen(this, targetGeometry, effectiveSlideOffsetY, revealEnabled);
    show();
    raise();

    const QSize shownSize = size();
    if (shownSize.isValid() && shownSize != targetGeometry.size()) {
        targetGeometry.setSize(shownSize);
        Detail::preparePopupOpen(this, targetGeometry, effectiveSlideOffsetY, revealEnabled);
    }

    m_openAnim = Detail::startPopupOpenAnimation(this, targetGeometry, effectiveSlideOffsetY, revealEnabled, [this]() {
        m_openAnim = nullptr;
    });
}

} // namespace Fluent
