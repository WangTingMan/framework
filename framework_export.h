#pragma once

#define FRAMEWORK_COMPONENT_BUILD

#if defined(FRAMEWORK_COMPONENT_BUILD)
#if defined(WIN32) || defined(_MSC_VER)

#if defined(FRAMEWORK_IMPLEMENTATION)
#define FRAMEWORK_EXPORT __declspec(dllexport)
#else
#define FRAMEWORK_EXPORT __declspec(dllimport)
#endif  // defined(FRAMEWORK_COMPONENT_BUILD)

#else  // defined(WIN32)
#if defined(FRAMEWORK_COMPONENT_BUILD)
#define FRAMEWORK_EXPORT __attribute__((visibility("default")))
#else
#define FRAMEWORK_EXPORT
#endif  // defined(FRAMEWORK_COMPONENT_BUILD)
#endif

#else  // defined(FRAMEWORK_COMPONENT_BUILD)
#define FRAMEWORK_EXPORT
#endif
