#pragma once

#include <QObject>
#include <QColor>
#include <QString>

namespace Fluent {

struct ThemeColors {
    QColor accent;
    QColor text;
    QColor subText;
    QColor disabledText;
    QColor background;
    QColor surface;
    QColor border;
    QColor hover;
    QColor pressed;
    QColor focus;
    QColor error;
};

class Theme final
{
public:
    static ThemeColors light();
    static ThemeColors dark();  // Added dark mode support
    static QString baseStyleSheet(const ThemeColors &colors);
    static QString buttonStyle(const ThemeColors &colors, bool primary);
    static QString labelStyle(const ThemeColors &colors);
    static QString lineEditStyle(const ThemeColors &colors);
    static QString textEditStyle(const ThemeColors &colors);
    static QString dateTimeStyle(const ThemeColors &colors);
    static QString calendarPopupStyle(const ThemeColors &colors);
    static QString checkBoxStyle(const ThemeColors &colors);
    static QString radioButtonStyle(const ThemeColors &colors);
    static QString toggleSwitchStyle(const ThemeColors &colors);
    static QString comboBoxStyle(const ThemeColors &colors);
    static QString sliderStyle(const ThemeColors &colors);
    static QString progressBarStyle(const ThemeColors &colors);
    static QString spinBoxStyle(const ThemeColors &colors);
    static QString toolButtonStyle(const ThemeColors &colors);
    static QString tabWidgetStyle(const ThemeColors &colors);
    static QString listViewStyle(const ThemeColors &colors);
    static QString tableViewStyle(const ThemeColors &colors);
    static QString treeViewStyle(const ThemeColors &colors);
    static QString groupBoxStyle(const ThemeColors &colors);
    static QString menuBarStyle(const ThemeColors &colors);
    static QString toolBarStyle(const ThemeColors &colors);
    static QString statusBarStyle(const ThemeColors &colors);
    static QString dialogStyle(const ThemeColors &colors);
    static QString cardStyle(const ThemeColors &colors);
};

class ThemeManager final : public QObject
{
    Q_OBJECT
public:
    enum class ThemeMode {
        Light,
        Dark
    };
    
    static ThemeManager &instance();

    const ThemeColors &colors() const;
    void setColors(const ThemeColors &colors);

    bool accentBorderEnabled() const;
    void setAccentBorderEnabled(bool enabled);
    
    ThemeMode themeMode() const;
    void setThemeMode(ThemeMode mode);
    void setLightMode();
    void setDarkMode();

signals:
    void themeChanged();

private:
    ThemeManager();

    void scheduleThemeChanged();

    ThemeColors m_colors;
    ThemeMode m_mode = ThemeMode::Light;

    bool m_accentBorderEnabled = true;
    bool m_themeChangedPending = false;
};

} // namespace Fluent
