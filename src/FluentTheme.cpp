#include "Fluent/FluentTheme.h"

#include <QApplication>
#include <QTimer>

namespace Fluent {

ThemeColors Theme::light() {
  ThemeColors colors;
  // Islands Style - High Contrast Fluent Design (Light Mode)
  colors.accent = QColor("#0067C0");       // Vivid Blue - more saturated
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
  colors.accent = QColor("#60CDFF");  // Light blue - better visibility on dark
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

QString Theme::baseStyleSheet(const ThemeColors &colors) {
  const bool dark = colors.background.lightnessF() < 0.5;
  const QColor sbHandle =
      dark ? QColor(255, 255, 255, 70) : QColor(0, 0, 0, 70);
  const QColor sbHandleHover =
      dark ? QColor(255, 255, 255, 110) : QColor(0, 0, 0, 110);
  const QColor sbHandlePressed =
      dark ? QColor(255, 255, 255, 140) : QColor(0, 0, 0, 140);

  return QString(
             "QWidget {"
             "  color: %1;"
             "  font-family: 'Segoe UI', 'Microsoft YaHei UI', 'Microsoft "
             "YaHei', sans-serif;"
             "  font-size: 14px;"
             "}"
             "QWidget:window, QMainWindow, QDialog {"
             "  background: %2;"
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
             "  background-color: %6;"
             "}"
             "QScrollBar::handle:vertical:hover {"
             "  background-color: %7;"
             "}"
             "QScrollBar::handle:vertical:pressed {"
             "  background-color: %8;"
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
             "  background-color: %6;"
             "}"
             "QScrollBar::handle:horizontal:hover {"
             "  background-color: %7;"
             "}"
             "QScrollBar::handle:horizontal:pressed {"
             "  background-color: %8;"
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
             "  background: %4;"
             "  color: %1;"
             "  border: 1px solid %3;"
             "  padding: 8px 10px;"
             "  border-radius: 8px;"
             "  font-size: 12px;"
             "}"
             "QLabel#FluentLink {"
             "  color: %5;"
             "}")
      .arg(colors.text.name(), colors.background.name(), colors.border.name(),
           colors.surface.name(), colors.accent.name())
      .arg(sbHandle.name(QColor::HexArgb))
      .arg(sbHandleHover.name(QColor::HexArgb))
      .arg(sbHandlePressed.name(QColor::HexArgb));
}

QString Theme::buttonStyle(const ThemeColors &colors, bool primary) {
  const QString borderColor =
      primary ? colors.accent.name() : colors.border.name();
  const QString background =
      primary ? colors.accent.name() : colors.surface.name();
  const QString textColor = primary ? QString("#FFFFFF") : colors.text.name();

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
      .arg(primary ? colors.accent.lighter(110).name() : colors.hover.name())
      .arg(primary ? colors.accent.darker(110).name() : colors.pressed.name())
      .arg(colors.hover.name())
      .arg(colors.disabledText.name())
      .arg(colors.border.name());
}

QString Theme::labelStyle(const ThemeColors &colors) {
  return QString("QLabel {"
                 "  color: %1;"
                 "}")
      .arg(colors.text.name());
}

QString Theme::lineEditStyle(const ThemeColors &colors) {
  return QString("QLineEdit {"
                 "  background: %1;"
                 "  color: %2;"
                 "  border: 1px solid %3;"
                 "  border-radius: 6px;"
                 "  padding: 6px 10px;"
                 "}"
                 "QLineEdit:focus {"
                 "  border: 1px solid %4;"
                 "}"
                 "QLineEdit:disabled {"
                 "  color: %5;"
                 "  background: %6;"
                 "}")
      .arg(colors.surface.name())
      .arg(colors.text.name())
      .arg(colors.border.name())
      .arg(colors.focus.name())
      .arg(colors.disabledText.name())
      .arg(colors.hover.name());
}

QString Theme::textEditStyle(const ThemeColors &colors) {
  return QString("QTextEdit {"
                 "  background: %1;"
                 "  color: %2;"
                 "  border: 1px solid %3;"
                 "  border-radius: 6px;"
                 "  padding: 6px 10px;"
                 "}"
                 "QTextEdit:focus {"
                 "  border: 1px solid %4;"
                 "}")
      .arg(colors.surface.name())
      .arg(colors.text.name())
      .arg(colors.border.name())
      .arg(colors.focus.name());
}

QString Theme::dateTimeStyle(const ThemeColors &colors) {
  return QString("QDateEdit, QTimeEdit {"
                 "  background: %1;"
                 "  color: %2;"
                 "  border: 1px solid %3;"
                 "  border-radius: 6px;"
                 "  padding: 6px 10px;"
                 "}"
                 "QDateEdit:hover, QTimeEdit:hover {"
                 "  border-color: %4;"
                 "}"
                 "QDateEdit:focus, QTimeEdit:focus {"
                 "  border: 2px solid %4;"
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
                 "  background: %5;"
                 "  border-radius: 3px;"
                 "}"
                 "QCalendarWidget QWidget {"
                 "  background: %1;"
                 "  color: %2;"
                 "}")
      .arg(colors.surface.name())
      .arg(colors.text.name())
      .arg(colors.border.name())
      .arg(colors.focus.name())
      .arg(colors.hover.name());
}

QString Theme::calendarPopupStyle(const ThemeColors &colors) {
  return QString("QCalendarWidget {"
                 "  background: %1;"
                 "  border: 1px solid %4;"
                 "  border-radius: 8px;"
                 "}"
                 "QCalendarWidget QWidget {"
                 "  background: %1;"
                 "  color: %2;"
                 "}"
                 "QCalendarWidget QAbstractItemView:enabled {"
                 "  background: %1;"
                 "  selection-background-color: %3;"
                 "  selection-color: white;"
                 "  border: none;"
                 "  outline: none;"
                 "}"
                 "QCalendarWidget QAbstractItemView::item {"
                 "  padding: 6px;"
                 "  border-radius: 4px;"
                 "}"
                 "QCalendarWidget QAbstractItemView::item:hover {"
                 "  background: %5;"
                 "}"
                 "QCalendarWidget QAbstractItemView::item:selected {"
                 "  background: %3;"
                 "  color: white;"
                 "  font-weight: 600;"
                 "}"
                 "QCalendarWidget QHeaderView::section {"
                 "  background: %1;"
                 "  color: %7;"
                 "  padding: 8px;"
                 "  border: none;"
                 "  font-weight: 600;"
                 "}"
                 "QCalendarWidget QToolButton {"
                 "  background: transparent;"
                 "  border: none;"
                 "  border-radius: 4px;"
                 "  padding: 6px;"
                 "  color: %2;"
                 "  icon-size: 16px;"
                 "}"
                 "QCalendarWidget QToolButton:hover {"
                 "  background: %5;"
                 "}"
                 "QCalendarWidget QToolButton:pressed {"
                 "  background: %6;"
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
                 "  border: 1px solid %4;"
                 "  border-radius: 4px;"
                 "  padding: 4px 8px;"
                 "  background: %1;"
                 "  selection-background-color: %3;"
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
                 "  background: %5;"
                 "}"
                 "QCalendarWidget QAbstractItemView::item:focus {"
                 "  border: 2px solid %3;"
                 "  background: transparent;"
                 "}")
      .arg(colors.surface.name())
      .arg(colors.text.name())
      .arg(colors.accent.name())
      .arg(colors.border.name())
      .arg(colors.hover.name())
      .arg(colors.pressed.name())
      .arg(colors.subText.name());
}

QString Theme::checkBoxStyle(const ThemeColors &colors) {
  return QString("QCheckBox {"
                 "  spacing: 8px;"
                 "  color: %1;"
                 "}"
                 "QCheckBox::indicator {"
                 "  width: 18px;"
                 "  height: 18px;"
                 "  border-radius: 4px;"
                 "  border: 1px solid %2;"
                 "  background: %3;"
                 "}"
                 "QCheckBox::indicator:checked {"
                 "  background: %4;"
                 "  border-color: %4;"
                 "  image: "
                 "url(:/qt-project.org/styles/commonstyle/images/"
                 "checkbox_checked.png);"
                 "}"
                 "QCheckBox::indicator:disabled {"
                 "  background: %5;"
                 "  border-color: %2;"
                 "}")
      .arg(colors.text.name())
      .arg(colors.border.name())
      .arg(colors.surface.name())
      .arg(colors.accent.name())
      .arg(colors.hover.name());
}

QString Theme::radioButtonStyle(const ThemeColors &colors) {
  return QString("QRadioButton {"
                 "  spacing: 8px;"
                 "  color: %1;"
                 "}"
                 "QRadioButton::indicator {"
                 "  width: 18px;"
                 "  height: 18px;"
                 "  border-radius: 9px;"
                 "  border: 1px solid %2;"
                 "  background: %3;"
                 "}"
                 "QRadioButton::indicator:checked {"
                 "  background: %4;"
                 "  border-color: %4;"
                 "}")
      .arg(colors.text.name())
      .arg(colors.border.name())
      .arg(colors.surface.name())
      .arg(colors.accent.name());
}

QString Theme::toggleSwitchStyle(const ThemeColors &colors) {
  return QString("QCheckBox {"
                 "  spacing: 10px;"
                 "  color: %1;"
                 "}"
                 "QCheckBox::indicator {"
                 "  width: 36px;"
                 "  height: 20px;"
                 "  border-radius: 10px;"
                 "  background: %2;"
                 "  border: 1px solid %3;"
                 "}"
                 "QCheckBox::indicator:checked {"
                 "  background: %4;"
                 "  border-color: %4;"
                 "}"
                 "QCheckBox::indicator:unchecked {"
                 "  background: %2;"
                 "}"
                 "QCheckBox::indicator:checked:pressed {"
                 "  background: %5;"
                 "}"
                 "QCheckBox::indicator:checked {"
                 "  image: none;"
                 "}"
                 "QCheckBox::indicator:unchecked {"
                 "  image: none;"
                 "}"
                 "QCheckBox::indicator::before {"
                 "  content: '';"
                 "}")
      .arg(colors.text.name())
      .arg(colors.border.name())
      .arg(colors.border.name())
      .arg(colors.accent.name())
      .arg(colors.accent.darker(110).name());
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
                 "  border-color: %5;"
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
                 "  border: 1px solid %3;"
                 "  border-radius: 6px;"
                 "  selection-background-color: %4;"
                 "  outline: none;"
                 "  padding: 4px;"
                 "}"
                 "QComboBox QAbstractItemView::item {"
                 "  padding: 6px 10px;"
                 "  border-radius: 4px;"
                 "}"
                 "QComboBox QAbstractItemView::item:hover {"
                 "  background: %4;"
                 "}")
      .arg(colors.surface.name())
      .arg(colors.text.name())
      .arg(colors.border.name())
      .arg(colors.hover.name())
      .arg(colors.focus.name());
}

QString Theme::sliderStyle(const ThemeColors &colors) {
  return QString("QSlider::groove:horizontal {"
                 "  height: 6px;"
                 "  background: %1;"
                 "  border-radius: 3px;"
                 "}"
                 "QSlider::handle:horizontal {"
                 "  width: 16px;"
                 "  height: 16px;"
                 "  margin: -6px 0;"
                 "  border-radius: 8px;"
                 "  background: %2;"
                 "}")
      .arg(colors.border.name())
      .arg(colors.accent.name());
}

QString Theme::progressBarStyle(const ThemeColors &colors) {
  return QString("QProgressBar {"
                 "  border: 1px solid %1;"
                 "  border-radius: 6px;"
                 "  text-align: center;"
                 "  background: %2;"
                 "}"
                 "QProgressBar::chunk {"
                 "  background: %3;"
                 "  border-radius: 6px;"
                 "}")
      .arg(colors.border.name())
      .arg(colors.surface.name())
      .arg(colors.accent.name());
}

QString Theme::spinBoxStyle(const ThemeColors &colors) {
  return QString("QSpinBox, QDoubleSpinBox {"
                 "  background: %1;"
                 "  color: %2;"
                 "  border: 1px solid %3;"
                 "  border-radius: 6px;"
                 "  padding: 6px 10px;"
                 "}"
                 "QSpinBox:hover, QDoubleSpinBox:hover {"
                 "  border-color: %4;"
                 "}")
      .arg(colors.surface.name())
      .arg(colors.text.name())
      .arg(colors.border.name())
      .arg(colors.focus.name());
}

QString Theme::toolButtonStyle(const ThemeColors &colors) {
  return QString("QToolButton {"
                 "  background: %1;"
                 "  color: %2;"
                 "  border: 1px solid %3;"
                 "  border-radius: 6px;"
                 "  padding: 6px 12px;"
                 "}"
                 "QToolButton:hover {"
                 "  background: %4;"
                 "}")
      .arg(colors.surface.name())
      .arg(colors.text.name())
      .arg(colors.border.name())
      .arg(colors.hover.name());
}

QString Theme::tabWidgetStyle(const ThemeColors &colors) {
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
                 "  background: %1;"
                 "}"
                 "QTabBar QToolButton:pressed {"
                 "  background: %2;"
                 "}")
      .arg(colors.hover.name())
      .arg(colors.pressed.name());
}

