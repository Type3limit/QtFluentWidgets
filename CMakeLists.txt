cmake_minimum_required(VERSION 3.16)
project(QtFluentWidgets LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

option(FLUENT_BUILD_DESIGNER_PLUGIN "Build Qt Designer plugin" ON)
option(FLUENT_BUILD_TESTS "Build QtFluent widget tests" OFF)

find_package(Qt6 COMPONENTS Widgets Svg QUIET)
if(Qt6_FOUND)
    set(QT_PACKAGE Qt6)
else()
    find_package(Qt5 REQUIRED COMPONENTS Widgets Svg)
    set(QT_PACKAGE Qt5)
endif()
get_filename_component(FLUENT_QT_BIN_DIR "${${QT_PACKAGE}_DIR}/../../../bin" ABSOLUTE)

if(FLUENT_BUILD_DESIGNER_PLUGIN)
    find_package(${QT_PACKAGE} REQUIRED COMPONENTS UiPlugin)
endif()

if(FLUENT_BUILD_TESTS)
    find_package(${QT_PACKAGE} REQUIRED COMPONENTS Test)
    enable_testing()
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/rlottie/CMakeLists.txt")
    message(FATAL_ERROR "rlottie submodule is missing. Run: git submodule update --init --recursive")
endif()

set(_QTFLUENT_BUILD_SHARED_LIBS_WAS_DEFINED FALSE)
if(DEFINED BUILD_SHARED_LIBS)
    set(_QTFLUENT_BUILD_SHARED_LIBS_WAS_DEFINED TRUE)
    set(_QTFLUENT_BUILD_SHARED_LIBS_VALUE "${BUILD_SHARED_LIBS}")
endif()

set(BUILD_SHARED_LIBS ON)
set(LOTTIE_MODULE OFF CACHE BOOL "Build rlottie image loader into the rlottie library" FORCE)
set(LOTTIE_TEST OFF CACHE BOOL "Build rlottie tests" FORCE)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
add_subdirectory(third_party/rlottie EXCLUDE_FROM_ALL)
set_target_properties(rlottie PROPERTIES
    AUTOMOC OFF
    AUTORCC OFF
    AUTOUIC OFF
    FOLDER "third_party"
)

if(_QTFLUENT_BUILD_SHARED_LIBS_WAS_DEFINED)
    set(BUILD_SHARED_LIBS "${_QTFLUENT_BUILD_SHARED_LIBS_VALUE}")
else()
    unset(BUILD_SHARED_LIBS)
endif()

add_library(QtFluentWidgets
    include/Fluent/FluentExport.h
    include/Fluent/FluentAnimatedButton.h
    include/Fluent/FluentAnimatedIcon.h
    include/Fluent/FluentAutoSuggestBox.h
    include/Fluent/FluentCodeEditor.h
    include/Fluent/FluentCommandBar.h
    include/Fluent/FluentCppHighlighter.h
    include/Fluent/FluentDropDownButton.h
    include/Fluent/FluentFlyout.h
    include/Fluent/FluentInfoBar.h
    include/Fluent/FluentIcon.h
    include/Fluent/FluentDiagnostics.h
    include/Fluent/FluentLottieWidget.h
    include/Fluent/FluentTheme.h
    include/Fluent/FluentToolTip.h
    include/Fluent/FluentQtCompat.h
    include/Fluent/FluentStyle.h
    include/Fluent/FluentFlowLayout.h
    include/Fluent/FluentMainWindow.h
    include/Fluent/FluentButton.h
    include/Fluent/FluentIconButton.h
    include/Fluent/FluentLabel.h
    include/Fluent/FluentLineEdit.h
    include/Fluent/FluentKeySequenceEdit.h
    include/Fluent/FluentTextEdit.h
    include/Fluent/FluentCalendarPicker.h
    include/Fluent/FluentDatePicker.h
    include/Fluent/datePicker/FluentCalendarPopup.h
    include/Fluent/FluentTimePicker.h
    include/Fluent/FluentColorPicker.h
    include/Fluent/FluentColorDialog.h
    include/Fluent/FluentCard.h
    include/Fluent/FluentCheckBox.h
    include/Fluent/FluentRadioButton.h
    include/Fluent/FluentToggleSwitch.h
    include/Fluent/FluentComboBox.h
    include/Fluent/FluentAngleSelector.h
    include/Fluent/FluentDial.h
    include/Fluent/FluentSlider.h
    include/Fluent/FluentProgressBar.h
    include/Fluent/FluentProgressRing.h
    include/Fluent/FluentSpinBox.h
    include/Fluent/FluentDial.h
    include/Fluent/FluentToolButton.h
    include/Fluent/FluentTabWidget.h
    include/Fluent/FluentListView.h
    include/Fluent/FluentTableView.h
    include/Fluent/FluentTableWidget.h
    include/Fluent/FluentTreeView.h
    include/Fluent/FluentGroupBox.h
    include/Fluent/FluentMenuBar.h
    include/Fluent/FluentMenu.h
    include/Fluent/FluentMotion.h
    include/Fluent/FluentPopupSurface.h
    include/Fluent/FluentToolBar.h
    include/Fluent/FluentStatusBar.h
    include/Fluent/FluentDialog.h
    include/Fluent/FluentMessageBox.h
    include/Fluent/FluentWidget.h
    include/Fluent/FluentResizeHelper.h
    include/Fluent/FluentScrollBar.h
    include/Fluent/FluentScrollArea.h
    include/Fluent/FluentAnnotatedScrollBar.h
    include/Fluent/FluentSplitter.h
    include/Fluent/FluentSplitButton.h
    include/Fluent/FluentToast.h
    include/Fluent/FluentTeachingTip.h
    include/Fluent/FluentDateRangePicker.h
    include/Fluent/FluentNavigationView.h
    src/FluentAnimatedButton.cpp
    src/FluentAnimatedIcon.cpp
    src/FluentAutoSuggestBox.cpp
    src/FluentCodeEditor.cpp
    src/FluentCommandBar.cpp
    src/FluentCppHighlighter.cpp
    src/FluentDropDownButton.cpp
    src/FluentFlyout.cpp
    src/FluentInfoBar.cpp
    src/FluentIcon.cpp
    src/FluentLottieWidget.cpp
    src/FluentTheme.cpp
    src/FluentToolTip.cpp
    src/FluentStyle.cpp
    src/FluentFlowLayout.cpp
    src/FluentMainWindow.cpp
    src/FluentButton.cpp
    src/FluentIconButton.cpp
    src/FluentLabel.cpp
    src/FluentLineEdit.cpp
    src/FluentKeySequenceEdit.cpp
    src/FluentTextEdit.cpp
    src/FluentCalendarPicker.cpp
    src/FluentDatePicker.cpp
    src/datePicker/FluentCalendarPopup.cpp
    src/datePicker/FluentWheelPickerSupport.cpp
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
    src/FluentAngleSelector.cpp
    src/FluentSlider.cpp
    src/FluentProgressBar.cpp
    src/FluentProgressRing.cpp
    src/FluentSpinBox.cpp
    src/FluentDial.cpp
    src/FluentToolButton.cpp
    src/FluentTabWidget.cpp
    src/FluentListView.cpp
    src/FluentTableView.cpp
    src/FluentTableWidget.cpp
    src/FluentTreeView.cpp
    src/FluentGroupBox.cpp
    src/FluentMenuBar.cpp
    src/FluentMenu.cpp
    src/FluentMenuPopupHost.cpp
    src/FluentToolBar.cpp
    src/FluentStatusBar.cpp
    src/FluentDialog.cpp
    src/FluentMessageBox.cpp
    src/FluentWidget.cpp
    src/FluentResizeHelper.cpp
    src/FluentScrollBar.cpp
    src/FluentScrollArea.cpp
    src/FluentAnnotatedScrollBar.cpp
    src/FluentSplitter.cpp
    src/FluentSplitButton.cpp
    src/FluentToast.cpp
    src/FluentTeachingTip.cpp
    src/FluentDateRangePicker.cpp
    src/FluentNavigationView.cpp
    src/FluentDiagnostics.cpp
    resources/icons/fluent_icons.qrc
)

target_include_directories(QtFluentWidgets PUBLIC include)
get_target_property(_QTFLUENTWIDGETS_TARGET_TYPE QtFluentWidgets TYPE)
if(_QTFLUENTWIDGETS_TARGET_TYPE STREQUAL "SHARED_LIBRARY")
    target_compile_definitions(QtFluentWidgets
        PUBLIC QTFLUENTWIDGETS_SHARED
        PRIVATE QTFLUENTWIDGETS_BUILD_LIBRARY
    )
else()
    target_compile_definitions(QtFluentWidgets PUBLIC QTFLUENTWIDGETS_STATIC)
endif()
target_link_libraries(QtFluentWidgets PUBLIC ${QT_PACKAGE}::Widgets ${QT_PACKAGE}::Svg rlottie::rlottie)
if(MSVC)
    target_compile_options(QtFluentWidgets PRIVATE /utf-8)
endif()

add_executable(QtFluentDemo WIN32
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
    demo/lottie_assets.qrc

    demo/pages/PageOverview.h
    demo/pages/PageOverview.cpp
    demo/pages/PageInputs.h
    demo/pages/PageInputs.cpp
    demo/pages/PageButtons.h
    demo/pages/PageButtons.cpp
    demo/pages/PageIcons.h
    demo/pages/PageIcons.cpp
    demo/pages/PageMotion.h
    demo/pages/PageMotion.cpp
    demo/pages/PagePickers.h
    demo/pages/PagePickers.cpp
    demo/pages/PageAngleControls.h
    demo/pages/PageAngleControls.cpp
    demo/pages/PageDataViews.h
    demo/pages/PageDataViews.cpp
    demo/pages/PageContainers.h
    demo/pages/PageContainers.cpp
    demo/pages/PageWindows.h
    demo/pages/PageWindows.cpp
)

target_link_libraries(QtFluentDemo PRIVATE QtFluentWidgets ${QT_PACKAGE}::Widgets)
if(MSVC)
    target_compile_options(QtFluentDemo PRIVATE /utf-8)
endif()
target_compile_definitions(QtFluentDemo
    PRIVATE
        QTFLUENT_RLOTTIE_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/third_party/rlottie/example/resource"
)

if(WIN32)
    add_custom_command(TARGET QtFluentDemo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:rlottie> $<TARGET_FILE_DIR:QtFluentDemo>
        VERBATIM
    )
endif()

if(FLUENT_BUILD_TESTS)
    add_executable(QtFluentWidgetsDesignerCompatTest
        tests/QtFluentWidgetsDesignerCompatTest.cpp
        tests/ui/DesignerCompatMainWindow.ui
        tests/ui/DesignerPromotedMainWindow.ui
    )
    set_target_properties(QtFluentWidgetsDesignerCompatTest PROPERTIES
        AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/tests/ui"
    )
    target_link_libraries(QtFluentWidgetsDesignerCompatTest PRIVATE QtFluentWidgets ${QT_PACKAGE}::Widgets ${QT_PACKAGE}::Test)
    if(MSVC)
        target_compile_options(QtFluentWidgetsDesignerCompatTest PRIVATE /utf-8)
    endif()
    add_test(NAME QtFluentWidgetsDesignerCompatTest COMMAND QtFluentWidgetsDesignerCompatTest)
    add_custom_target(run-QtFluentWidgetsDesignerCompatTest
        COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:QtFluentWidgetsDesignerCompatTest>$<SEMICOLON>${FLUENT_QT_BIN_DIR}$<SEMICOLON>$ENV{PATH}"
            QTFLUENT_DESIGNER_COMPAT_TEST_VISIBLE=1
            QTFLUENT_DESIGNER_COMPAT_TEST_VISIBLE_MS=0
            $<TARGET_FILE:QtFluentWidgetsDesignerCompatTest>
        DEPENDS QtFluentWidgetsDesignerCompatTest
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        VERBATIM
    )

    if(WIN32)
        add_custom_command(TARGET QtFluentWidgetsDesignerCompatTest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:rlottie> $<TARGET_FILE_DIR:QtFluentWidgetsDesignerCompatTest>
            VERBATIM
        )
        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.22)
            set(_fluent_designer_compat_test_environment
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:QtFluentWidgetsDesignerCompatTest>"
                "PATH=path_list_prepend:${FLUENT_QT_BIN_DIR}"
                "QT_QPA_PLATFORM=set:offscreen"
                "QTFLUENT_SUPPRESS_KNOWN_WARNINGS=set:1"
            )
            set_tests_properties(QtFluentWidgetsDesignerCompatTest PROPERTIES
                ENVIRONMENT_MODIFICATION "${_fluent_designer_compat_test_environment}"
            )
        else()
            set_tests_properties(QtFluentWidgetsDesignerCompatTest PROPERTIES
                ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QTFLUENT_SUPPRESS_KNOWN_WARNINGS=1"
            )
        endif()
    endif()

    add_executable(QtFluentWidgetsVisualSmokeTest
        tests/QtFluentWidgetsVisualSmokeTest.cpp
    )
    target_link_libraries(QtFluentWidgetsVisualSmokeTest PRIVATE QtFluentWidgets ${QT_PACKAGE}::Widgets ${QT_PACKAGE}::Test)
    if(MSVC)
        target_compile_options(QtFluentWidgetsVisualSmokeTest PRIVATE /utf-8 /bigobj)
    endif()
    add_test(NAME QtFluentWidgetsVisualSmokeTest COMMAND QtFluentWidgetsVisualSmokeTest -maxwarnings 100000)
    add_test(NAME QtFluentWidgetsVisualSmokeTestLottieHighDpi
        COMMAND QtFluentWidgetsVisualSmokeTest -maxwarnings 100000 lottieTintCacheSurvivesResizeAndThemeChanges
    )
    add_custom_target(run-QtFluentWidgetsVisualSmokeTest
        COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:QtFluentWidgetsVisualSmokeTest>$<SEMICOLON>${FLUENT_QT_BIN_DIR}$<SEMICOLON>$ENV{PATH}"
            QTFLUENT_VISUAL_SMOKE_VISIBLE=1
            QTFLUENT_VISUAL_SMOKE_VISIBLE_MS=0
            QTFLUENT_VISUAL_SMOKE_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/visual-smoke-output
            $<TARGET_FILE:QtFluentWidgetsVisualSmokeTest>
        DEPENDS QtFluentWidgetsVisualSmokeTest
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        VERBATIM
    )

    if(WIN32)
        add_custom_command(TARGET QtFluentWidgetsVisualSmokeTest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:rlottie> $<TARGET_FILE_DIR:QtFluentWidgetsVisualSmokeTest>
            VERBATIM
        )
        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.22)
            set(_fluent_visual_smoke_test_environment
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:QtFluentWidgetsVisualSmokeTest>"
                "PATH=path_list_prepend:${FLUENT_QT_BIN_DIR}"
                "QT_QPA_PLATFORM=set:offscreen"
                "QTFLUENT_VISUAL_SMOKE_OUTPUT_DIR=set:${CMAKE_CURRENT_BINARY_DIR}/visual-smoke-output"
                "QTFLUENT_SUPPRESS_KNOWN_WARNINGS=set:1"
            )
            set_tests_properties(QtFluentWidgetsVisualSmokeTest PROPERTIES
                ENVIRONMENT_MODIFICATION "${_fluent_visual_smoke_test_environment}"
            )
            set(_fluent_visual_smoke_high_dpi_environment
                ${_fluent_visual_smoke_test_environment}
                "QT_SCALE_FACTOR=set:1.5"
            )
            set_tests_properties(QtFluentWidgetsVisualSmokeTestLottieHighDpi PROPERTIES
                ENVIRONMENT_MODIFICATION "${_fluent_visual_smoke_high_dpi_environment}"
            )
        else()
            set_tests_properties(QtFluentWidgetsVisualSmokeTest PROPERTIES
                ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QTFLUENT_VISUAL_SMOKE_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/visual-smoke-output;QTFLUENT_SUPPRESS_KNOWN_WARNINGS=1"
            )
            set_tests_properties(QtFluentWidgetsVisualSmokeTestLottieHighDpi PROPERTIES
                ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_SCALE_FACTOR=1.5;QTFLUENT_VISUAL_SMOKE_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/visual-smoke-output;QTFLUENT_SUPPRESS_KNOWN_WARNINGS=1"
            )
        endif()
    endif()
