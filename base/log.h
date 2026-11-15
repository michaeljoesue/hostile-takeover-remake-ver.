#ifndef __LOG_H__
#define __LOG_H__

// Compile-in logging facilities. If not turned on at compile time, the LOG
// statements will not produce any code.
//
// Two major flags:
// DEBUG_LOGGING: turns on debug logging (LOG statements). If not turned on,
// all debug LOG statements will not get compiled in.
// RELEASE_LOGGING: turns on release logging (RLOG statements). If not turned
// on, all RLOG statements will not get compled in.

// legacy: turn on LOGGING if DEBUG_LOGGING is on and vice versa. LOGGING is
// used both to turn on logging, and as a compile time conditional in existing
// code.

#if defined(LOGGING)
#if !defined(DEBUG_LOGGING)
#define DEBUG_LOGGING
#endif
#else
#if defined(DEBUG_LOGGING)
#define LOGGING
#endif
#endif

// If DEBUG_LOGGING, RELEASE_LOGGING is turned on too
#if defined(DEBUG_LOGGING)
#define RELEASE_LOGGING
#endif

// If either of these types of logging are turned on, we need CONSOLE_LOGGING
#if defined(DEBUG_LOGGING) || defined(RELEASE_LOGGING)
#define CONSOLE_LOGGING
#endif

#if !defined(DEBUG_LOGGING) || !defined(RELEASE_LOGGING)
#define EMPTY_LOGGING
#endif

// Name of the current function, with its signature
#if defined(_MSC_VER)
#define LOG_FUNCTION __FUNCSIG__
#else
#define LOG_FUNCTION __PRETTY_FUNCTION__
#endif

#if defined(DEBUG_LOGGING)
#define LOG() base::ConsoleLog(LOG_FUNCTION, NULL, __LINE__).stream()
#define LOGF() base::ConsoleLog(LOG_FUNCTION, __FILE__, __LINE__).stream()
#define LOGS() base::ConsoleLog().stream()
#define LOGX() base::ConsoleLog(NULL, NULL, -1).stream()
#else
#define LOG() while(false) base::EmptyLog()
#define LOGF() while(false) base::EmptyLog()
#define LOGS() while(false) base::EmptyLog()
#define LOGX() while(false) base::EmptyLog()
#endif

#if defined(DEBUG_LOGGING) || defined(RELEASE_LOGGING)
#define RLOG() base::ConsoleLog(LOG_FUNCTION, NULL, __LINE__).stream()
#define RLOGF() base::ConsoleLog(LOG_FUNCTION, __FILE__, __LINE__).stream()
#define RLOGS() base::ConsoleLog().stream()
#define RLOGX() base::ConsoleLog(NULL, NULL, -1).stream()
#else
#define RLOG() while(false) base::EmptyLog()
#define RLOGF() while(false) base::EmptyLog()
#define RLOGS() while(false) base::EmptyLog()
#define RLOGX() while(false) base::EmptyLog()
#endif

// Format string compile time check
#if defined(__GNUC__) || defined(__clang__)
#define printfFormat12 __attribute__((format(printf, 1, 2)))
#else
#define printfFormat12
#endif

// Log string formatting class (for legacy reasons called Log)

#if defined(CONSOLE_LOGGING)

#include <string>
#include <stdarg.h>
#include <stdio.h>


namespace base {
class Log {
public:
    static std::string Format(const char *format, ...) printfFormat12 {
        va_list va;
        va_start(va, format);
        std::string str = vFormat(format, va);
        va_end(va);
        return str;
    }
private:
    static std::string vFormat(const char *format, va_list va) {
        char sz[512];
        vsnprintf(sz, sizeof(sz), format, va);
        return std::string(sz);
    }
};
} // namespace base

#else

namespace base {
class Log {
public:
    static int Format(const char *format, ...) printfFormat12 { return 0; }
};
} // namespace base

#endif // CONSOLE_LOGGING

// Empty Logging

#if defined(EMPTY_LOGGING)
namespace base {
class EmptyLog {
public:
    EmptyLog() {}
    EmptyLog &operator<<(const char *s) { return *this; }
    EmptyLog &operator<<(int n) { return *this; }
#ifdef CONSOLE_LOGGING
    EmptyLog &operator<<(const std::string& s) { return *this; }
#endif
};
} // namespace base
#endif // EMPTY_LOGGING

// Console Logging

#if defined(CONSOLE_LOGGING)
#include "inc/basictypes.h"
#include <iostream>
#include <sstream>
#include <time.h>
#include <chrono>

// Log output goes to stderr and to the debugger (Visual Studio's Output
// window). Declared here to avoid including windows.h everywhere.
extern "C" __declspec(dllimport) void __stdcall OutputDebugStringA(
        const char *lpOutputString);

namespace base {
class ConsoleLog {
public:
    ConsoleLog(const char *function, const char *file, int line) {
        time_t t = time(NULL);
        struct tm *tm = localtime(&t); 
        int ms = (int)(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()
                % 1000);
        stream_ << Log::Format("%02d%02d%02d%02d.%02d%03d|",
                tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min,
                tm->tm_sec, ms);

        if (function != NULL) {
            std::string func(function);
            size_t n2 = func.find('(');
            if (n2 != std::string::npos) {
                size_t n1 = func.rfind(' ', n2);
                if (n1 != std::string::npos) {
                    func = std::string(func, n1 + 1, n2 - n1 - 1);
                }
            }
            if (line == -1 && file == NULL) {
                stream_ << func << "(): ";
            } else {
                stream_ << func << "() ";
            }
        }

        if (line != -1) {
            if (file != NULL) {
                stream_ << "line " << line << " ";
            } else {
                stream_ << "line " << line << ": ";
            }
        }

        if (file != NULL) {
            stream_ << "file " << file << ": ";
        }
    }

    ConsoleLog() {
    }

    ~ConsoleLog() {
        stream_ << "\n";
        std::string str = stream_.str();
        std::cerr << str;
        OutputDebugStringA(str.c_str());
    }

    std::ostringstream& stream() { return stream_; }

private:
    std::ostringstream stream_;
};
} // namespace base
#endif // CONSOLE_LOGGING

#endif // __LOG_H__
