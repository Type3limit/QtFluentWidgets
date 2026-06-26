#pragma once

#include "Fluent/FluentExport.h"

namespace Fluent {

class FLUENT_EXPORT Diagnostics final
{
public:
    Diagnostics() = delete;

    // Installs a pass-through Qt message handler that can filter selected warnings.
    static void installMessageHandler();
    static bool messageHandlerInstalled();

    // Filters only the known high-frequency Qt warnings that can dominate Debug output.
    static void setKnownQtWarningSuppressionEnabled(bool enabled);
    static bool knownQtWarningSuppressionEnabled();

    // Enables/disables all QtWarningMsg output. Use only for local Debug profiling.
    static void setQtWarningOutputEnabled(bool enabled);
    static bool qtWarningOutputEnabled();
};

} // namespace Fluent
