//
// RosKmdLogging.h -- corrected monolithic fallback header for builds without WPP
//
// Drop-in replacement for the project's RosKmdLogging.h when building without
// WPP/.tmh generation. This header:
//
//  - Declares the existing _RosLogBugcheck/_RosLogDebug (implemented in
//    RosKmdLogging.cpp shipped with the repo) and does NOT redefine them.
//  - Provides safe inline RosLog overloads for const char* and const wchar_t*
//    format strings (variadic), to allow call-sites using wide or narrow format
//    literals to compile and print.
//  - Adds conservative stubs for a few WPP-related macros/types used elsewhere
//    (WPP_INIT_TRACING, WPP_CLEANUP, RECORDER_CONFIGURE_PARAMS, etc.) so the
//    code compiles without WPP-enabled toolchain.
//  - Defines the ROS_LOG_* / ROS_TRACE_EVENTS / ROS_LOG_ASSERTION macros to
//    route to the RosLog overloads. These macros accept zero or more args and
//    support multi-line argument lists.
//
// IMPORTANT:
// - If you enable WPP tracing and generate .tmh files, remove or guard these
//   fallback definitions to avoid conflicts with real WPP-generated macros.
// - This header tries to be minimal and safe; it is intended for development /
//   non-WPP builds and not to reproduce full WPP functionality.
//
// Based on earlier attempts and reported compile errors, this version fixes:
//  - RECORDER_CONFIGURE_PARAMS_INIT to use '.' (not '->')
//  - WPP_RECORDER_LEVEL_FILTER to expand to an l-value (recorderConfigureParams.LevelFilter)
//    so code that assigns to it compiles.
//  - Overloaded RosLog functions so ROS_LOG_* calls with wide literals (L"...")
//    are handled correctly.
//
// Author: Assembled to address the user's build issues
//
#ifndef isumdd_precomp_h_rosumd
#ifndef _ROSLOGGING_FALLBACK_FIXED_H_
#define _ROSLOGGING_FALLBACK_FIXED_H_ 1


// Kernel-mode headers
#include <ntddk.h>
#include <wdm.h>
#include <ntstrsafe.h>
#include <stdarg.h>


