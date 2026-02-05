# 代码编辑器（FluentCodeEditor）

`FluentCodeEditor` 是面向 C++ 的代码编辑器控件：

- 行号区（gutter）、当前行高亮、括号匹配高亮。
- `FluentCppHighlighter` 提供基础 C++ 语法高亮。
- 格式化：支持调用外部 `clang-format`（可配置路径）；未安装时会使用轻量基础格式化。
- 自动格式化：支持 debounce 或“仅失焦/回车后触发”策略。

## 最小示例

```cpp
#include "Fluent/FluentCodeEditor.h"

auto *ed = new Fluent::FluentCodeEditor();
ed->setPlainText(QStringLiteral("int main(){return 0;}\n"));
```

Demo 页面：Inputs（`demo/pages/PageInputs.cpp`）与 Overview（`demo/pages/PageOverview.cpp`）。

对应公开头文件：

- `Fluent/FluentCodeEditor.h`
- `Fluent/FluentCppHighlighter.h`

## clang-format 路径

- 留空：自动从 PATH 中查找 `clang-format`
- 设置路径或命令名：

```cpp
ed->setClangFormatPath(QStringLiteral("clang-format"));
// 或绝对路径（Windows 示例）
ed->setClangFormatPath(QStringLiteral("C:/Program Files/LLVM/bin/clang-format.exe"));
```

关键 API：

- `setClangFormatPath(const QString&)` / `clangFormatPath()`：配置 clang-format 路径或命令名。
- `clangFormatAvailable()`：是否可用（会影响自动格式化与提示）。
- `setClangFormatMissingHintEnabled(bool)`：缺失 clang-format 时是否在编辑器内提示。

## 手动格式化

默认快捷键：`Ctrl+Shift+F`

也可直接调用槽函数：

```cpp
ed->formatDocumentNow();
```

信号：

- `formatStarted()` / `formatFinished(bool applied)`
- `clangFormatAvailabilityChanged(bool available)`

## 自动格式化

### 开关与 debounce

```cpp
ed->setAutoFormatEnabled(true);
ed->setAutoFormatDebounceMs(1200);
```

关键 API：

- `setAutoFormatEnabled(bool)` / `autoFormatEnabled()`
- `setAutoFormatDebounceMs(int)` / `autoFormatDebounceMs()`
- `setMaxAutoFormatCharacters(int)` / `maxAutoFormatCharacters()`：超大文本时避免卡顿。

### 触发策略（你可以选择只在“输入完成”后触发）

```cpp
using P = Fluent::FluentCodeEditor::AutoFormatTriggerPolicy;

// 1) 连续输入 debounce（默认）
ed->setAutoFormatTriggerPolicy(P::DebouncedOnTextChange);

// 2) 仅失焦或按 Enter 换行后触发
ed->setAutoFormatTriggerPolicy(P::OnEnterOrFocusOut);
```

策略说明：

- `DebouncedOnTextChange`：适合英文/代码输入；连续输入停顿后自动格式化。
- `OnEnterOrFocusOut`：更适合“等我输入完成再格式化”的场景（回车换行或失焦触发）。

其他 IDE-like 开关：

- `setLineNumbersEnabled(bool)`：行号区。
- `setCurrentLineHighlightEnabled(bool)`：当前行高亮。
- `setBracketMatchHighlightEnabled(bool)`：括号匹配高亮。
- `setCppHighlightingEnabled(bool)`：语法高亮。
- `setAutoBraceNewlineEnabled(bool)`：基础的括号换行行为（不依赖 clang-format）。

## 高亮器

如果你只需要语法高亮，也可以直接使用：

```cpp
#include "Fluent/FluentCppHighlighter.h"

new Fluent::FluentCppHighlighter(ed->document());
```
