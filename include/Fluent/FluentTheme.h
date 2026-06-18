#pragma once

#include <QObject>
#include <QColor>
#include <QList>
#include <QElapsedTimer>
#include <QEasingCurve>
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

struct FluentAccentRamp {
    QColor base;
    QColor light1;
    QColor light2;
    QColor light3;
    QColor dark1;
    QColor dark2;
    QColor dark3;
};

struct FluentNeutralRamp {
    QColor background;
    QColor layer;
    QColor layerAlt;
    QColor card;
    QColor cardHover;
    QColor stroke;
    QColor strokeSubtle;
    QColor strokeStrong;
    QColor fillSecondary;
    QColor fillTertiary;
};

struct FluentSemanticRamp {
    QColor info;
    QColor success;
    QColor warning;
    QColor error;
};

struct FluentRadiusTokens {
    int control = 6;
    int overlay = 8;
    int card = 10;
    int window = 12;
    int pill = 999;
};

struct FluentSpacingTokens {
    int xxs = 2;
    int xs = 4;
    int s = 6;
    int m = 8;
    int l = 12;
    int xl = 16;
    int xxl = 24;
};

struct FluentTypographyTokens {
    QString family = QStringLiteral("'Segoe UI', 'Microsoft YaHei', 'Microsoft YaHei UI', sans-serif");
    int caption = 12;
    int body = 14;
    int bodyLarge = 16;
    int subtitle = 18;
    int title = 20;
    int titleLarge = 28;
};

struct FluentElevationTokens {
    QColor shadow;
    int lowBlur = 10;
    int mediumBlur = 18;
    int highBlur = 28;
    int lowYOffset = 2;
    int mediumYOffset = 6;
    int highYOffset = 12;
};

struct FluentMotionTokens {
    int hoverDuration = 120;
    int pressDuration = 90;
    int focusDuration = 180;
    int popupOpenDuration = 150;
    int popupCloseDuration = 100;
    int collapseDuration = 180;
    int selectionDuration = 180;
    int navigationDuration = 220;
    int layoutDuration = 140;
    int pageDuration = 300;
    int toastDuration = 180;
    int wheelSnapDuration = 120;
    int popupSlideOffset = 8;
    int pressOffset = 1;
    QEasingCurve::Type easeOut = QEasingCurve::OutCubic;
    QEasingCurve::Type easeIn = QEasingCurve::InCubic;
    QEasingCurve::Type easeInOut = QEasingCurve::InOutCubic;
};

struct FluentThemeTokens {
    ThemeColors legacyColors;
    FluentAccentRamp accent;
    FluentNeutralRamp neutral;
    FluentSemanticRamp semantic;
    FluentRadiusTokens radius;
    FluentSpacingTokens spacing;
    FluentTypographyTokens typography;
    FluentElevationTokens elevation;
    FluentMotionTokens motion;
    QColor onAccent;
    bool dark = false;
};

class Theme final
{
public:
    static ThemeColors light();
    static ThemeColors dark();  // Added dark mode support
    static bool isDark(const ThemeColors &colors);
    static QColor contrastColor(const QColor &background);
    static QColor accentForMode(const QColor &accent, bool dark);
    static FluentAccentRamp accentRamp(const QColor &accent, bool dark);
    static FluentNeutralRamp neutralRamp(const ThemeColors &colors);
    static FluentSemanticRamp semanticRamp(const ThemeColors &colors);
    static FluentThemeTokens tokens(const ThemeColors &colors);
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

    // Accent border rendering style. Flow animates a rotating accent-tinted
    // conic gradient around the window edge (only when accentBorderEnabled).
    enum class AccentBorderStyle {
        Solid,
        Flow
    };

    static ThemeManager &instance();

    const ThemeColors &colors() const;
    const FluentThemeTokens &tokens() const;
    void setColors(const ThemeColors &colors);
    void setAccentColor(const QColor &accent);

    bool accentBorderEnabled() const;
    void setAccentBorderEnabled(bool enabled);

    AccentBorderStyle accentBorderStyle() const;
    void setAccentBorderStyle(AccentBorderStyle style);

    // Flow gradient palette shared by the Flow accent border (conic) and
    // FluentCard's flow background (linear). Empty => derived from the accent.
    QList<QColor> flowGradientColors() const;
    void setFlowGradientColors(const QList<QColor> &colors);
    // Stops actually used for painting: the custom palette if it has >= 2
    // colors, otherwise an accent-derived set. Always returns >= 2 colors.
    QList<QColor> resolvedFlowColors() const;

    bool animationsEnabled() const;
    void setAnimationsEnabled(bool enabled);

    const FluentMotionTokens &motionTokens() const;
    void setMotionTokens(const FluentMotionTokens &tokens);
    void resetMotionTokens();
    
    ThemeMode themeMode() const;
    void setThemeMode(ThemeMode mode);
    void setLightMode();
    void setDarkMode();

signals:
    void themeChanged();

private:
    ThemeManager();

    void scheduleThemeChanged(const QString &reason = QString());
    void setColorsInternal(const ThemeColors &colors, bool updateBaseAccent, const QString &reason = QString());

    ThemeColors m_colors;
    FluentThemeTokens m_tokens;
    FluentMotionTokens m_motionTokens;
    QColor m_baseAccent;
    ThemeMode m_mode = ThemeMode::Light;

    bool m_accentBorderEnabled = true;
    AccentBorderStyle m_accentBorderStyle = AccentBorderStyle::Solid;
    QList<QColor> m_flowGradientColors;
    bool m_animationsEnabled = true;
    bool m_themeChangedPending = false;
    QElapsedTimer m_themeChangeTimer;
    QString m_themeChangeReason;
    int m_themeChangeSequence = 0;
};

} // namespace Fluent
