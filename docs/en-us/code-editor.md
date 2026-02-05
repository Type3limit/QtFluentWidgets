# Code Editor (FluentCodeEditor)

`FluentCodeEditor` is a C++-oriented code editor widget:

- Line numbers (gutter), current line highlight, bracket matching.
- `FluentCppHighlighter` for basic C++ syntax highlighting.
- Formatting via `clang-format` (configurable path); falls back to a lightweight basic formatter.
- Auto-format policies: debounced typing, or only on Enter / focus out.

## Minimal usage

```cpp
#include "Fluent/FluentCodeEditor.h"

auto *ed = new Fluent::FluentCodeEditor();
ed->setPlainText(QStringLiteral("int main(){return 0;}\n"));
```

Demo pages: Inputs (`demo/pages/PageInputs.cpp`) and Overview (`demo/pages/PageOverview.cpp`).

Public headers:

- `Fluent/FluentCodeEditor.h`
- `Fluent/FluentCppHighlighter.h`

## clang-format path

```cpp
ed->setClangFormatPath(QStringLiteral("clang-format"));
// or absolute path
ed->setClangFormatPath(QStringLiteral("C:/Program Files/LLVM/bin/clang-format.exe"));
```

Key APIs:

- `setClangFormatPath(const QString&)` / `clangFormatPath()`
- `clangFormatAvailable()`
- `setClangFormatMissingHintEnabled(bool)`

## Manual format

Default shortcut: `Ctrl+Shift+F`

Or call:

```cpp
ed->formatDocumentNow();
```

Signals:

- `formatStarted()` / `formatFinished(bool applied)`
- `clangFormatAvailabilityChanged(bool available)`

## Auto-format policy

```cpp
using P = Fluent::FluentCodeEditor::AutoFormatTriggerPolicy;

ed->setAutoFormatEnabled(true);
ed->setAutoFormatTriggerPolicy(P::OnEnterOrFocusOut);
```

More related APIs:

- `setAutoFormatDebounceMs(int)` / `autoFormatDebounceMs()`
- `setMaxAutoFormatCharacters(int)` / `maxAutoFormatCharacters()`

Policy notes:

- `DebouncedOnTextChange`: format after you pause typing.
- `OnEnterOrFocusOut`: format only after pressing Enter (new line) or focus out.

Other IDE-like toggles:

- `setLineNumbersEnabled(bool)`
- `setCurrentLineHighlightEnabled(bool)`
- `setBracketMatchHighlightEnabled(bool)`
- `setCppHighlightingEnabled(bool)`
- `setAutoBraceNewlineEnabled(bool)`