endif()

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

if(WIN32)
    add_custom_target(render-QtFluentDemoVisualBaselines
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass
            -File "${CMAKE_CURRENT_SOURCE_DIR}/tools/render_demo_visual_baselines.ps1"
            -Executable "$<TARGET_FILE:QtFluentDemo>"
            -OutputDir "${CMAKE_CURRENT_BINARY_DIR}/demo-visual-baselines"
            -QtBinDir "${FLUENT_QT_BIN_DIR}"
            -Pages "overview,input,buttons,pickers,data,containers,windows,motion,settings"
            -Variants "light,dark,accent"
            -ScaleFactors "1,1.5"
            -Platform "windows"
        DEPENDS QtFluentDemo
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        VERBATIM
    )
else()
    add_custom_target(render-QtFluentDemoVisualBaselines
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/demo-visual-baselines
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/demo-visual-baselines
        COMMAND ${CMAKE_COMMAND} -E env
            QTFLUENT_SUPPRESS_KNOWN_PAINT_WARNINGS=1
            QT_QPA_PLATFORM=offscreen
            QT_SCALE_FACTOR=1
            $<TARGET_FILE:QtFluentDemo>
            --render-baselines ${CMAKE_CURRENT_BINARY_DIR}/demo-visual-baselines
            --render-pages overview,input,buttons,pickers,data,containers,windows,motion,settings
            --render-variants light,dark,accent
        COMMAND ${CMAKE_COMMAND} -E env
            QTFLUENT_SUPPRESS_KNOWN_PAINT_WARNINGS=1
            QT_QPA_PLATFORM=offscreen
            QT_SCALE_FACTOR=1.5
            $<TARGET_FILE:QtFluentDemo>
            --render-baselines ${CMAKE_CURRENT_BINARY_DIR}/demo-visual-baselines
            --render-pages overview,input,buttons,pickers,data,containers,windows,motion,settings
            --render-variants light,dark,accent
        DEPENDS QtFluentDemo
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        VERBATIM
    )
