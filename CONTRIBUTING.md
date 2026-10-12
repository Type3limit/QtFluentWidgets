# Contributing to QtFluentWidgets

QtFluentWidgets is a Qt Widgets library supporting Qt5 and Qt6, with a demo and an optional Qt Designer plugin.

## Build and verification

Initialize the required rlottie submodule before configuring:

```bash
git submodule update --init --recursive
cmake -S . -B build -DFLUENT_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

CMake detects Qt6 first and falls back to Qt5. Required components are Widgets and Svg, plus Test when tests are enabled and UiPlugin when the Designer plugin is enabled. Pass `-DCMAKE_PREFIX_PATH="C:/Qt/<version>/<kit>"` if Qt is not on the search path. To select Qt5 on a machine with both versions, also pass `-DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON`.

Use `-DFLUENT_BUILD_DESIGNER_PLUGIN=OFF` to disable the plugin. `BUILD_SHARED_LIBS` selects the library type; it defaults to ON. The value is temporarily set to ON for rlottie, then restored. On Windows, post-build steps copy rlottie beside the demo and test executables. MSVC targets use `/utf-8`; preserve that option and save sources as UTF-8.

The `run-QtFluentDemo` target runs the demo. `render-QtFluentDemoVisualBaselines` generates images for the theme and scale matrix, and `smoke-QtFluentDemoInteractions` runs the demo interaction checks. The demo suppresses known paint warnings by default through `QTFLUENT_DEMO_SUPPRESS_KNOWN_PAINT_WARNINGS`; suppression is not evidence that a test passed.

Run regression tests appropriate to the change. Record the Qt version, toolchain, commands and actual results. Report blocked or unrun checks explicitly.

## Structure and conventions

- Public headers live in `include/Fluent/`, implementations in `src/`, and code in `namespace Fluent`.
- CMake lists sources explicitly. Add both the header and source to the library target when adding a widget; add any demo page sources to the demo target.
- Use `#include "Fluent/FluentX.h"`, four-space indentation, PascalCase types, camelCase methods and `m_camelCase` members.
- Use Qt-style getters and setters, braces for single-line blocks, and `override` for reimplemented virtual functions.
- Use CMake AUTOMOC for QObject classes. A QObject declared inside a test `.cpp` includes its generated `.moc` file as required by AUTOMOC.
- Theme colors and tokens come from `ThemeManager` and `ThemeColors`. Reuse `Style::paintControlSurface()` for input surface painting.
- User-facing module documentation lives in `docs/en-us/` and `docs/zh-cn/`. Update both when changing a public API or documented behavior. Start with the module links in [README.md](README.md).

## Commits and attribution

Use the contributor's configured Git identity. Do not add automatic AI tool `Co-authored-by` trailers to new commits. Preserve existing commit history, licenses and third-party copyright notices. Repository changes cannot control attribution settings in tools installed outside this repository.
