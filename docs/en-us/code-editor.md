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

## Formatting details (behavioral constraints)

### clang-format: async execution and “do not overwrite new edits”

When `clangFormatAvailable()` is true, `formatDocumentNow()` runs clang-format via `QProcess` **asynchronously**, with:

- `-assume-filename=code.cpp` (gives clang-format better defaults)

To avoid overwriting user edits made while formatting is in flight, the editor records `document()->revision()` at the start:

- If the revision is unchanged when clang-format finishes: apply the output and emit `formatFinished(bool applied)`.
- If the revision changed: the output is treated as stale and will not be applied; a pending re-format is scheduled according to the current policy.

### Basic formatter: minimal indentation normalization

If clang-format is not available, the editor falls back to an internal “basic formatter”:

- Line-based, indentation-only normalization based on `{` / `}` brace depth.
- Indent width is fixed at **4 spaces**.
- When counting braces, it tries to ignore strings/chars and `//` / `/* */` comments.
- It does not perform reflow/alignment/include sorting.

### Caret and selection restore

Before applying formatted text, the editor snapshots caret + selection (anchor/position) and restores them afterwards to reduce cursor jumps.

### clang-format missing hint

If the user manually triggers formatting (default `Ctrl+Shift+F`) while clang-format is missing, and `setClangFormatMissingHintEnabled(true)` is set, the editor shows a one-time `QToolTip` hint.

## Auto-format: when it is suppressed

Auto-format will be skipped or delayed in these cases (to avoid disruption and performance issues):

- `isReadOnly()` is true.
- IME preedit is active (composition in progress): formatting is postponed until preedit ends.
- The document is too large: `document()->characterCount() > maxAutoFormatCharacters()`.
- clang-format process is still running: no concurrent formatting; a pending run will be scheduled.

## Visuals & interactions (implementation notes)

### Fluent surface vs. border/focus ring

`FluentCodeEditor` keeps the viewport transparent and paints the Fluent **surface fill** in its own `paintEvent()`.
The border and focus ring are painted by a dedicated overlay widget so they are not overwritten by `QPlainTextEdit` internals.

Hover/focus levels are animated:

- Enter/leave on the viewport drives hover animation.
- Focus in/out drives focus animation and updates extra selections.

### Gutter (line numbers) details

- Gutter width adapts to `blockCount()` digit count and is applied via `setViewportMargins(...)`.
- The gutter has a subtle tint and a 1px divider line.
- For selections, the gutter highlights the covered block range; if selection ends exactly at a block start, the next block is excluded from the highlight.

### Bracket matching

- Only supports `()`, `{}`, `[]`.
- Matching is a simple character scan with depth counting (no lexer): brackets inside strings/comments are not excluded.
- If no match is found, the bracket is marked with `colors.error`.
