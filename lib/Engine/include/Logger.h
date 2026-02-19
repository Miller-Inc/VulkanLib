//
// Created by James Miller on 9/3/2025.
//

#pragma once
#include <cstdio>
#include <string>

#ifdef M_LOGGER
#undef M_LOGGER
#endif

#define LOG_RED "\033[31m"
#define LOG_GRN "\033[32m"
#define LOG_BLU "\033[34m"
#define LOG_BLK "\033[35m"
#define LOG_GRY "\033[36m"
#define LOG_YEL "\033[37m"
#define LOG_RESET "\033[0m"

// Keep the enums so existing call sites using Logger::Info etc. still compile
class Logger {
public:
    enum Verbosity {
        Info,
        Warning,
        Error,
        Debug
    };
    enum Category {
        LogTemp,
        LogGraphics,
        LogAudio,
        LogPhysics,
        LogCore,
        LogNetwork,
        LogInput,
    };
};

// Provide default values in case CMake didn't define them
#ifndef LOGGING_ENABLED
#define LOGGING_ENABLED 1
#endif
#ifndef LOGGING_LEVEL
#define LOGGING_LEVEL 2
#endif

// Macro-only logger: behavior depends on LOGGING_ENABLED and LOGGING_LEVEL
#if LOGGING_ENABLED == 0
    // Logging disabled entirely
    #define M_LOGGER(category, verbosity, fmt, ...) (void)0
#else
    // Logging enabled; different detail levels
    #if LOGGING_LEVEL == 1
        // Minimal logging: only WARNING and ERROR
        #define M_LOGGER(category, verbosity, fmt, ...)                                     \
            do {                                                                           \
                int _m_log_v = (int)(verbosity);                                           \
                if (_m_log_v == (int)Logger::Warning || _m_log_v == (int)Logger::Error) { \
                    FILE* _m_logger_stream = (_m_log_v == (int)Logger::Error) ? stderr : stdout;  \
                    const char* _m_logger_color = (_m_log_v == (int)Logger::Warning) ? LOG_YEL : LOG_RED; \
                    fprintf(_m_logger_stream, "%s[%s] [%s] (%s:%d): ",                          \
                            _m_logger_color, #category, #verbosity, __FILE__, __LINE__);         \
                    fprintf(_m_logger_stream, ((std::string)(fmt)).c_str(), ##__VA_ARGS__);     \
                    fprintf(_m_logger_stream, "%s\n", LOG_RESET);                            \
                }                                                                          \
            } while (0)
    #else
        // Full logging: print all levels (Info, Warning, Error, Debug)
        #define M_LOGGER(category, verbosity, fmt, ...)                                     \
            {int _m_log_v = (int)(verbosity);                                           \
            FILE* _m_logger_stream = (_m_log_v == (int)Logger::Error) ? stderr : stdout;  \
            const char* _m_logger_color = (_m_log_v == (int)Logger::Info) ? LOG_GRN       \
                : (_m_log_v == (int)Logger::Warning) ? LOG_YEL                         \
                : (_m_log_v == (int)Logger::Error) ? LOG_RED                           \
                : (_m_log_v == (int)Logger::Debug) ? LOG_GRY                           \
                : LOG_BLK;                                                           \
            fprintf(_m_logger_stream, "%s[%s] [%s] (%s:%d): ",                          \
                    _m_logger_color, #category, #verbosity, __FILE__, __LINE__);         \
            fprintf(_m_logger_stream, fmt, ##__VA_ARGS__);     \
            fprintf(_m_logger_stream, "%s\n", LOG_RESET);}
    #endif
#endif