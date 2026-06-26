#pragma once

#define FOR_EMBEDDED_SYSTEM

#ifdef _MSC_VER

#ifdef FOR_EMBEDDED_SYSTEM

#if defined(_CPPUNWIND) || (defined(_HAS_EXCEPTIONS) && (_HAS_EXCEPTIONS != 0))
#error "[Error] Please disable C++ Exceptions! (Remove /EHsc and add _HAS_EXCEPTIONS=0 in preprocessor definitions)"
#endif

#ifdef _CPPRTTI
#error "[Error] Please disable RTTI! (Specify /GR- in compiler options)"
#endif

#endif

#endif

