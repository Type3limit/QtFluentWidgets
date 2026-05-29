#include "Fluent/FluentTheme.h"

#include "Fluent/FluentDiagnostics.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentToolTip.h"

#include <QApplication>
#include <QDebug>
#include <QTimer>
#include <QWidget>
#include <QtMath>
#include <QtGlobal>

namespace Fluent {

namespace {

qreal linearColorChannel(qreal value)
{
  return value <= 0.03928 ? value / 12.92 : qPow((value + 0.055) / 1.055, 2.4);
}

qreal relativeLuminance(const QColor &color)
{
  return 0.2126 * linearColorChannel(color.redF())
       + 0.7152 * linearColorChannel(color.greenF())
       + 0.0722 * linearColorChannel(color.blueF());
}

qreal contrastRatio(const QColor &a, const QColor &b)
{
  const qreal la = relativeLuminance(a);
  const qreal lb = relativeLuminance(b);
  const qreal lighter = qMax(la, lb);
  const qreal darker = qMin(la, lb);
  return (lighter + 0.05) / (darker + 0.05);
}

} // namespace

ThemeColors Theme::light() {
  ThemeColors colors;
  // Islands Style - High Contrast Fluent Design (Light Mode)
  colors.accent = QColor("#0066B4");       // QFluentKit-like default blue
  colors.text = QColor("#1A1A1A");         // Almost black - highest contrast
  colors.subText = QColor("#5A5A5A");      // Medium gray
  colors.disabledText = QColor("#999999"); // Light gray
  colors.background = QColor("#F5F5F5");   // Light gray background
  colors.surface = QColor("#FFFFFF");      // Pure white cards
  colors.border = QColor("#E0E0E0");       // Visible borders
  colors.hover = QColor("#F0F0F0");        // Subtle hover
  colors.pressed = QColor("#E5E5E5");      // Clear pressed state
  colors.focus = QColor("#0067C0");        // Match accent
  colors.error = QColor("#C50F1F");        // Vivid red
  return colors;
}

ThemeColors Theme::dark() {
  ThemeColors colors;
  // Fluent Design Dark Mode
  colors.accent = accentForMode(QColor("#0066B4"), true);
  colors.text = QColor("#FFFFFF");    // Pure white text
  colors.subText = QColor("#C0C0C0"); // Light gray
  colors.disabledText = QColor("#6D6D6D"); // Mid gray
  colors.background = QColor("#202020");   // Dark gray background
  colors.surface = QColor("#2D2D2D");      // Slightly lighter surface
  colors.border = QColor("#3D3D3D");       // Subtle borders
  colors.hover = QColor("#383838");        // Lighter hover
  colors.pressed = QColor("#4D4D4D");      // Even lighter pressed
  colors.focus = QColor("#60CDFF");        // Match accent
  colors.error = QColor("#FF6B6B");        // Soft red
  return colors;
}

bool Theme::isDark(const ThemeColors &colors)
{
  return colors.background.lightnessF() < 0.5;
}

QColor Theme::contrastColor(const QColor &background)
{
  const QColor black("#000000");
  const QColor white("#FFFFFF");
  return contrastRatio(background, black) >= contrastRatio(background, white) ? black : white;
}

QColor Theme::accentForMode(const QColor &accent, bool dark)
{
  if (!accent.isValid()) {
    return dark ? QColor("#29A2FF") : QColor("#0066B4");
  }

  if (!dark) {
    return accent;
  }

  qreal h = 0.0;
  qreal s = 0.0;
  qreal v = 0.0;
  qreal a = 1.0;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  float hf = 0.0f;
  float sf = 0.0f;
  float vf = 0.0f;
  float af = 1.0f;
  accent.getHsvF(&hf, &sf, &vf, &af);
  h = hf;
  s = sf;
  v = vf;
  a = af;
#else
  accent.getHsvF(&h, &s, &v, &a);
#endif

  if (h < 0.0) {
    return accent;
  }

  s *= 0.84;
  v = 1.0;
  return QColor::fromHsvF(h, qBound<qreal>(0.0, s, 1.0), qBound<qreal>(0.0, v, 1.0), a);
}

FluentAccentRamp Theme::accentRamp(const QColor &accent, bool dark)
{
  FluentAccentRamp ramp;
  ramp.base = accent;
  const QColor white("#FFFFFF");
  const QColor black("#000000");

  ramp.light1 = Style::mix(accent, white, dark ? 0.18 : 0.12);
  ramp.light2 = Style::mix(accent, white, dark ? 0.34 : 0.24);
  ramp.light3 = Style::mix(accent, white, dark ? 0.50 : 0.40);
  ramp.dark1 = Style::mix(accent, black, dark ? 0.12 : 0.10);
  ramp.dark2 = Style::mix(accent, black, dark ? 0.24 : 0.22);
  ramp.dark3 = Style::mix(accent, black, dark ? 0.38 : 0.36);
  return ramp;
}

FluentNeutralRamp Theme::neutralRamp(const ThemeColors &colors)
{
  const bool darkMode = isDark(colors);
  const QColor black("#000000");
  const QColor white("#FFFFFF");

  FluentNeutralRamp ramp;
  ramp.background = colors.background;
  ramp.layer = darkMode ? Style::mix(colors.background, white, 0.055) : colors.surface;
  ramp.layerAlt = darkMode ? Style::mix(colors.background, white, 0.08) : QColor("#FAFAFA");
  ramp.card = colors.surface;
  ramp.cardHover = darkMode ? Style::mix(colors.surface, white, 0.055) : Style::mix(colors.surface, black, 0.025);
  ramp.stroke = colors.border;
  ramp.strokeSubtle = darkMode ? Style::mix(colors.border, colors.surface, 0.42) : Style::mix(colors.border, colors.surface, 0.55);
  ramp.strokeStrong = darkMode ? Style::mix(colors.border, white, 0.24) : Style::mix(colors.border, black, 0.18);
  ramp.fillSecondary = colors.hover;
  ramp.fillTertiary = colors.pressed;
  return ramp;
}

FluentSemanticRamp Theme::semanticRamp(const ThemeColors &colors)
{
  const bool darkMode = isDark(colors);
  FluentSemanticRamp ramp;
  ramp.info = colors.accent;
  ramp.success = darkMode ? QColor("#6CCB5F") : QColor("#107C10");
  ramp.warning = darkMode ? QColor("#FFB900") : QColor("#F7630C");
  ramp.error = colors.error;
  return ramp;
}

FluentThemeTokens Theme::tokens(const ThemeColors &colors)
{
  const bool darkMode = isDark(colors);
  FluentThemeTokens t;
  t.legacyColors = colors;
  t.accent = accentRamp(colors.accent, darkMode);
  t.neutral = neutralRamp(colors);
  t.semantic = semanticRamp(colors);
  t.elevation.shadow = darkMode ? QColor(0, 0, 0, 180) : QColor(0, 0, 0, 80);
  t.onAccent = contrastColor(colors.accent);
  t.dark = darkMode;
  return t;
}

QString Theme::baseStyleSheet(const ThemeColors &colors) {
  const auto themeTokens = tokens(colors);
  Q_UNUSED(colors);

  return QString(
             "QWidget {"
             "  font-family: 'Segoe UI', 'Microsoft YaHei UI', 'Microsoft "
             "YaHei', sans-serif;"
             "  font-size: %1px;"
             "}"
             "QLabel {"
             "  background: transparent;"
             "}"
             /* Fluent Design Scrollbar - Win11-like overlay (appears on hover) */
             "QScrollBar::handle {"
             "  background-color: transparent;"
             "  border: 1px solid transparent;"
             "  border-radius: 999px;"
             "}"
             "QScrollBar:vertical {"
             "  background: transparent;"
             "  width: 10px;"
             "  margin: 0px;"
             "}"
             "QScrollBar::handle:vertical {"
             "  background-color: transparent;"
             "  min-height: 24px;"
             "  margin: 2px;"
             "}"
             "QAbstractScrollArea:hover QScrollBar::handle:vertical {"
             "  background-color: rgba(128,128,128,0.45);"
             "}"
             "QScrollBar::handle:vertical:hover {"
             "  background-color: rgba(128,128,128,0.62);"
             "}"
             "QScrollBar::handle:vertical:pressed {"
             "  background-color: rgba(128,128,128,0.76);"
             "}"
             "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
             "  height: 0px;"
             "  border: none;"
             "  background: none;"
             "}"
             "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
             "  background: none;"
             "}"
             "QScrollBar:horizontal {"
             "  background: transparent;"
             "  height: 10px;"
             "  margin: 0px;"
             "}"
             "QScrollBar::handle:horizontal {"
             "  background-color: transparent;"
             "  min-width: 24px;"
             "  margin: 2px;"
             "}"
             "QAbstractScrollArea:hover QScrollBar::handle:horizontal {"
             "  background-color: rgba(128,128,128,0.45);"
             "}"
             "QScrollBar::handle:horizontal:hover {"
             "  background-color: rgba(128,128,128,0.62);"
             "}"
             "QScrollBar::handle:horizontal:pressed {"
             "  background-color: rgba(128,128,128,0.76);"
             "}"
             "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal "
             "{"
             "  width: 0px;"
             "  border: none;"
             "  background: none;"
             "}"
             "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal "
             "{"
             "  background: none;"
             "}"
             "QToolTip {"
             "  background: palette(tool-tip-base);"
             "  color: palette(tool-tip-text);"
             "  border: 1px solid rgba(128,128,128,0.55);"
             "  padding: 7px 10px;"
             "  border-radius: 8px;"
             "  font-size: 12px;"
             "  font-weight: 500;"
             "}")
      .arg(themeTokens.typography.body);
}

QString Theme::buttonStyle(const ThemeColors &colors, bool primary) {
  const auto themeTokens = tokens(colors);
  const QString background = primary ? themeTokens.accent.base.name(QColor::HexArgb)
                                     : colors.surface.name(QColor::HexArgb);
  const QString textColor = primary ? themeTokens.onAccent.name(QColor::HexArgb)
                                    : colors.text.name(QColor::HexArgb);
  const QString borderColor = primary ? Style::mix(themeTokens.accent.base, themeTokens.onAccent, 0.18).name(QColor::HexArgb)
                                      : colors.border.name(QColor::HexArgb);
  const QString hover = primary ? themeTokens.accent.light1.name(QColor::HexArgb)
                                : Style::mix(colors.surface, colors.hover, 0.88).name(QColor::HexArgb);
  const QString pressed = primary ? themeTokens.accent.dark1.name(QColor::HexArgb)
                                  : Style::mix(colors.surface, colors.pressed, 0.92).name(QColor::HexArgb);
  return QString("QPushButton {"
                 "  background: %1;"
                 "  color: %2;"
                 "  border: 1px solid %3;"
                 "  border-radius: 6px;"
                 "  padding: 6px 14px;"
                 "}"
                 "QPushButton:hover {"
                 "  background: %4;"
                 "}"
                 "QPushButton:pressed {"
                 "  background: %5;"
                 "}"
                 "QPushButton:disabled {"
                 "  background: %6;"
                 "  color: %7;"
                 "  border-color: %8;"
                 "}")
      .arg(background)
      .arg(textColor)
      .arg(borderColor)
      .arg(hover)
      .arg(pressed)
      .arg(Style::mix(colors.surface, colors.hover, 0.45).name(QColor::HexArgb))
      .arg(colors.disabledText.name(QColor::HexArgb))
      .arg(Style::mix(colors.border, colors.disabledText, 0.25).name(QColor::HexArgb));
}

QString Theme::labelStyle(const ThemeColors &colors) {
  Q_UNUSED(colors);
  return QString("QLabel {"
                 "  color: palette(window-text);"
                 "}");
}

QString Theme::lineEditStyle(const ThemeColors &colors) {
  Q_UNUSED(colors);
  return QString("QLineEdit {"
                 "  background: palette(base);"
                 "  color: palette(text);"
                 "  border: 1px solid palette(mid);"
                 "  border-radius: 6px;"
                 "  padding: 6px 10px;"
                 "}"
                 "QLineEdit:focus {"
                 "  border: 1px solid palette(highlight);"
                 "}"
                 "QLineEdit:disabled {"
                 "  color: palette(mid);"
                 "  background: palette(light);"
                 "}");
}

QString Theme::textEditStyle(const ThemeColors &colors) {
  Q_UNUSED(colors);
  return QString("QTextEdit {"
                 "  background: palette(base);"
                 "  color: palette(text);"
                 "  border: 1px solid palette(mid);"
                 "  border-radius: 6px;"
                 "  padding: 6px 10px;"
                 "}"
                 "QTextEdit:focus {"
                 "  border: 1px solid palette(highlight);"
                 "}");
}

QString Theme::dateTimeStyle(const ThemeColors &colors) {
  Q_UNUSED(colors);
  return QString("QDateEdit, QTimeEdit {"
                 "  background: palette(base);"
                 "  color: palette(text);"
                 "  border: 1px solid palette(mid);"
                 "  border-radius: 6px;"
                 "  padding: 6px 10px;"
                 "}"
                 "QDateEdit:hover, QTimeEdit:hover {"
                 "  border-color: palette(highlight);"
                 "}"
                 "QDateEdit:focus, QTimeEdit:focus {"
                 "  border: 2px solid palette(highlight);"
                 "}"
                 "QDateEdit::up-button, QTimeEdit::up-button,"
                 "QDateEdit::down-button, QTimeEdit::down-button {"
                 "  subcontrol-origin: border;"
                 "  width: 20px;"
                 "  border: none;"
                 "  background: transparent;"
                 "}"
                 "QDateEdit::up-button, QTimeEdit::up-button {"
                 "  subcontrol-position: top right;"
                 "}"
                 "QDateEdit::down-button, QTimeEdit::down-button {"
                 "  subcontrol-position: bottom right;"
                 "}"
                 "QDateEdit::up-button:hover, QTimeEdit::up-button:hover,"
                 "QDateEdit::down-button:hover, QTimeEdit::down-button:hover {"
                 "  background: palette(light);"
                 "  border-radius: 3px;"
                 "}"
                 "QCalendarWidget QWidget {"
                 "  background: palette(base);"
                 "  color: palette(text);"
                 "}");
}

QString Theme::calendarPopupStyle(const ThemeColors &colors) {
  Q_UNUSED(colors);
  return QString("QCalendarWidget {"
                 "  background: palette(base);"
                 "  border: 1px solid palette(mid);"
                 "  border-radius: 8px;"
                 "}"
                 "QCalendarWidget QWidget {"
                 "  background: palette(base);"
                 "  color: palette(text);"
                 "}"
                 "QCalendarWidget QAbstractItemView:enabled {"
                 "  background: palette(base);"
                 "  selection-background-color: palette(highlight);"
                 "  selection-color: palette(highlighted-text);"
                 "  border: none;"
                 "  outline: none;"
                 "}"
                 "QCalendarWidget QAbstractItemView::item {"
                 "  padding: 6px;"
                 "  border-radius: 4px;"
                 "}"
                 "QCalendarWidget QAbstractItemView::item:hover {"
                 "  background: palette(light);"
                 "}"
                 "QCalendarWidget QAbstractItemView::item:selected {"
                 "  background: palette(highlight);"
                 "  color: palette(highlighted-text);"
                 "  font-weight: 600;"
                 "}"
                 "QCalendarWidget QHeaderView::section {"
                 "  background: palette(base);"
                 "  color: palette(mid);"
                 "  padding: 8px;"
                 "  border: none;"
                 "  font-weight: 600;"
                 "}"
                 "QCalendarWidget QToolButton {"
                 "  background: transparent;"
                 "  border: none;"
                 "  border-radius: 4px;"
                 "  padding: 6px;"
                 "  color: palette(text);"
                 "  icon-size: 16px;"
                 "}"
                 "QCalendarWidget QToolButton:hover {"
                 "  background: palette(light);"
                 "}"
                 "QCalendarWidget QToolButton:pressed {"
                 "  background: palette(dark);"
                 "}"
                 "QCalendarWidget QToolButton#qt_calendar_prevmonth,"
                 "QCalendarWidget QToolButton#qt_calendar_nextmonth {"
                 "  min-width: 32px;"
                 "  min-height: 32px;"
                 "  border-radius: 16px;"
                 "}"
                 "QCalendarWidget QToolButton#qt_calendar_monthbutton,"
                 "QCalendarWidget QToolButton#qt_calendar_yearbutton {"
                 "  padding: 4px 12px;"
                 "  font-weight: 600;"
                 "}"
                 "QCalendarWidget QToolButton::menu-indicator {"
                 "  image: none;"
                 "  width: 0px;"
                 "}"
                 "QCalendarWidget QSpinBox {"
                 "  border: 1px solid palette(mid);"
                 "  border-radius: 4px;"
                 "  padding: 4px 8px;"
                 "  background: palette(base);"
                 "  selection-background-color: palette(highlight);"
                 "}"
                 "QCalendarWidget QSpinBox::up-button,"
                 "QCalendarWidget QSpinBox::down-button {"
                 "  width: 16px;"
                 "  border: none;"
                 "  background: transparent;"
                 "  subcontrol-origin: padding;"
                 "  border-radius: 4px;"
                 "}"
                 "QCalendarWidget QSpinBox::up-button:hover,"
                 "QCalendarWidget QSpinBox::down-button:hover {"
                 "  background: palette(light);"
                 "}"
                 "QCalendarWidget QAbstractItemView::item:focus {"
                 "  border: 2px solid palette(highlight);"
                 "  background: transparent;"
                 "}");
}

QString Theme::checkBoxStyle(const ThemeColors &colors) {
  Q_UNUSED(colors);
  return QString("QCheckBox {"
                 "  spacing: 8px;"
                 "  color: palette(window-text);"
                 "}"
                 "QCheckBox::indicator {"
                 "  width: 18px;"
                 "  height: 18px;"
                 "  border-radius: 4px;"
                 "  border: 1px solid palette(mid);"
                 "  background: palette(base);"
                 "}"
                 "QCheckBox::indicator:checked {"
                 "  background: palette(highlight);"
                 "  border-color: palette(highlight);"
                 "  image: "
                 "url(:/qt-project.org/styles/commonstyle/images/"
                 "checkbox_checked.png);"
                 "}"
                 "QCheckBox::indicator:disabled {"
                 "  background: palette(light);"
                 "  border-color: palette(mid);"
                 "}");
}

QString Theme::radioButtonStyle(const ThemeColors &colors) {
  Q_UNUSED(colors);
  return QString("QRadioButton {"
                 "  spacing: 8px;"
                 "  color: palette(window-text);"
                 "}"
                 "QRadioButton::indicator {"
                 "  width: 18px;"
                 "  height: 18px;"
                 "  border-radius: 9px;"
                 "  border: 1px solid palette(mid);"
                 "  background: palette(base);"
                 "}"
                 "QRadioButton::indicator:checked {"
                 "  background: palette(highlight);"
                 "  border-color: palette(highlight);"
                 "}");
}

QString Theme::toggleSwitchStyle(const ThemeColors &colors) {
  Q_UNUSED(colors);
  return QString("QCheckBox {"
                 "  spacing: 10px;"
                 "  color: palette(window-text);"
                 "}"
                 "QCheckBox::indicator {"
                 "  width: 36px;"
                 "  height: 20px;"
                 "  border-radius: 10px;"
                 "  background: palette(mid);"
                 "  border: 1px solid palette(mid);"
                 "}"
                 "QCheckBox::indicator:checked {"
                 "  background: palette(highlight);"
                 "  border-color: palette(highlight);"
                 "}"
                 "QCheckBox::indicator:unchecked {"
                 "  background: palette(mid);"
                 "}"
                 "QCheckBox::indicator:checked:pressed {"
                 "  background: palette(shadow);"
                 "}"
                 "QCheckBox::indicator:checked {"
                 "  image: none;"
                 "}"
                 "QCheckBox::indicator:unchecked {"
                 "  image: none;"
                 "}"
                 "QCheckBox::indicator::before {"
                 "  content: '';"
                 "}");
}

QString Theme::comboBoxStyle(const ThemeColors &colors) {
  return QString("QComboBox {"
                 "  background: %1;"
                 "  color: %2;"
                 "  border: 1px solid %3;"
                 "  border-radius: 6px;"
                 "  padding: 6px 30px 6px 10px;"
                 "}"
                 "QComboBox:hover {"
                 "  border-color: %4;"
                 "}"
                 "QComboBox::drop-down {"
                 "  subcontrol-origin: padding;"
                 "  subcontrol-position: center right;"
                 "  width: 24px;"
                 "  border: none;"
                 "  border-top-right-radius: 6px;"
                 "  border-bottom-right-radius: 6px;"
                 "}"
                 "QComboBox QAbstractItemView {"
                 "  background: %1;"
                 "  color: %2;"
                 "  border: 1px solid %3;"
                 "  border-radius: 6px;"
                 "  selection-background-color: %5;"
                 "  outline: none;"
                 "  padding: 4px;"
                 "}"
                 "QComboBox QAbstractItemView::item {"
                 "  padding: 6px 10px;"
                 "  border-radius: 4px;"
                 "}"
                 "QComboBox QAbstractItemView::item:hover {"
                 "  background: %5;"
                 "}")
      .arg(colors.surface.name(QColor::HexArgb),
           colors.text.name(QColor::HexArgb),
           colors.border.name(QColor::HexArgb),
           colors.accent.name(QColor::HexArgb),
           colors.hover.name(QColor::HexArgb));
}

QString Theme::sliderStyle(const ThemeColors &colors) {
  Q_UNUSED(colors);
  return QString("QSlider::groove:horizontal {"
                 "  height: 6px;"
                 "  background: palette(mid);"
                 "  border-radius: 3px;"
                 "}"
                 "QSlider::handle:horizontal {"
                 "  width: 16px;"
                 "  height: 16px;"
                 "  margin: -6px 0;"
                 "  border-radius: 8px;"
                 "  background: palette(highlight);"
                 "}");
}

QString Theme::progressBarStyle(const ThemeColors &colors) {
  Q_UNUSED(colors);
  return QString("QProgressBar {"
                 "  border: 1px solid palette(mid);"
                 "  border-radius: 6px;"
                 "  text-align: center;"
                 "  background: palette(base);"
                 "}"
                 "QProgressBar::chunk {"
                 "  background: palette(highlight);"
                 "  border-radius: 6px;"
                 "}");
}

QString Theme::spinBoxStyle(const ThemeColors &colors) {
  Q_UNUSED(colors);
  return QString("QSpinBox, QDoubleSpinBox {"
                 "  background: palette(base);"
                 "  color: palette(text);"
                 "  border: 1px solid palette(mid);"
                 "  border-radius: 6px;"
                 "  padding: 6px 10px;"
                 "}"
                 "QSpinBox:hover, QDoubleSpinBox:hover {"
                 "  border-color: palette(highlight);"
                 "}");
}

QString Theme::toolButtonStyle(const ThemeColors &colors) {
  Q_UNUSED(colors);
  return QString("QToolButton {"
                 "  background: palette(button);"
                 "  color: palette(button-text);"
                 "  border: 1px solid palette(mid);"
                 "  border-radius: 6px;"
                 "  padding: 6px 12px;"
                 "}"
                 "QToolButton:hover {"
                 "  background: palette(light);"
                 "}");
}

QString Theme::tabWidgetStyle(const ThemeColors &colors) {
  Q_UNUSED(colors);
  return QString("QTabWidget::pane {"
                 "  border: none;"
                 "  background: transparent;"
                 "  padding: 0px;"
                 "  top: 0px;"
                 "}"
                 "QTabBar {"
                 "  background: transparent;"
                 "  border: none;"
                 "}"
                 "QTabBar QToolButton {"
                 "  background: transparent;"
                 "  border: none;"
                 "  border-radius: 8px;"
                 "  padding: 4px;"
                 "  margin: 6px 6px;"
                 "}"
                 "QTabBar QToolButton:hover {"
                 "  background: palette(light);"
                 "}"
                 "QTabBar QToolButton:pressed {"
                 "  background: palette(dark);"
                 "}");
}

QString Theme::listViewStyle(const ThemeColors &colors) {
  const auto themeTokens = tokens(colors);
  return QString("QListView {"
                 "  background: %1;"
                 "  color: %2;"
                 "  border: 1px solid %3;"
                 "  border-radius: 8px;"
                 "  outline: none;"
                 "}"
                 "QListView:focus {"
                 "  border-color: %4;"
                 "}"
                 "QListView:disabled {"
                 "  background: %5;"
                 "  color: %6;"
                 "  border-color: %7;"
                 "}"
                 "QListView::item {"
                 "  padding: 6px 10px;"
                 "  border-radius: 4px;"
                 "  background: transparent;"
                 "}"
                 "QListView::item:hover {"
                 "  background: transparent;"
                 "}"
                 "QListView::item:selected {"
                 "  background: transparent;"
                 "}")
      .arg(colors.surface.name(QColor::HexArgb),
           colors.text.name(QColor::HexArgb),
           themeTokens.neutral.strokeSubtle.name(QColor::HexArgb),
           colors.accent.name(QColor::HexArgb),
           Style::mix(themeTokens.neutral.card, themeTokens.neutral.fillSecondary, themeTokens.dark ? 0.30 : 0.46).name(QColor::HexArgb),
           colors.disabledText.name(QColor::HexArgb),
           Style::mix(themeTokens.neutral.strokeSubtle, colors.disabledText, themeTokens.dark ? 0.18 : 0.12).name(QColor::HexArgb));
}

QString Theme::tableViewStyle(const ThemeColors &colors) {
  const auto themeTokens = tokens(colors);
  return QString("QTableView, QTableWidget {"
                 "  background: %1;"
                 "  color: %2;"
                 "  border: 1px solid %3;"
                 "  gridline-color: %3;"
                 "  border-radius: 8px;"
                 "  outline: none;"
                 "}"
                 "QTableView:focus, QTableWidget:focus {"
                 "  border-color: %5;"
                 "}"
                 "QTableView:disabled, QTableWidget:disabled {"
                 "  background: %6;"
                 "  color: %7;"
                 "  border-color: %8;"
                 "  gridline-color: %8;"
                 "}"
                 "QTableView::item, QTableWidget::item {"
                 "  padding: 4px 8px;"
                 "  background: transparent;"
                 "}"
                 "QTableView::item:hover, QTableWidget::item:hover {"
                 "  background: transparent;"
                 "}"
                 "QHeaderView::section {"
                 "  background: %4;"
                 "  color: %2;"
                 "  border: none;"
                 "  border-bottom: 1px solid %3;"
                 "  padding: 6px 8px;"
                 "  font-weight: 600;"
                 "}"
                 "QHeaderView::section:disabled {"
                 "  color: %7;"
                 "  background: %6;"
                 "  border-bottom-color: %8;"
                 "}"
                 "QTableView::item:selected, QTableWidget::item:selected {"
                 "  background: transparent;"
                 "}")
      .arg(colors.surface.name(QColor::HexArgb),
           colors.text.name(QColor::HexArgb),
           themeTokens.neutral.strokeSubtle.name(QColor::HexArgb),
           themeTokens.neutral.layer.name(QColor::HexArgb),
           colors.accent.name(QColor::HexArgb),
           Style::mix(themeTokens.neutral.card, themeTokens.neutral.fillSecondary, themeTokens.dark ? 0.30 : 0.46).name(QColor::HexArgb),
           colors.disabledText.name(QColor::HexArgb),
           Style::mix(themeTokens.neutral.strokeSubtle, colors.disabledText, themeTokens.dark ? 0.18 : 0.12).name(QColor::HexArgb));
}

QString Theme::treeViewStyle(const ThemeColors &colors) {
  const auto themeTokens = tokens(colors);
  return QString("QTreeView {"
                 "  background: %1;"
                 "  color: %2;"
                 "  border: 1px solid %3;"
                 "  border-radius: 8px;"
                 "  outline: none;"
                 "}"
                 "QTreeView:focus {"
                 "  border-color: %5;"
                 "}"
                 "QTreeView:disabled {"
                 "  background: %6;"
                 "  color: %7;"
                 "  border-color: %8;"
                 "}"
                 "QTreeView::item {"
                 "  padding: 6px 8px;"
                 "  border-radius: 4px;"
                 "  background: transparent;"
                 "}"
                 "QTreeView::item:hover {"
                 "  background: transparent;"
                 "}"
                 "QTreeView::item:selected {"
                 "  background: transparent;"
                 "}"
                 "QTreeView::branch {"
                 "  background: transparent;"
                 "}"
                 "QTreeView::branch:closed:has-children {"
                 "  border-image: none;"
                 "  image: none;"
                 "}"
                 "QTreeView::branch:open:has-children {"
                 "  border-image: none;"
                 "  image: none;"
                 "}"
                 "QHeaderView::section {"
                 "  background: %4;"
                 "  color: %2;"
                 "  border: none;"
                 "  border-bottom: 1px solid %3;"
                 "  padding: 6px 8px;"
                 "  font-weight: 600;"
                 "}")
      .arg(colors.surface.name(QColor::HexArgb),
           colors.text.name(QColor::HexArgb),
           themeTokens.neutral.strokeSubtle.name(QColor::HexArgb),
           themeTokens.neutral.layer.name(QColor::HexArgb),
           colors.accent.name(QColor::HexArgb),
           Style::mix(themeTokens.neutral.card, themeTokens.neutral.fillSecondary, themeTokens.dark ? 0.30 : 0.46).name(QColor::HexArgb),
           colors.disabledText.name(QColor::HexArgb),
           Style::mix(themeTokens.neutral.strokeSubtle, colors.disabledText, themeTokens.dark ? 0.18 : 0.12).name(QColor::HexArgb));
}

QString Theme::groupBoxStyle(const ThemeColors &colors) {
  Q_UNUSED(colors);
  return QString("QGroupBox {"
                 "  background: palette(base);"
                 "  border: 1px solid palette(mid);"
                 "  border-radius: 10px;"
                 "  margin-top: 14px;"
                 "  padding: 10px;"
                 "  padding-top: 14px;"
                 "}"
                 "QGroupBox:disabled {"
                 "  background: palette(light);"
                 "  color: palette(mid);"
                 "}"
                 "QGroupBox::title {"
                 "  subcontrol-origin: margin;"
                 "  left: 12px;"
                 "  padding: 0 6px;"
                 "  color: palette(text);"
                 "  background: palette(base);"
                 "}");
}

QString Theme::menuBarStyle(const ThemeColors &colors) {
  Q_UNUSED(colors);
  return QString("QMenuBar {"
                 "  background: palette(button);"
                 "  color: palette(button-text);"
                 "  border-bottom: 1px solid palette(mid);"
                 "}"
                 "QMenuBar::item {"
                 "  padding: 6px 12px;"
                 "  background: transparent;"
                 "}"
                 "QMenuBar::item:selected {"
                 "  background: palette(light);"
                 "  border-radius: 4px;"
                 "}");
}

QString Theme::toolBarStyle(const ThemeColors &colors) {
  Q_UNUSED(colors);
  return QString("QToolBar {"
                 "  background: palette(button);"
                 "  border-bottom: 1px solid palette(mid);"
                 "  spacing: 6px;"
                 "}"
                 "QToolBar::separator {"
                 "  background: palette(mid);"
                 "  width: 1px;"
                 "  margin: 4px 6px;"
                 "}"
                 "QToolButton {"
                 "  background: palette(button);"
                 "  color: palette(button-text);"
                 "  border: 1px solid palette(mid);"
                 "  border-radius: 6px;"
                 "  padding: 6px 10px;"
                 "}"
                 "QToolButton:hover {"
                 "  background: palette(light);"
                 "}");
}

QString Theme::statusBarStyle(const ThemeColors &colors) {
  Q_UNUSED(colors);
  return QString("QStatusBar {"
                 "  background: palette(button);"
                 "  color: palette(button-text);"
                 "  border-top: 1px solid palette(mid);"
                 "  min-height: 28px;"
                 "}"
                 "QStatusBar::item {"
                 "  border: none;"
                 "}"
                 "QStatusBar QLabel {"
                 "  color: palette(button-text);"
                 "  padding: 2px 8px;"
                 "}");
}

QString Theme::dialogStyle(const ThemeColors &colors) {
  Q_UNUSED(colors);
  return QString("QDialog {"
                 "  background: palette(base);"
                 "  color: palette(text);"
                 "  border: 1px solid palette(mid);"
                 "  border-radius: 10px;"
                 "}"
                 "QDialog QWidget {"
                 "  background: transparent;"
                 "}"
                 "QDialog QLabel {"
                 "  color: palette(text);"
                 "}");
}

QString Theme::cardStyle(const ThemeColors &colors) {
  Q_UNUSED(colors);
  return QString("QWidget#FluentCard {"
                 "  background: palette(base);"
                 "  border: 1px solid palette(mid);"
                 "  border-radius: 10px;"
                 "}"
                 "QWidget#FluentCard:disabled {"
                 "  background: palette(light);"
                 "  border-color: palette(mid);"
                 "}");
}

ThemeManager &ThemeManager::instance() {
  static ThemeManager instance;
  return instance;
}

ThemeManager::ThemeManager()
    : m_colors(Theme::light())
    , m_tokens(Theme::tokens(m_colors))
    , m_motionTokens(m_tokens.motion)
    , m_baseAccent(m_colors.accent)
{
  if (Diagnostics::knownQtWarningSuppressionEnabled() || !Diagnostics::qtWarningOutputEnabled()) {
    Diagnostics::installMessageHandler();
  }
  if (qApp) {
    QTimer::singleShot(0, qApp, []() { FluentToolTip::ensureInstalled(); });
  }
}

const ThemeColors &ThemeManager::colors() const { return m_colors; }

const FluentThemeTokens &ThemeManager::tokens() const { return m_tokens; }

bool ThemeManager::accentBorderEnabled() const { return m_accentBorderEnabled; }

bool ThemeManager::animationsEnabled() const { return m_animationsEnabled; }

const FluentMotionTokens &ThemeManager::motionTokens() const { return m_motionTokens; }

namespace {

QString themeModeName(ThemeManager::ThemeMode mode)
{
  return mode == ThemeManager::ThemeMode::Dark ? QStringLiteral("Dark") : QStringLiteral("Light");
}

struct UpdatesBlocker {
  QVector<QWidget *> widgets;