#ifdef __cplusplus
extern "C" {
#endif

    // The real implementations of these functions live in RosKmdLogging.cpp
    // in the repository. We only declare them here so other units can call them.
    // Do NOT provide definitions here to avoid duplicate definition/link errors.
    extern int _RosLogBugcheck(ULONG Level);
    extern int _RosLogDebug(ULONG Level);

    // Keep the WPP control GUID block intact so projects that still use WPP can
    // parse this header. (This is inert when WPP is not used.)
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(ROSKMD, (B5B486C1,F57B,4993,8ED7,E3C2F5E4E65A), \
        WPP_DEFINE_BIT(ROS_TRACING_DEFAULT) \
        WPP_DEFINE_BIT(ROS_TRACING_PRESENT) \
        WPP_DEFINE_BIT(ROS_TRACING_VIDPN) \
        WPP_DEFINE_BIT(ROS_TRACING_DEBUG) \
        WPP_DEFINE_BIT(ROS_TRACING_BUGCHECK) \
    )

// begin_wpp config (kept for clarity)
//
// FUNC ROS_LOG_CRITICAL_ERROR{LEVEL=TRACE_LEVEL_CRITICAL, FLAGS=ROS_TRACING_BUGCHECK}(MSG, ...);
// USEPREFIX (ROS_LOG_CRITICAL_ERROR, "%!STDPREFIX! [%s @ %u] CRITICAL ERROR:", __FILE__, __LINE__);
//
// FUNC ROS_LOG_ASSERTION{LEVEL=TRACE_LEVEL_ERROR, FLAGS=ROS_TRACING_DEBUG}(MSG, ...);
// USEPREFIX (ROS_LOG_ASSERTION, "%!STDPREFIX! [%s @ %u] ASSERTION :", __FILE__, __LINE__);
//
// FUNC ROS_LOG_ERROR{LEVEL=TRACE_LEVEL_ERROR, FLAGS=ROS_TRACING_DEFAULT}(MSG, ...);
// USEPREFIX (ROS_LOG_ERROR, "%!STDPREFIX! [%s @ %u] ERROR :", __FILE__, __LINE__);
//
// FUNC ROS_LOG_LOW_MEMORY{LEVEL=TRACE_LEVEL_ERROR, FLAGS=ROS_TRACING_DEFAULT}(MSG, ...);
// USEPREFIX (ROS_LOG_LOW_MEMORY, "%!STDPREFIX! [%s @ %u] LOW MEMORY :", __FILE__, __LINE__);
//
// FUNC ROS_LOG_WARNING{LEVEL=TRACE_LEVEL_WARNING, FLAGS=ROS_TRACING_DEFAULT}(MSG, ...);
// USEPREFIX (ROS_LOG_WARNING, "%!STDPREFIX! [%s @ %u] WARNING :", __FILE__, __LINE__);
//
// FUNC ROS_LOG_INFORMATION{LEVEL=TRACE_LEVEL_INFORMATION, FLAGS=ROS_TRACING_DEFAULT}(MSG, ...);
// USEPREFIX (ROS_LOG_INFORMATION, "%!STDPREFIX! [%s @ %u] INFO :", __FILE__, __LINE__);
//
// FUNC ROS_LOG_TRACE{LEVEL=TRACE_LEVEL_VERBOSE, FLAGS=ROS_TRACING_DEFAULT}(MSG, ...);
// USEPREFIX (ROS_LOG_TRACE, "%!STDPREFIX! [%s @ %u] TRACE :", __FILE__, __LINE__);
//
// FUNC ROS_TRACE_EVENTS(LEVEL, FLAGS, MSG, ...);
// USEPREFIX (ROS_TRACE_EVENTS, "%!STDPREFIX! [%s @ %u] TRACE :", __FILE__, __LINE__);
//
// end_wpp

// -----------------------------------------------------------------------------
// Minimal WPP-related stubs and constants for non-WPP builds
// -----------------------------------------------------------------------------

// Ensure TRACE_LEVEL_* constants exist (normally provided by WPP)
#ifndef TRACE_LEVEL_NONE
#define TRACE_LEVEL_NONE         0
#define TRACE_LEVEL_CRITICAL     1
#define TRACE_LEVEL_ERROR        2
#define TRACE_LEVEL_WARNING      3
#define TRACE_LEVEL_INFORMATION  4
#define TRACE_LEVEL_VERBOSE      5
#endif

// Provide no-op WPP init/cleanup macros if WPP's versions aren't available.
#ifndef WPP_INIT_TRACING
#define WPP_INIT_TRACING(DeviceObject, RegistryPath) ((void)0)
#endif

#ifndef WPP_CLEANUP
#define WPP_CLEANUP() ((void)0)
#endif



    // Important: use '.' because the caller passes the struct (not a pointer)
#ifndef RECORDER_CONFIGURE_PARAMS_INIT
#define RECORDER_CONFIGURE_PARAMS_INIT(P) ((P).LevelFilter = 0)
#endif

// Provide a minimal WppRecorderConfigure fallback that simply returns success.
#ifndef WppRecorderConfigure
#define WppRecorderConfigure(P) (STATUS_SUCCESS)
#endif

// WPP_RECORDER_LEVEL_FILTER is used in assignments in RosKmdGlobal.cpp. The
// code assigns to WPP_RECORDER_LEVEL_FILTER(flag) = recorderConfigureParams.LevelFilter;
// We must expand this macro to an l-value referring to the local variable
// recorderConfigureParams.LevelFilter. This is a fragile but pragmatic fallback
// that works for the project's code (only used in that translation unit).
#ifndef WPP_RECORDER_LEVEL_FILTER
#define WPP_RECORDER_LEVEL_FILTER(flag) (recorderConfigureParams.LevelFilter)
#endif

// Provide flags used in the source to avoid undefined-identifier errors.
#ifndef ROS_TRACING_VIDPN
#define ROS_TRACING_VIDPN 0x1
#endif
#ifndef ROS_TRACING_PRESENT
#define ROS_TRACING_PRESENT 0x2
#endif

// Provide DECLARE_CONST_UNICODE_STRING if missing (used in a few places)
#ifndef DECLARE_CONST_UNICODE_STRING
#define DECLARE_CONST_UNICODE_STRING(name, str) \
    static const UNICODE_STRING name = RTL_CONSTANT_STRING(str)
#endif

#ifdef __cplusplus
} // extern "C"
#endif

// -----------------------------------------------------------------------------
// C++ inline RosLog overloads (narrow + wide). These are static inline to avoid
// duplicate external symbols across translation units.
// -----------------------------------------------------------------------------
#ifdef __cplusplus

// Narrow format string variant
static inline int RosLog(
    const char* file,
    unsigned line,
    const char* levelStr,
    const char* format,
    ...)
{
    // Compose prefix
    char prefix[384];
    RtlStringCchPrintfA(prefix, ARRAYSIZE(prefix), "[%s @ %u] %s : ", file, line, levelStr);

    // Print prefix
    DbgPrintEx(DPFLTR_IHVVIDEO_ID, DPFLTR_TRACE_LEVEL, "%s", prefix);

    // Print formatted message using varargs
    va_list args;
    va_start(args, format);
    vDbgPrintEx(DPFLTR_IHVVIDEO_ID, DPFLTR_TRACE_LEVEL, format, args);
    va_end(args);

    DbgPrintEx(DPFLTR_IHVVIDEO_ID, DPFLTR_TRACE_LEVEL, "\n");
    return 0;
}

