#pragma once

#include "Fluent/FluentAccentBorderTrace.h"
#include "Fluent/FluentFramePainter.h"
#include "Fluent/FluentTheme.h"

#include <QColor>
#include <QTimer>

namespace Fluent {

// Easy-to-use helper to add Fluent accent border + trace animation to any QWidget.
//
// Typical usage:
//   class Foo : public QWidget {
//     FluentBorderEffect m_border{this};
//     ...
//     Foo() {
//       m_border.syncFromTheme();
//       connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]{
//         m_border.onThemeChanged();
//       });
//     }
//     void showEvent(QShowEvent*) override { m_border.playInitialTraceOnce(); }
//     void paintEvent(QPaintEvent*) override {
//       QPainter p(this);
//       FluentFrameSpec frame; ...
//       m_border.applyToFrameSpec(frame, ThemeManager::instance().colors());
//       paintFluentFrame(p, rect(), ThemeManager::instance().colors(), frame);
//     }
//   };
class FluentBorderEffect final
{
public:
    explicit FluentBorderEffect(QWidget *updateTarget, QObject *parent = nullptr)
        : m_trace(updateTarget, parent)
    {
    }

    void syncEnabled(bool accentEnabled) { m_trace.setCurrentEnabled(accentEnabled); }
    bool enabled() const { return m_trace.currentEnabled(); }

    bool fromEnabled() const { return m_trace.fromEnabled(); }
    bool toEnabled() const { return m_trace.toEnabled(); }

    void onAccentEnabledChanged(bool accentEnabled) { m_trace.onEnabledChanged(accentEnabled); }

    void syncFromTheme() { syncEnabled(ThemeManager::instance().accentBorderEnabled()); }
    void onThemeChanged() { onAccentEnabledChanged(ThemeManager::instance().accentBorderEnabled()); }

    // Plays a single "trace-in" once per instance (useful when a popup/dialog appears).
    void playInitialTraceOnce(int delayMs = 0)
    {
        if (m_initialPlayed) {
            return;
        }
        m_initialPlayed = true;

        if (!ThemeManager::instance().accentBorderEnabled()) {
            return;
        }

        if (delayMs <= 0) {
            m_trace.play(false, true);
            return;
        }

        QTimer::singleShot(delayMs, [this]() {
            if (ThemeManager::instance().accentBorderEnabled()) {
                m_trace.play(false, true);
            }
        });
    }

    void resetInitial() { m_initialPlayed = false; }

    bool isAnimating() const { return m_trace.isAnimating(); }
    qreal t() const { return m_trace.t(); }

    void setRequestUpdate(std::function<void()> requestUpdate) { m_trace.setRequestUpdate(std::move(requestUpdate)); }

    // Apply border/trace fields to a FluentFrameSpec.
    // You can override normal/accent colors (e.g. Toast uses alpha-modified colors).
    void applyToFrameSpec(
        FluentFrameSpec &frame,
        const ThemeColors &colors,
        const QColor &normalBorderOverride = QColor(),
        const QColor &accentBorderOverride = QColor()) const
    {
        const QColor normalBorder = normalBorderOverride.isValid() ? normalBorderOverride : colors.border;
        const QColor accentBorder = accentBorderOverride.isValid() ? accentBorderOverride : colors.accent;

        const bool animating = m_trace.isAnimating();
        const QColor fromBorderColor = m_trace.fromEnabled() ? accentBorder : normalBorder;
        const QColor toBorderColor = m_trace.toEnabled() ? accentBorder : normalBorder;

        const QColor baseBorderColor = animating
            ? fromBorderColor
            : (m_trace.currentEnabled() ? accentBorder : normalBorder);

        frame.accentBorderEnabled = m_trace.currentEnabled();
        frame.borderColorOverride = baseBorderColor;
        frame.traceEnabled = animating;
        frame.traceColor = toBorderColor;
        frame.traceT = m_trace.t();
    }

private:
    FluentAccentBorderTrace m_trace;
    bool m_initialPlayed = false;
};

} // namespace Fluent
