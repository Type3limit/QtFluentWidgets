#pragma once

#include "Fluent/FluentExport.h"

// Exports internal classes from the shared library only for builds with tests
// enabled, so test executables can use them without widening the public API.
#if defined(QTFLUENTWIDGETS_BUILD_AUTOTEST_EXPORTS)
#  define FLUENT_AUTOTEST_EXPORT FLUENT_EXPORT
#else
#  define FLUENT_AUTOTEST_EXPORT
#endif