QString Theme::listViewStyle(const ThemeColors &colors) {
  return QString("QListView {"
                 "  background: %1;"
                 "  border: 1px solid %2;"
                 "  border-radius: 6px;"
                 "  outline: none;"
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
      .arg(colors.surface.name())
  .arg(colors.border.name());
}

QString Theme::tableViewStyle(const ThemeColors &colors) {
  return QString("QTableView {"
                 "  background: %1;"
                 "  border: 1px solid %2;"
                 "  gridline-color: %2;"
                 "  border-radius: 6px;"
                 "  outline: none;"
                 "}"
                 "QTableView::item {"
                 "  padding: 4px 8px;"
                 "  background: transparent;"
                 "}"
                 "QTableView::item:hover {"
                 "  background: transparent;"
                 "}"
                 "QHeaderView::section {"
                 "  background: %3;"
                 "  color: %4;"
                 "  border: none;"
                 "  border-bottom: 1px solid %2;"
                 "  padding: 6px 8px;"
                 "  font-weight: 600;"
                 "}"
                 "QTableView::item:selected {"
                 "  background: transparent;"
                 "}")
      .arg(colors.surface.name())
      .arg(colors.border.name())
      .arg(colors.hover.name())
  .arg(colors.text.name());
}

QString Theme::treeViewStyle(const ThemeColors &colors) {
  return QString("QTreeView {"
                 "  background: %1;"
                 "  border: 1px solid %2;"
                 "  border-radius: 6px;"
                 "  outline: none;"
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
                 "  background: %3;"
                 "  color: %4;"
                 "  border: none;"
                 "  border-bottom: 1px solid %2;"
                 "  padding: 6px 8px;"
                 "  font-weight: 600;"
                 "}")
      .arg(colors.surface.name())
      .arg(colors.border.name())
      .arg(colors.hover.name())
  .arg(colors.text.name());
}

QString Theme::groupBoxStyle(const ThemeColors &colors) {
  return QString("QGroupBox {"
                 "  background: %1;"
                 "  border: 1px solid %2;"
                 "  border-radius: 10px;"
                 "  margin-top: 14px;"
                 "  padding: 10px;"
                 "  padding-top: 14px;"
                 "}"
                 "QGroupBox:disabled {"
                 "  background: %3;"
                 "  color: %4;"
                 "}"
                 "QGroupBox::title {"
                 "  subcontrol-origin: margin;"
                 "  left: 12px;"
                 "  padding: 0 6px;"
                 "  color: %5;"
                 "  background: %1;"
                 "}")
      .arg(colors.surface.name())
      .arg(colors.border.name())
      .arg(colors.hover.name())
      .arg(colors.disabledText.name())
      .arg(colors.text.name());
}

QString Theme::menuBarStyle(const ThemeColors &colors) {
  return QString("QMenuBar {"
                 "  background: %1;"
                 "  color: %2;"
                 "  border-bottom: 1px solid %3;"
                 "}"
                 "QMenuBar::item {"
                 "  padding: 6px 12px;"
                 "  background: transparent;"
                 "}"
                 "QMenuBar::item:selected {"
                 "  background: %4;"
                 "  border-radius: 4px;"
                 "}")
      .arg(colors.surface.name())
      .arg(colors.text.name())
      .arg(colors.border.name())
      .arg(colors.hover.name());
}

QString Theme::toolBarStyle(const ThemeColors &colors) {
  return QString("QToolBar {"
                 "  background: %1;"
                 "  border-bottom: 1px solid %2;"
                 "  spacing: 6px;"
                 "}"
                 "QToolBar::separator {"
                 "  background: %2;"
                 "  width: 1px;"
                 "  margin: 4px 6px;"
                 "}"
                 "QToolButton {"
                 "  background: %1;"
                 "  color: %3;"
                 "  border: 1px solid %2;"
                 "  border-radius: 6px;"
                 "  padding: 6px 10px;"
                 "}"
                 "QToolButton:hover {"
                 "  background: %4;"
                 "}")
      .arg(colors.surface.name())
      .arg(colors.border.name())
      .arg(colors.text.name())
      .arg(colors.hover.name());
}

QString Theme::statusBarStyle(const ThemeColors &colors) {
  return QString("QStatusBar {"
                 "  background: %1;"
                 "  color: %2;"
                 "  border-top: 1px solid %3;"
                 "  min-height: 28px;"
                 "}"
                 "QStatusBar::item {"
                 "  border: none;"
                 "}"
                 "QStatusBar QLabel {"
                 "  color: %2;"
                 "  padding: 2px 8px;"
                 "}")
      .arg(colors.surface.name())
      .arg(colors.text.name())
      .arg(colors.border.name());
}

QString Theme::dialogStyle(const ThemeColors &colors) {
  return QString("QDialog {"
                 "  background: %1;"
                 "  color: %2;"
                 "  border: 1px solid %3;"
                 "  border-radius: 10px;"
                 "}"
                 "QDialog QWidget {"
                 "  background: transparent;"
                 "}"
                 "QDialog QLabel {"
                 "  color: %2;"
                 "}")
      .arg(colors.surface.name())
      .arg(colors.text.name())
      .arg(colors.border.name());
}

QString Theme::cardStyle(const ThemeColors &colors) {
  return QString("QWidget#FluentCard {"
                 "  background: %1;"
                 "  border: 1px solid %2;"
                 "  border-radius: 10px;"
                 "}"
                 "QWidget#FluentCard:disabled {"
                 "  background: %3;"
                 "  border-color: %2;"
                 "}")
      .arg(colors.surface.name(), colors.border.name(), colors.hover.name());
}

ThemeManager &ThemeManager::instance() {
  static ThemeManager instance;
  return instance;
}

ThemeManager::ThemeManager() : m_colors(Theme::light()) {}

const ThemeColors &ThemeManager::colors() const { return m_colors; }

bool ThemeManager::accentBorderEnabled() const { return m_accentBorderEnabled; }

void ThemeManager::scheduleThemeChanged()
{
  if (m_themeChangedPending) {
    return;
  }
  m_themeChangedPending = true;
  // Coalesce multiple changes and avoid blocking the current UI event.
  QTimer::singleShot(0, this, [this]() {
    m_themeChangedPending = false;
    emit themeChanged();
  });
}

void ThemeManager::setAccentBorderEnabled(bool enabled)
{
  if (m_accentBorderEnabled == enabled) {
    return;
  }
  m_accentBorderEnabled = enabled;
  scheduleThemeChanged();
}

void ThemeManager::setColors(const ThemeColors &colors) {
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

  m_colors = colors;
  scheduleThemeChanged();
}

ThemeManager::ThemeMode ThemeManager::themeMode() const { return m_mode; }

void ThemeManager::setThemeMode(ThemeMode mode) {
  if (m_mode == mode) {
    return;
  }

  // Preserve current accent across theme mode switches (Fluent behavior).
  const QColor accent = m_colors.accent;

  m_mode = mode;
  ThemeColors next = (mode == ThemeMode::Dark) ? Theme::dark() : Theme::light();
  next.accent = accent;
  next.focus = accent.lighter(135);
  setColors(next);
}

void ThemeManager::setLightMode() { setThemeMode(ThemeMode::Light); }

void ThemeManager::setDarkMode() { setThemeMode(ThemeMode::Dark); }

} // namespace Fluent
