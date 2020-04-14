#pragma once

#include <QtGlobal>

#if defined(CONAN_LIBRARY)
#  define CONANSHARED_EXPORT Q_DECL_EXPORT
#else
#  define CONANSHARED_EXPORT Q_DECL_IMPORT
#endif
