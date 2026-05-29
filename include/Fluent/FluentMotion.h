#pragma once

#include "Fluent/FluentTheme.h"

#include <QEasingCurve>
#include <QPropertyAnimation>
#include <QVariantAnimation>

namespace Fluent {

enum class FluentMotionRole {
    Hover,
    Press,
    Focus,
    PopupOpen,
    PopupClose,
    Collapse,
    Selection,
    Navigation,
    Layout,
    Page,
    Toast,
    WheelSnap
};

namespace FluentMotion {

inline const FluentMotionTokens &tokens()
{
    return ThemeManager::instance().tokens().motion;
}

inline FluentMotionTokens defaultTokens()
{
    return FluentMotionTokens{};
}

inline bool animationsEnabled()
{
    return ThemeManager::instance().animationsEnabled();
}

inline int configuredDuration(FluentMotionRole role)
{
    const auto &m = tokens();
    switch (role) {
    case FluentMotionRole::Hover:
        return m.hoverDuration;
    case FluentMotionRole::Press:
        return m.pressDuration;
    case FluentMotionRole::Focus:
        return m.focusDuration;
    case FluentMotionRole::PopupOpen:
        return m.popupOpenDuration;
    case FluentMotionRole::PopupClose:
        return m.popupCloseDuration;
    case FluentMotionRole::Collapse:
        return m.collapseDuration;
    case FluentMotionRole::Selection:
        return m.selectionDuration;
    case FluentMotionRole::Navigation:
        return m.navigationDuration;
    case FluentMotionRole::Layout:
        return m.layoutDuration;
    case FluentMotionRole::Page:
        return m.pageDuration;
    case FluentMotionRole::Toast:
        return m.toastDuration;
    case FluentMotionRole::WheelSnap:
        return m.wheelSnapDuration;
    }
    return m.hoverDuration;
}

inline int duration(FluentMotionRole role)
{
    if (!animationsEnabled()) {
        return 0;
    }
    return configuredDuration(role);
}

inline QEasingCurve easing(FluentMotionRole role)
{
    const auto &m = tokens();
    switch (role) {
    case FluentMotionRole::PopupClose:
        return QEasingCurve(m.easeIn);
    case FluentMotionRole::Selection:
    case FluentMotionRole::Page:
        return QEasingCurve(m.easeInOut);
    case FluentMotionRole::Hover:
    case FluentMotionRole::Press:
    case FluentMotionRole::Focus:
    case FluentMotionRole::PopupOpen:
    case FluentMotionRole::Collapse:
    case FluentMotionRole::Navigation:
    case FluentMotionRole::Layout:
    case FluentMotionRole::Toast:
    case FluentMotionRole::WheelSnap:
    default:
        return QEasingCurve(m.easeOut);
    }
}

inline int popupSlideOffset()
{
    return tokens().popupSlideOffset;
}

inline int pressOffset()
{
    return tokens().pressOffset;
}

inline void setTokens(const FluentMotionTokens &tokens)
{
    ThemeManager::instance().setMotionTokens(tokens);
}

inline void resetTokens()
{
    ThemeManager::instance().resetMotionTokens();
}

inline void setDuration(FluentMotionRole role, int milliseconds)
{
    FluentMotionTokens next = tokens();
    switch (role) {
    case FluentMotionRole::Hover:
        next.hoverDuration = milliseconds;
        break;
    case FluentMotionRole::Press:
        next.pressDuration = milliseconds;
        break;
    case FluentMotionRole::Focus:
        next.focusDuration = milliseconds;
        break;
    case FluentMotionRole::PopupOpen:
        next.popupOpenDuration = milliseconds;
        break;
    case FluentMotionRole::PopupClose:
        next.popupCloseDuration = milliseconds;
        break;
    case FluentMotionRole::Collapse:
        next.collapseDuration = milliseconds;
        break;
    case FluentMotionRole::Selection:
        next.selectionDuration = milliseconds;
        break;
    case FluentMotionRole::Navigation:
        next.navigationDuration = milliseconds;
        break;
    case FluentMotionRole::Layout:
        next.layoutDuration = milliseconds;
        break;
    case FluentMotionRole::Page:
        next.pageDuration = milliseconds;
        break;
    case FluentMotionRole::Toast:
        next.toastDuration = milliseconds;
        break;
    case FluentMotionRole::WheelSnap:
        next.wheelSnapDuration = milliseconds;
        break;
    }
    setTokens(next);
}

inline void configure(QVariantAnimation *animation, FluentMotionRole role)
{
    if (!animation) {
        return;
    }
    animation->setDuration(duration(role));
    animation->setEasingCurve(easing(role));
}

inline void configure(QPropertyAnimation *animation, FluentMotionRole role)
{
    if (!animation) {
        return;
    }
    animation->setDuration(duration(role));
    animation->setEasingCurve(easing(role));
}

} // namespace FluentMotion
} // namespace Fluent