endif()

add_custom_target(capture-QtFluentDemoScreenshots
    DEPENDS render-QtFluentDemoVisualBaselines
)

if(WIN32)
    add_custom_target(smoke-QtFluentDemoInteractions
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass
            -File "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_demo_interaction_smoke.ps1"
            -Executable "$<TARGET_FILE:QtFluentDemo>"
            -OutputDir "${CMAKE_CURRENT_BINARY_DIR}/demo-interaction-smoke"
            -QtBinDir "${FLUENT_QT_BIN_DIR}"
            -Platform "windows"
        DEPENDS QtFluentDemo
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        VERBATIM
    )
else()
    add_custom_target(smoke-QtFluentDemoInteractions
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/demo-interaction-smoke
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/demo-interaction-smoke
        COMMAND ${CMAKE_COMMAND} -E env
            QTFLUENT_SUPPRESS_KNOWN_PAINT_WARNINGS=1
            QT_QPA_PLATFORM=offscreen
            $<TARGET_FILE:QtFluentDemo>
            --interaction-smoke ${CMAKE_CURRENT_BINARY_DIR}/demo-interaction-smoke
        DEPENDS QtFluentDemo
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        VERBATIM
    )
endif()

# Visual Studio: set debugger environment for F5.
set_property(TARGET QtFluentDemo PROPERTY VS_DEBUGGER_ENVIRONMENT "QTFLUENT_SUPPRESS_KNOWN_PAINT_WARNINGS=$<IF:$<BOOL:${QTFLUENT_DEMO_SUPPRESS_KNOWN_PAINT_WARNINGS}>,1,0>")

if(FLUENT_BUILD_DESIGNER_PLUGIN)
    add_library(QtFluentWidgetsPlugin MODULE
        designer/FluentWidgetsPlugin.cpp
    )

    target_include_directories(QtFluentWidgetsPlugin PRIVATE include)
    target_link_libraries(QtFluentWidgetsPlugin PRIVATE QtFluentWidgets ${QT_PACKAGE}::UiPlugin ${QT_PACKAGE}::Widgets)
    if(MSVC)
        target_compile_options(QtFluentWidgetsPlugin PRIVATE /utf-8)
    endif()

    set_target_properties(QtFluentWidgetsPlugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/designer"
    )

    if(WIN32)
        add_custom_command(TARGET QtFluentWidgetsPlugin POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:rlottie> $<TARGET_FILE_DIR:QtFluentWidgetsPlugin>
            VERBATIM
        )
    endif()
endif()
