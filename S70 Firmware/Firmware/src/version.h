#ifndef _SRC_VERSION_H_
#define _SRC_VERSION_H_

#include "version-build.h"
#ifndef FW_VERSION_BUILD_STR
#define FW_VERSION_BUILD_STR ""
#warning FW_VERSION_BUILD_STR is undefined.
#endif

#define FW_MAJOR_VER 7
#define FW_MINOR_VER 1
#define FW_PATCH_VER 1

#define FW_VERSION_ENABLE_PRERELEASE 0
#define FW_VERSION_PRERELEASE_STR ""

#ifdef MACRO_AS_STRING
#error "Macro MACRO_AS_STRING already defined."
#endif

#ifdef MACRO_AS_STRING_HELPER
#error "Macro MACRO_AS_STRING_HELPER already defined."
#endif

#define MACRO_AS_STRING_HELPER(arg) #arg
#define MACRO_AS_STRING(macro) MACRO_AS_STRING_HELPER(macro)

#if FW_VERSION_ENABLE_PRERELEASE
#define FW_VERSION_STR                                     \
    MACRO_AS_STRING(FW_MAJOR_VER)                          \
    "." MACRO_AS_STRING(FW_MINOR_VER) "." MACRO_AS_STRING( \
        FW_PATCH_VER) "-" FW_VERSION_PRERELEASE_STR "+" FW_VERSION_BUILD_STR
#else
#define FW_VERSION_STR                                     \
    MACRO_AS_STRING(FW_MAJOR_VER)                          \
    "." MACRO_AS_STRING(FW_MINOR_VER) "." MACRO_AS_STRING( \
        FW_PATCH_VER) "+" FW_VERSION_BUILD_STR
#endif

#endif
