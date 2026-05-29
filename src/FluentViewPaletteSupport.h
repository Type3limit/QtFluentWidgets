#pragma once

#include "Fluent/FluentTheme.h"

#include <QPalette>
#include <QWidget>

namespace Fluent::Detail {

inline QPalette fluentViewPalette(const QPalette &base, const ThemeColors &colors)
{
    QPalette palette = base;
    palette.setColor(QPalette::Window, colors.surface);
    palette.setColor(QPalette::Base, colors.surface);
    palette.setColor(QPalette::AlternateBase, colors.hover);
    palette.setColor(QPalette::WindowText, colors.text);
    palette.setColor(QPalette::Text, colors.text);
    palette.setColor(QPalette::ButtonText, colors.text);
    palette.setColor(QPalette::Mid, colors.border);
    palette.setColor(QPalette::Light, colors.hover);
    palette.setColor(QPalette::Dark, colors.pressed);
    palette.setColor(QPalette::Highlight, colors.accent);
    palette.setColor(QPalette::HighlightedText, Theme::contrastColor(colors.accent));
    palette.setColor(QPalette::PlaceholderText, colors.disabledText);
    palette.setColor(QPalette::Disabled, QPalette::Window, colors.surface);
    palette.setColor(QPalette::Disabled, QPalette::Base, colors.surface);
    palette.setColor(QPalette::Disabled, QPalette::AlternateBase, colors.hover);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, colors.disabledText);
    palette.setColor(QPalette::Disabled, QPalette::Text, colors.disabledText);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, colors.disabledText);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, colors.hover);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, colors.disabledText);
    return palette;
}

inline void applyFluentViewPalette(QWidget *widget, QWidget *viewport, const ThemeColors &colors)
{
    if (!widget) {
        return;
    }

    const QPalette palette = fluentViewPalette(widget->palette(), colors);
    widget->setPalette(palette);
    if (viewport) {
        viewport->setPalette(palette);
    }
}

} // namespace Fluent::Detail
