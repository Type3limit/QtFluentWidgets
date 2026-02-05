cmake_minimum_required(VERSION 3.16)
project(QtFluentWidgets LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Widgets UiPlugin Svg QUIET)
if(Qt6_FOUND)
    set(QT_PACKAGE Qt6)
else()
    find_package(Qt5 REQUIRED COMPONENTS Widgets UiPlugin Svg)
    set(QT_PACKAGE Qt5)
endif()

add_library(QtFluentWidgets
    include/Fluent/FluentCodeEditor.h
    include/Fluent/FluentCppHighlighter.h
    include/Fluent/FluentTheme.h
    include/Fluent/FluentStyle.h
    include/Fluent/FluentFlowLayout.h
    include/Fluent/FluentMainWindow.h
    include/Fluent/FluentButton.h
    include/Fluent/FluentLabel.h
    include/Fluent/FluentLineEdit.h
    include/Fluent/FluentTextEdit.h
    include/Fluent/FluentCalendarPicker.h
    include/Fluent/datePicker/FluentCalendarPopup.h
    include/Fluent/FluentTimePicker.h
    include/Fluent/FluentColorPicker.h
    include/Fluent/FluentColorDialog.h
    include/Fluent/FluentCard.h
    include/Fluent/FluentCheckBox.h
    include/Fluent/FluentRadioButton.h
    include/Fluent/FluentToggleSwitch.h
    include/Fluent/FluentComboBox.h
    include/Fluent/FluentSlider.h
    include/Fluent/FluentProgressBar.h
    include/Fluent/FluentSpinBox.h
    include/Fluent/FluentToolButton.h
    include/Fluent/FluentTabWidget.h
    include/Fluent/FluentListView.h
    include/Fluent/FluentTableView.h
    include/Fluent/FluentTreeView.h
    include/Fluent/FluentGroupBox.h
    include/Fluent/FluentMenuBar.h
    include/Fluent/FluentMenu.h
    include/Fluent/FluentToolBar.h
    include/Fluent/FluentStatusBar.h
    include/Fluent/FluentDialog.h
    include/Fluent/FluentMessageBox.h
    include/Fluent/FluentWidget.h
    include/Fluent/FluentResizeHelper.h
    include/Fluent/FluentScrollBar.h
    include/Fluent/FluentScrollArea.h
    include/Fluent/FluentSplitter.h
    include/Fluent/FluentToast.h
    src/FluentCodeEditor.cpp
    src/FluentCppHighlighter.cpp
    src/FluentTheme.cpp
    src/FluentStyle.cpp
    src/FluentFlowLayout.cpp
    src/FluentMainWindow.cpp
    src/FluentButton.cpp
    src/FluentLabel.cpp
    src/FluentLineEdit.cpp
    src/FluentTextEdit.cpp
    src/FluentCalendarPicker.cpp
    src/datePicker/FluentCalendarPopup.cpp
    src/FluentTimePicker.cpp
    src/FluentColorPicker.cpp
    src/FluentColorDialog.cpp
    src/colorPicker/EyeDropper.cpp
    src/colorPicker/ColorPickerWidgets.h
    src/colorPicker/ColorPickerWidgets.cpp
    src/FluentCard.cpp
    src/FluentCheckBox.cpp
    src/FluentRadioButton.cpp
    src/FluentToggleSwitch.cpp
    src/FluentComboBox.cpp
    src/FluentSlider.cpp
    src/FluentProgressBar.cpp
    src/FluentSpinBox.cpp
    src/FluentToolButton.cpp
    src/FluentTabWidget.cpp
    src/FluentListView.cpp
    src/FluentTableView.cpp
    src/FluentTreeView.cpp
    src/FluentGroupBox.cpp
    src/FluentMenuBar.cpp
    src/FluentMenu.cpp
    src/FluentToolBar.cpp
    src/FluentStatusBar.cpp
    src/FluentDialog.cpp
    src/FluentMessageBox.cpp
    src/FluentWidget.cpp
    src/FluentResizeHelper.cpp
    src/FluentScrollBar.cpp
    src/FluentScrollArea.cpp
    src/FluentSplitter.cpp
    src/FluentToast.cpp
    src/FluentDiagnostics.cpp
)

target_include_directories(QtFluentWidgets PUBLIC include)
target_link_libraries(QtFluentWidgets PUBLIC ${QT_PACKAGE}::Widgets ${QT_PACKAGE}::Svg)

add_executable(QtFluentDemo
    demo/main.cpp
    demo/DemoWindow.h
    demo/DemoWindow.cpp
    demo/DemoSidebar.h
    demo/DemoSidebar.cpp
    demo/DemoCodeEditorSettings.h
    demo/DemoCodeEditorSettings.cpp
    demo/sidebar/DemoThemePanel.h
    demo/sidebar/DemoThemePanel.cpp
    demo/sidebar/DemoCodeEditorPanel.h
    demo/sidebar/DemoCodeEditorPanel.cpp
    demo/DemoHelpers.h
    demo/DemoHelpers.cpp

    demo/pages/PageOverview.h
    demo/pages/PageOverview.cpp
    demo/pages/PageInputs.h
    demo/pages/PageInputs.cpp
    demo/pages/PageButtons.h
    demo/pages/PageButtons.cpp
    demo/pages/PagePickers.h
    demo/pages/PagePickers.cpp
    demo/pages/PageDataViews.h
    demo/pages/PageDataViews.cpp
    demo/pages/PageContainers.h
    demo/pages/PageContainers.cpp
    demo/pages/PageWindows.h
    demo/pages/PageWindows.cpp
)

target_link_libraries(QtFluentDemo PRIVATE QtFluentWidgets ${QT_PACKAGE}::Widgets)

# Demo convenience: run with known paint warnings suppressed.
option(QTFLUENT_DEMO_SUPPRESS_KNOWN_PAINT_WARNINGS "Suppress known Qt viewport paint warnings in demo via env var" ON)

if (QTFLUENT_DEMO_SUPPRESS_KNOWN_PAINT_WARNINGS)
    add_custom_target(run-QtFluentDemo
        COMMAND ${CMAKE_COMMAND} -E env QTFLUENT_SUPPRESS_KNOWN_PAINT_WARNINGS=1 $<TARGET_FILE:QtFluentDemo>
        DEPENDS QtFluentDemo
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        VERBATIM
    )
else()
    add_custom_target(run-QtFluentDemo
        COMMAND $<TARGET_FILE:QtFluentDemo>
        DEPENDS QtFluentDemo
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        VERBATIM
    )
endif()

# Visual Studio: set debugger environment for F5.
set_property(TARGET QtFluentDemo PROPERTY VS_DEBUGGER_ENVIRONMENT "QTFLUENT_SUPPRESS_KNOWN_PAINT_WARNINGS=$<IF:$<BOOL:${QTFLUENT_DEMO_SUPPRESS_KNOWN_PAINT_WARNINGS}>,1,0>")

option(FLUENT_BUILD_DESIGNER_PLUGIN "Build Qt Designer plugin" ON)

if(FLUENT_BUILD_DESIGNER_PLUGIN)
    add_library(QtFluentWidgetsPlugin MODULE
        designer/FluentWidgetsPlugin.cpp
    )

    target_include_directories(QtFluentWidgetsPlugin PRIVATE include)
    target_link_libraries(QtFluentWidgetsPlugin PRIVATE QtFluentWidgets ${QT_PACKAGE}::UiPlugin ${QT_PACKAGE}::Widgets)

    set_target_properties(QtFluentWidgetsPlugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/designer"
    )
endif()