  UpdatesBlocker()
  {
    const auto topLevels = QApplication::topLevelWidgets();
    widgets.reserve(topLevels.size());
    for (QWidget *w : topLevels) {
      if (!w || !w->isVisible() || !w->updatesEnabled()) {
        continue;
      }
      widgets.push_back(w);
      w->setUpdatesEnabled(false);
    }
  }

  ~UpdatesBlocker()
  {
    for (QWidget *w : std::as_const(widgets)) {
      if (!w) {
        continue;
      }
      w->setUpdatesEnabled(true);
      w->update();
    }
  }
};

bool motionTokensEqual(const FluentMotionTokens &a, const FluentMotionTokens &b)
{
  return a.hoverDuration == b.hoverDuration &&
         a.pressDuration == b.pressDuration &&
         a.focusDuration == b.focusDuration &&
         a.popupOpenDuration == b.popupOpenDuration &&
         a.popupCloseDuration == b.popupCloseDuration &&
         a.collapseDuration == b.collapseDuration &&
         a.selectionDuration == b.selectionDuration &&
         a.navigationDuration == b.navigationDuration &&
         a.layoutDuration == b.layoutDuration &&
         a.pageDuration == b.pageDuration &&
         a.toastDuration == b.toastDuration &&
         a.wheelSnapDuration == b.wheelSnapDuration &&
         a.popupSlideOffset == b.popupSlideOffset &&
         a.pressOffset == b.pressOffset &&
         a.easeOut == b.easeOut &&
         a.easeIn == b.easeIn &&
         a.easeInOut == b.easeInOut;
}

FluentMotionTokens normalizedMotionTokens(FluentMotionTokens tokens)
{
  tokens.hoverDuration = qMax(0, tokens.hoverDuration);
  tokens.pressDuration = qMax(0, tokens.pressDuration);
  tokens.focusDuration = qMax(0, tokens.focusDuration);
  tokens.popupOpenDuration = qMax(0, tokens.popupOpenDuration);
  tokens.popupCloseDuration = qMax(0, tokens.popupCloseDuration);
  tokens.collapseDuration = qMax(0, tokens.collapseDuration);
  tokens.selectionDuration = qMax(0, tokens.selectionDuration);
  tokens.navigationDuration = qMax(0, tokens.navigationDuration);
  tokens.layoutDuration = qMax(0, tokens.layoutDuration);
  tokens.pageDuration = qMax(0, tokens.pageDuration);
  tokens.toastDuration = qMax(0, tokens.toastDuration);
  tokens.wheelSnapDuration = qMax(0, tokens.wheelSnapDuration);
  tokens.popupSlideOffset = qMax(0, tokens.popupSlideOffset);
  tokens.pressOffset = qMax(0, tokens.pressOffset);
  return tokens;
}

} // namespace

void ThemeManager::scheduleThemeChanged(const QString &reason)
{
  const QString nextReason = reason.isEmpty() ? QStringLiteral("themeChanged") : reason;
  if (m_themeChangedPending) {
    if (!m_themeChangeReason.contains(nextReason)) {
      m_themeChangeReason += QStringLiteral(", ");
      m_themeChangeReason += nextReason;
    }
    qInfo().noquote() << QStringLiteral("[ThemeSwitch] #%1 coalesced %2 after %3 ms")
                             .arg(m_themeChangeSequence)
                             .arg(nextReason)
                             .arg(m_themeChangeTimer.isValid() ? m_themeChangeTimer.elapsed() : 0);
    return;
  }

  m_themeChangedPending = true;
  m_themeChangeReason = nextReason;
  m_themeChangeTimer.restart();
  const int sequence = ++m_themeChangeSequence;

  qInfo().noquote() << QStringLiteral("[ThemeSwitch] #%1 scheduled: %2, mode=%3, accent=%4")
                           .arg(sequence)
                           .arg(m_themeChangeReason)
                           .arg(themeModeName(m_mode))
                           .arg(m_colors.accent.name());

  // Coalesce multiple changes and avoid blocking the current UI event.
  QTimer::singleShot(0, this, [this, sequence]() {
    const QString reason = m_themeChangeReason;
    qInfo().noquote() << QStringLiteral("[ThemeSwitch] #%1 dispatch begin after %2 ms: %3")
                             .arg(sequence)
                             .arg(m_themeChangeTimer.isValid() ? m_themeChangeTimer.elapsed() : 0)
                             .arg(reason);

    QElapsedTimer dispatchTimer;
    dispatchTimer.start();
    m_themeChangedPending = false;
    {
      QElapsedTimer blockerTimer;
      blockerTimer.start();
      UpdatesBlocker updatesBlocker;
      qInfo().noquote() << QStringLiteral("[ThemeSwitch] #%1 updates blocked for %2 top-level widgets +%3 ms")
                               .arg(sequence)
                               .arg(updatesBlocker.widgets.size())
                               .arg(blockerTimer.elapsed());
      emit themeChanged();
    }
    qInfo().noquote() << QStringLiteral("[ThemeSwitch] #%1 dispatch done +%2 ms, total %3 ms")
                             .arg(sequence)
                             .arg(dispatchTimer.elapsed())
                             .arg(m_themeChangeTimer.isValid() ? m_themeChangeTimer.elapsed() : dispatchTimer.elapsed());
  });
}

void ThemeManager::setAccentBorderEnabled(bool enabled)
{
  if (m_accentBorderEnabled == enabled) {
    return;
  }
  m_accentBorderEnabled = enabled;
  scheduleThemeChanged(QStringLiteral("setAccentBorderEnabled"));
}

void ThemeManager::setAnimationsEnabled(bool enabled)
{
  if (m_animationsEnabled == enabled) {
    return;
  }
  m_animationsEnabled = enabled;
  scheduleThemeChanged(QStringLiteral("setAnimationsEnabled"));
}

void ThemeManager::setMotionTokens(const FluentMotionTokens &tokens)
{
  const FluentMotionTokens next = normalizedMotionTokens(tokens);
  if (motionTokensEqual(m_motionTokens, next)) {
    return;
  }
  m_motionTokens = next;
  m_tokens.motion = m_motionTokens;
  scheduleThemeChanged(QStringLiteral("setMotionTokens"));
}

void ThemeManager::resetMotionTokens()
{
  setMotionTokens(FluentMotionTokens{});
}

void ThemeManager::setColors(const ThemeColors &colors) {
  setColorsInternal(colors, true, QStringLiteral("setColors"));
}

void ThemeManager::setAccentColor(const QColor &accent)
{
  if (!accent.isValid()) {
    return;
  }

  m_baseAccent = accent;
  ThemeColors colors = m_colors;
  const bool dark = m_mode == ThemeMode::Dark;
  colors.accent = Theme::accentForMode(accent, dark);
  colors.focus = colors.accent.lighter(135);
  setColorsInternal(colors, false, QStringLiteral("setAccentColor"));
}

void ThemeManager::setColorsInternal(const ThemeColors &colors, bool updateBaseAccent, const QString &reason) {
  if (m_colors.accent == colors.accent &&
      m_colors.text == colors.text &&
      m_colors.subText == colors.subText &&
      m_colors.disabledText == colors.disabledText &&
      m_colors.background == colors.background &&
      m_colors.surface == colors.surface &&
      m_colors.border == colors.border &&
      m_colors.hover == colors.hover &&
      m_colors.pressed == colors.pressed &&
      m_colors.focus == colors.focus &&
      m_colors.error == colors.error) {
    return;
  }

  if (updateBaseAccent && m_colors.accent != colors.accent) {
    m_baseAccent = colors.accent;
  }

  m_colors = colors;
  m_tokens = Theme::tokens(m_colors);
  m_tokens.motion = m_motionTokens;
  scheduleThemeChanged(reason.isEmpty() ? QStringLiteral("setColorsInternal") : reason);
}

ThemeManager::ThemeMode ThemeManager::themeMode() const { return m_mode; }

void ThemeManager::setThemeMode(ThemeMode mode) {
  if (m_mode == mode) {
    return;
  }

  m_mode = mode;
  ThemeColors next = (mode == ThemeMode::Dark) ? Theme::dark() : Theme::light();
  next.accent = Theme::accentForMode(m_baseAccent, mode == ThemeMode::Dark);
  next.focus = next.accent.lighter(135);
  setColorsInternal(next,
                    false,
                    QStringLiteral("setThemeMode(%1)").arg(themeModeName(mode)));
}

void ThemeManager::setLightMode() { setThemeMode(ThemeMode::Light); }

void ThemeManager::setDarkMode() { setThemeMode(ThemeMode::Dark); }

} // namespace Fluent
