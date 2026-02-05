#pragma once

#include "Fluent/FluentStyle.h"

#include <functional>
#include <QPointer>
#include <QTimer>
#include <utility>
#include <QVariantAnimation>
#include <QWidget>

namespace Fluent {

// Shared controller for the Win11-like accent border trace animation.
//
// Design goals:
// - No Q_OBJECT / moc required
// - Owns its internal QVariantAnimation
// - Exposes (from/to enabled, t) so callers can decide actual colors
class FluentAccentBorderTrace final : public QObject
{
public:
    explicit FluentAccentBorderTrace(QWidget *updateTarget, QObject *parent = nullptr)
        : QObject(parent)
        , m_target(updateTarget)
    {
    }

    // Optional: provide a custom update hook (e.g. update multiple widgets/overlays).
    // If set, it is used instead of calling updateTarget->update().
    void setRequestUpdate(std::function<void()> requestUpdate)
    {
        m_requestUpdate = std::move(requestUpdate);
    }

    void setCurrentEnabled(bool enabled)
    {
        m_currentEnabled = enabled;
        stop();
    }

    bool currentEnabled() const { return m_currentEnabled; }

    void onEnabledChanged(bool enabled)
    {
        if (enabled == m_currentEnabled) {
            return;
        }
        play(m_currentEnabled, enabled);
        m_currentEnabled = enabled;
    }

    void play(bool fromEnabled, bool toEnabled)
    {
        // "关闭描边"：不播放关闭动画，直接停掉。
        if (!toEnabled) {
            stop();
            return;
        }

        if (fromEnabled == toEnabled) {
            m_t = -1.0;
            return;
        }

        m_fromEnabled = fromEnabled;
        m_toEnabled = toEnabled;

        ensureAnim();

        const auto wm = Style::windowMetrics();
        m_anim->setDuration(wm.accentBorderTraceDurationMs);
        m_anim->setEasingCurve(toEnabled ? wm.accentBorderTraceEnableEasing : wm.accentBorderTraceDisableEasing);

        QVariantAnimation::KeyValues kv;
        kv << QVariantAnimation::KeyValue(0.0, 0.0);
        if (toEnabled && wm.accentBorderTraceEnableOvershoot > 0.0) {
            const qreal at = qBound<qreal>(0.0, wm.accentBorderTraceEnableOvershootAt, 0.99);
            kv << QVariantAnimation::KeyValue(at, 1.0 + wm.accentBorderTraceEnableOvershoot);
        }
        kv << QVariantAnimation::KeyValue(1.0, 1.0);
        m_anim->setKeyValues(kv);

        m_anim->stop();
        m_t = 0.0;
        requestUpdate();
        m_anim->setStartValue(0.0);
        m_anim->setEndValue(1.0);
        m_anim->start();
    }

    void stop()
    {
        if (m_anim) {
            m_anim->stop();
        }
        m_t = -1.0;
        requestUpdate();
    }

    bool isAnimating() const { return m_t >= 0.0; }
    qreal t() const { return m_t; }

    bool fromEnabled() const { return m_fromEnabled; }
    bool toEnabled() const { return m_toEnabled; }

private:
    void ensureAnim()
    {
        if (m_anim) {
            return;
        }

        m_anim = new QVariantAnimation(this);
        QObject::connect(m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
            m_t = v.toReal();
            requestUpdate();
        });
        QObject::connect(m_anim, &QVariantAnimation::finished, this, [this]() {
            m_t = -1.0;
            requestUpdate();
        });
    }

    void requestUpdate()
    {
        if (m_requestUpdate) {
            m_requestUpdate();
            return;
        }
        if (m_target) {
            m_target->update();
        }
    }

    QPointer<QWidget> m_target;
    std::function<void()> m_requestUpdate;

    bool m_currentEnabled = true;

    bool m_fromEnabled = true;
    bool m_toEnabled = true;

    qreal m_t = -1.0;
    QVariantAnimation *m_anim = nullptr;
};

} // namespace Fluent
