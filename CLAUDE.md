# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

QtFluentWidgets is a Fluent Design widget library built on **Qt Widgets** (Qt5 or Qt6), with a demo app and an optional Qt Designer plugin. The monorepo-level `G:\Code\CLAUDE.md` covers cross-project guidelines — only QtFluent-specific notes belong here.

## Doc navigation (agent knowledge base)

A progressive-disclosure engineering knowledge base for agents lives in `.rules/` (local, git-ignored). **Start at [`.rules/README.md`](.rules/README.md)** — it has a "我要做 X → 读哪些" routing table so you can jump straight to the right module doc instead of re-reading the whole repo. High-value entry points:

- Global mental model → [`.rules/overview.md`](.rules/overview.md)
- Code style + hard conventions (CMake explicit source list, theming rules) → [`.rules/conventions.md`](.rules/conventions.md)
- Theming/tokens & control-surface painting → [`.rules/architecture/theme-style.md`](.rules/architecture/theme-style.md)
- Build / run / test / visual baselines → [`.rules/workflows/build-verify.md`](.rules/workflows/build-verify.md)

`.rules/` cites code by symbol name (never line numbers) and links to (does not duplicate) the user-facing `docs/`. Keep it in sync when you change the symbols it references.

## First-time setup

The `third_party/rlottie` git submodule is required — CMake configure fails with `FATAL_ERROR` if it is missing. After cloning without `--recursive`, run once:

```bash
git submodule update --init --recursive
```

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

- CMake auto-detects **Qt6 first, falls back to Qt5**. Required components: `Widgets`, `Svg`, and `UiPlugin` (only when the designer plugin is enabled).
- If Qt isn't on the default search path, pass `-DCMAKE_PREFIX_PATH="C:/Qt/<ver>/<kit>"`.
- The designer plugin builds by default. Disable with `-DFLUENT_BUILD_DESIGNER_PLUGIN=OFF`.
- MSVC: `/utf-8` is already applied to every target — don't strip it. Source files use UTF-8 encoding.
- `BUILD_SHARED_LIBS` is forced ON locally only while adding `rlottie` (then restored), so embedding via `add_subdirectory` doesn't change the parent project's value.

## Running the demo

- Targets: `QtFluentDemo` (the exe) and `run-QtFluentDemo` (launches it).
- `run-QtFluentDemo` sets `QTFLUENT_SUPPRESS_KNOWN_PAINT_WARNINGS=1` by default (controlled by the `QTFLUENT_DEMO_SUPPRESS_KNOWN_PAINT_WARNINGS` CMake option). Visual Studio's F5 debug session also inherits this via `VS_DEBUGGER_ENVIRONMENT`.
- On Windows, the `rlottie` shared library is copied next to the demo / designer-plugin binaries by a post-build step.

## Adding a new widget

Sources are listed **explicitly** in `CMakeLists.txt` — there is no globbing. When you add a widget:

1. Create `include/Fluent/FluentX.h` and `src/FluentX.cpp`.
2. Add **both** paths to the `add_library(QtFluentWidgets ...)` source list in `CMakeLists.txt`.
3. If the widget is showcased in the demo, add the new page sources to the `add_executable(QtFluentDemo ...)` list as well.

All public headers live under `include/Fluent/` and code lives in `namespace Fluent`. Include them as `#include "Fluent/FluentX.h"`.

## Theming / style architecture

- Color tokens and theme switching go through `ThemeManager` + `ThemeColors` (`FluentTheme.h`).
- Input-surface painting (radius, border, focus ring) is centralized in `Style::paintControlSurface()` (`FluentStyle.h`) — reuse it rather than re-implementing the Fluent look.

## Documentation

Per-module docs live in `docs/en-us/` and `docs/zh-cn/` (mirror each other). When adding or changing a public widget API, update the relevant module doc in **both** locales.

## Code style

Match surrounding code. Notable repo conventions:

- 4-space indentation.
- Class/type names PascalCase; methods camelCase; member variables `m_camelCase`.
- Getter `var()`, setter `setVar()` (Qt convention).
- Always use braces, even for single-line bodies.
- Use `override` on reimplemented virtuals; omit redundant `virtual` in the subclass header.
- Q_OBJECT-derived widgets rely on `CMAKE_AUTOMOC` — no manual `.moc` includes needed.