// Wide format string variant
static inline int RosLog(
    const char* file,
    unsigned line,
    const char* levelStr,
    const wchar_t* format,
    ...)
{
    // Prefix
    char prefix[384];
    RtlStringCchPrintfA(prefix, ARRAYSIZE(prefix), "[%s @ %u] %s : ", file, line, levelStr);
    DbgPrintEx(DPFLTR_IHVVIDEO_ID, DPFLTR_TRACE_LEVEL, "%s", prefix);

    // Format wide into buffer
    wchar_t wbuf[2048];
    va_list args;
    va_start(args, format);
    RtlStringCchVPrintfW(wbuf, ARRAYSIZE(wbuf), format, args);
    va_end(args);

    // Convert to ANSI for DbgPrintEx
    UNICODE_STRING ustr;
    RtlInitUnicodeString(&ustr, wbuf);
    ANSI_STRING astr;
    NTSTATUS st = RtlUnicodeStringToAnsiString(&astr, &ustr, TRUE);
    if (NT_SUCCESS(st)) {
        DbgPrintEx(DPFLTR_IHVVIDEO_ID, DPFLTR_TRACE_LEVEL, "%s", astr.Buffer);
        DbgPrintEx(DPFLTR_IHVVIDEO_ID, DPFLTR_TRACE_LEVEL, "\n");
        RtlFreeAnsiString(&astr);
    }
    else {
        DbgPrintEx(DPFLTR_IHVVIDEO_ID, DPFLTR_TRACE_LEVEL, "(failed to convert wide log)\n");
    }
    return 0;
}

#endif // __cplusplus

// -----------------------------------------------------------------------------
// Logging macros that forward to RosLog. These accept zero or more args.
// Use the C++ overloads of RosLog to handle narrow vs wide format literals.
// -----------------------------------------------------------------------------
#ifndef ROS_LOG_ERROR
#define ROS_LOG_ERROR(Format, ...)    RosLog(__FILE__, __LINE__, "ERROR", Format, ##__VA_ARGS__)
#define ROS_LOG_WARNING(Format, ...)  RosLog(__FILE__, __LINE__, "WARNING", Format, ##__VA_ARGS__)
#define ROS_LOG_INFORMATION(Format, ...) RosLog(__FILE__, __LINE__, "INFO", Format, ##__VA_ARGS__)
#define ROS_LOG_LOW_MEMORY(Format, ...) RosLog(__FILE__, __LINE__, "LOW MEMORY", Format, ##__VA_ARGS__)
#define ROS_LOG_TRACE(Format, ...)    RosLog(__FILE__, __LINE__, "TRACE", Format, ##__VA_ARGS__)
#define ROS_LOG_CRITICAL_ERROR(Format, ...) RosLog(__FILE__, __LINE__, "CRITICAL ERROR", Format, ##__VA_ARGS__)

// If a call site explicitly uses wide literal but macro couldn't select overload,
// provide explicit wide macros (optional)
#define ROS_LOG_TRACE_W(Format, ...)  RosLog(__FILE__, __LINE__, "TRACE", Format, ##__VA_ARGS__)

// Assertions -- print then route to existing helpers for interactive behavior
#define ROS_LOG_ASSERTION(Format, ...) \
    do { RosLog(__FILE__, __LINE__, "ASSERTION", Format, ##__VA_ARGS__); (void)_RosLogDebug(TRACE_LEVEL_ERROR); } while(0)

#define ROS_CRITICAL_ASSERT(Exp) \
    do { if (!(Exp)) { RosLog(__FILE__, __LINE__, "CRITICAL ASSERTION", "%s", #Exp); (void)_RosLogBugcheck(TRACE_LEVEL_CRITICAL); } } while(0)

#define ROS_ASSERT(Exp) \
    do { if (!(Exp)) { RosLog(__FILE__, __LINE__, "ASSERTION", "%s", #Exp); (void)_RosLogDebug(TRACE_LEVEL_ERROR); } } while(0)

// Trace events: (Level, Flags, Format, ...)
#define ROS_TRACE_EVENTS(Level, Flags, Format, ...) \
    do { RosLog(__FILE__, __LINE__, "TRACE_EVENTS", Format, ##__VA_ARGS__); } while(0)

#endif // ROS_LOG_ERROR

#endif // _ROSLOGGING_FALLBACK_FIXED_H_
#endif

#ifdef isumdd_precomp_h_rosumd
static inline int RosLog(
    const char* file,
    unsigned line,
    const char* levelStr,
    const char* format,
    ...)
{
    return 0;
}
#define ROS_LOG_TRACE(Format, ...)    RosLog(__FILE__, __LINE__, "TRACE", Format, ##__VA_ARGS__)
#define ROS_LOG_ERROR(Format, ...)    RosLog(__FILE__, __LINE__, "ERROR", Format, ##__VA_ARGS__)
#define WPP_CLEANUP() ((void)0)
#define WPP_INIT_TRACING(DeviceObject, RegistryPath) ((void)0)
#endif