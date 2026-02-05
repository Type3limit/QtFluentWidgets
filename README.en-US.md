# QtFluentWidgets

A Fluent Design styled widget library built on **Qt Widgets** (with a full demo app and an optional Qt Designer plugin). It provides a unified theme/color system and a set of commonly used controls.

- Languages:
  - English: README.en-US.md (this file)
  - 中文: [README.zh-CN.md](README.zh-CN.md)

## Highlights

- Qt5 / Qt6 compatible (CMake auto-detects Qt6, otherwise falls back to Qt5).
- Unified theming: `ThemeManager` + `ThemeColors`.
- Fluent-like input surfaces via `Style::paintControlSurface()` (radius/border/focus ring).
- Demo covers all widgets and includes a sidebar panel to configure the CodeEditor (including clang-format path).
- Optional Qt Designer plugin (enabled by default).

## Quick Start

### Requirements

- CMake >= 3.16
- Qt Widgets (Qt5 or Qt6)
- Qt Svg, Qt UiPlugin (Demo / Designer plugin)

### Build (out-of-source recommended)

```bash
cmake -S . -B build
cmake --build build --config Release
```

If Qt is not in your default search path, set `CMAKE_PREFIX_PATH` (Windows example):

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.6.0/msvc2019_64"
cmake --build build --config Release
```

### Run the demo

- CMake target: `QtFluentDemo`
- Convenience target: `run-QtFluentDemo`

## Use in Your Project

### Option A: add_subdirectory (simplest)

```cmake
add_subdirectory(path/to/QtFluent)

target_link_libraries(your_app PRIVATE QtFluentWidgets)
```

Include headers from `include/Fluent/`:

```cpp
#include "Fluent/FluentButton.h"
#include "Fluent/FluentTheme.h"

using namespace Fluent;
```

## Docs (by module)

Controls are documented by module under `docs/`:

- Theme / Style: [docs/en-us/theme-style.md](docs/en-us/theme-style.md)
- Buttons & toggles: [docs/en-us/buttons.md](docs/en-us/buttons.md)
- Inputs: [docs/en-us/inputs.md](docs/en-us/inputs.md)
- Code editor: [docs/en-us/code-editor.md](docs/en-us/code-editor.md)
- Pickers: [docs/en-us/pickers.md](docs/en-us/pickers.md)
- Data views: [docs/en-us/data-views.md](docs/en-us/data-views.md)
- Containers / layout: [docs/en-us/containers-layout.md](docs/en-us/containers-layout.md)
- Windows / menus / dialogs: [docs/en-us/windows-dialogs.md](docs/en-us/windows-dialogs.md)
- Utilities: [docs/en-us/utilities.md](docs/en-us/utilities.md)

## Qt Designer Plugin

The Designer plugin is built by default and placed under `designer/` inside your build directory. Copy the built plugin into Qt Designer's plugin directory.

Disable the plugin build:

```bash
cmake -S . -B build -DFLUENT_BUILD_DESIGNER_PLUGIN=OFF
cmake --build build
```

## License

Add license information here if you plan to publish/distribute (the repository currently does not include a license text).
