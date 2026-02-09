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

## 外观与交互（实现语义）

### Fluent 边框/焦点环的绘制方式

`FluentCodeEditor` 会把 `QPlainTextEdit` 的 viewport 保持为透明，并在自身 `paintEvent()` 中绘制 Fluent 的 surface（随 hover 轻微混合 `colors.hover`）。

边框与焦点环则由一个独立的 overlay widget 绘制，以避免被 `QPlainTextEdit` 内部绘制覆盖。

同时控件维护 hover/focus 的动画 level：

- 鼠标进入/离开 viewport 会触发 hover level 动画。
- `focusInEvent/focusOutEvent` 会触发 focus level 动画，并更新高亮 selections。

### 行号区（gutter）细节

- 行号区宽度会随 `blockCount()` 的位数变化（按 `'9'` 的字宽估算），并通过 `setViewportMargins(...)` 把文本区域整体向右让出 gutter。
- gutter 会绘制轻微底色（surface 与 hover 的混合），并额外绘制一条细分隔线。
- 当存在选区时，gutter 会对“覆盖到的行范围”做背景高亮；若选区结束正好落在某个 block 的起始位置，会避免把下一行误算进选区范围。

### 括号匹配高亮

- 仅支持三类括号：`()`, `{}`, `[]`。
- 匹配算法是纯字符扫描（按深度计数），不做词法分析：不会识别字符串/注释语境。
	- 这意味着在包含括号字符的字符串/注释附近，匹配结果可能与“真实语义”不一致。
- 若找不到匹配括号，会用 `colors.error` 标出当前位置括号。

## 高亮器

如果你只需要语法高亮，也可以直接使用：

```cpp
#include "Fluent/FluentCppHighlighter.h"

new Fluent::FluentCppHighlighter(ed->document());
```

## 格式化实现细节（与行为约束）

### clang-format：异步执行与“避免覆盖用户新输入”

当 `clangFormatAvailable()` 为 true 时，`formatDocumentNow()` 会通过 `QProcess` 异步调用 clang-format，并使用参数：

- `-assume-filename=code.cpp`（提供更合理的默认语言上下文）

为避免 clang-format 输出覆盖“格式化期间用户继续输入”的新内容，控件会记录开始格式化时的 `document()->revision()`：

- clang-format 返回时，如果 `revision` 未变化：应用输出（可能 applied=false，表示格式化后文本未变化），并发出 `formatFinished(bool applied)`。
- 如果 `revision` 已变化：不会应用这次输出，而是标记一次 pending，稍后按当前触发策略重新安排格式化。

### basic formatter：仅做最小缩进归一化

当 clang-format 不可用时，会回退到内部的 `runBasicFormatter()`：

- 逐行处理，只根据 `{` / `}` 的括号深度归一化缩进。
- 每级缩进固定为 4 个空格。
- 计数括号时会尽量忽略字符串/字符常量，以及 `//` 与 `/* */` 注释中的内容。
- 不会做重排（换行、对齐、include 排序等），目标更像是“把缩进拉回可读状态”。

### 光标与选择区域

格式化前会快照光标与选区（含 anchor/position），格式化后会尝试恢复，减少“格式化把光标跳走”的干扰。

### clang-format 缺失提示

当用户通过“手动格式化”（默认 `Ctrl+Shift+F`）触发格式化、但 clang-format 不可用时：

- 若 `setClangFormatMissingHintEnabled(true)`，编辑器会弹出一次 `QToolTip` 提示（仅首次）。

## 自动格式化：何时会被抑制

自动格式化在以下情况下会直接跳过或延后（实现层面的防误触/防卡顿策略）：

- `isReadOnly()` 为 true。
- 文本处于 IME 预编辑（preedit）阶段：会等待预编辑结束（避免打断中文/日文等输入法的组合输入）。
- 文本过大：`document()->characterCount() > maxAutoFormatCharacters()`。
- clang-format 仍在运行：不会并发启动第二次；会标记为 pending，待进程结束后再按策略触发。
