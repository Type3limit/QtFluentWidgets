#pragma once

#include "Fluent/FluentMotion.h"
#include "Fluent/FluentPopupSurface.h"

#include <QApplication>
#include <QEasingCurve>
#include <QRect>
#include <QRegion>
#include <QScreen>
#include <QVariantAnimation>
#include <QWidget>

#include <cmath>
#include <functional>

namespace Fluent::Detail {

struct PopupPlacementResult {
    QRect geometry;
    int slideOffsetY = -FluentMotion::popupSlideOffset();
    bool placedBelow = true;
};

inline QRect popupAvailableGeometryFor(const QRect &anchor)
{
    QScreen *screen = QApplication::screenAt(anchor.center());
    if (!screen) {
        screen = QApplication::primaryScreen();
    }
    return screen ? screen->availableGeometry() : QRect();
}

inline PopupPlacementResult placePopupBelowOrAbove(const QRect &anchor,
                                                   const QSize &shadowSize,
                                                   int gap,
                                                   int horizontalMargin = 4)
{
    PopupPlacementResult result;
    if (!anchor.isValid() || !shadowSize.isValid()) {
        result.geometry = QRect(anchor.topLeft(), shadowSize);
        return result;
    }

    const QRect available = popupAvailableGeometryFor(anchor);
    const QSize contentSize = PopupSurface::contentSizeFromShadowSize(shadowSize);
    const int belowTop = anchor.top() + anchor.height() + gap;
    const int aboveTop = anchor.top() - gap - contentSize.height();

    auto shadowGeometry = [&shadowSize](const QPoint &contentTopLeft) {
        return QRect(PopupSurface::topLeftForContentTopLeft(contentTopLeft), shadowSize);
    };

    const QPoint belowContent(anchor.left(), belowTop);
    const QPoint aboveContent(anchor.left(), aboveTop);
    const bool fitsBelow = !available.isValid()
        || shadowGeometry(belowContent).bottom() <= available.bottom() + 1;
    const bool fitsAbove = !available.isValid()
        || shadowGeometry(aboveContent).top() >= available.top();

    result.placedBelow = fitsBelow || !fitsAbove;
    result.slideOffsetY = result.placedBelow
        ? -FluentMotion::popupSlideOffset()
        : FluentMotion::popupSlideOffset();
    result.geometry = shadowGeometry(result.placedBelow ? belowContent : aboveContent);

    if (available.isValid()) {
        if (result.geometry.left() < available.left() + horizontalMargin) {
            result.geometry.moveLeft(available.left() + horizontalMargin);
        }
        if (result.geometry.right() > available.right() - horizontalMargin) {
            result.geometry.moveRight(available.right() - horizontalMargin);
        }
        if (result.geometry.top() < available.top() + horizontalMargin) {
            result.geometry.moveTop(available.top() + horizontalMargin);
        }
        if (result.geometry.bottom() > available.bottom() - horizontalMargin) {
            result.geometry.moveBottom(available.bottom() - horizontalMargin);
        }
    }

    return result;
}

inline PopupPlacementResult placePopupBelowOrAbove(QWidget *anchor,
                                                   const QSize &shadowSize,
                                                   int gap,
                                                   int horizontalMargin = 4)
{
    if (!anchor) {
        return {};
    }

    return placePopupBelowOrAbove(QRect(anchor->mapToGlobal(QPoint(0, 0)), anchor->size()),
                                  shadowSize,
                                  gap,
                                  horizontalMargin);
}

inline QRect popupRevealMaskRect(const QRect &popupRect, int slideOffsetY, qreal progress)
{
    const qreal revealProgress = qBound<qreal>(0.52, progress, 1.0);
    const int revealHeight = qBound(1,
                                    static_cast<int>(std::ceil(popupRect.height() * revealProgress)),
                                    popupRect.height());
    if (slideOffsetY > 0) {
        return QRect(0, popupRect.height() - revealHeight, popupRect.width(), revealHeight);
    }
    return QRect(0, 0, popupRect.width(), revealHeight);
}

inline void applyPopupRevealMask(QWidget *popup, int slideOffsetY, qreal progress, bool revealEnabled = true)
{
    if (!popup) {
        return;
    }

    if (!revealEnabled || slideOffsetY == 0 || progress >= 0.995) {
        popup->clearMask();
        return;
    }

    popup->setMask(QRegion(popupRevealMaskRect(popup->rect(), slideOffsetY, progress)));
}

inline void applyPopupOpenProgress(QWidget *popup,
                                   const QRect &targetGeometry,
                                   int slideOffsetY,
                                   qreal progress,
                                   bool revealEnabled = true)
{
    if (!popup) {
        return;
    }

    const qreal t = qBound<qreal>(0.0, progress, 1.0);
    if (targetGeometry.isValid()) {
        if (popup->size() != targetGeometry.size()) {
            popup->resize(targetGeometry.size());
        }
        QPoint topLeft = targetGeometry.topLeft();
        topLeft.ry() += static_cast<int>(std::lround((1.0 - t) * slideOffsetY));
        if (popup->pos() != topLeft) {
            popup->move(topLeft);
        }
    }
    popup->setWindowOpacity(t);
    applyPopupRevealMask(popup, slideOffsetY, t, revealEnabled);
}

inline void preparePopupOpen(QWidget *popup,
                             const QRect &targetGeometry,
                             int slideOffsetY,
                             bool revealEnabled = true)
{
    if (!popup) {
        return;
    }

    if (targetGeometry.isValid()) {
        if (popup->size() != targetGeometry.size()) {
            popup->resize(targetGeometry.size());
        }
        QPoint topLeft = targetGeometry.topLeft();
        topLeft.ry() += slideOffsetY;
        if (popup->pos() != topLeft) {
            popup->move(topLeft);
        }
    }
    popup->setWindowOpacity(0.0);
    applyPopupRevealMask(popup, slideOffsetY, 0.0, revealEnabled);
}

inline void finishPopupOpen(QWidget *popup, const QRect &targetGeometry)
{
    if (!popup) {
        return;
    }

    popup->setWindowOpacity(1.0);
    if (targetGeometry.isValid()) {
        if (popup->size() != targetGeometry.size()) {
            popup->resize(targetGeometry.size());
        }
        if (popup->pos() != targetGeometry.topLeft()) {
            popup->move(targetGeometry.topLeft());
        }
    }
    popup->clearMask();
}

inline void resetPopupOpenState(QWidget *popup)
{
    if (!popup) {
        return;
    }

    popup->setWindowOpacity(1.0);
    popup->clearMask();
}

inline QVariantAnimation *startPopupOpenAnimation(QWidget *popup,
                                                  const QRect &targetGeometry,
                                                  int slideOffsetY,
                                                  bool revealEnabled = true,
                                                  std::function<void()> finished = {})
{
    auto *animation = new QVariantAnimation(popup);
    FluentMotion::configure(animation, FluentMotionRole::PopupOpen);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    QObject::connect(animation, &QVariantAnimation::valueChanged, popup,
                     [popup, targetGeometry, slideOffsetY, revealEnabled](const QVariant &value) {
                         applyPopupOpenProgress(popup, targetGeometry, slideOffsetY, value.toReal(), revealEnabled);
                     });
    QObject::connect(animation, &QVariantAnimation::finished, popup,
                     [popup, targetGeometry, animation, finished = std::move(finished)]() mutable {
                         finishPopupOpen(popup, targetGeometry);
                         if (finished) {
                             finished();
                         }
                         animation->deleteLater();
                     });
    if (animation->duration() <= 0) {
        finishPopupOpen(popup, targetGeometry);
        if (finished) {
            finished();
        }
        animation->deleteLater();
        return nullptr;
    }

    animation->start();
    return animation;
}

} // namespace Fluent::Detail
