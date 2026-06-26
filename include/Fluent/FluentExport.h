#pragma once

#include <QtGlobal>

#if defined(QTFLUENTWIDGETS_STATIC)
#  define FLUENT_EXPORT
#elif defined(QTFLUENTWIDGETS_SHARED)
#  if defined(QTFLUENTWIDGETS_BUILD_LIBRARY)
#    define FLUENT_EXPORT Q_DECL_EXPORT
#  else
#    define FLUENT_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define FLUENT_EXPORT
#endif
