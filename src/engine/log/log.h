#pragma once 

void log_init();

/* These correspond to log_level() functions. */
enum LogLevel {
	
	LOG_DEBUG, 
	/* Not logged in release builds */

	LOG_INFO,

	LOG_WARN,
	/* Example: Missing font for a glyph */

	LOG_ERROR,
	/* Unexpected errors that don't take down the whole engine, at least 
	 * immediately. 
	 * Example: Text rendering ran out of cache or glyph count was exceeded.
	 * This doesn't need to crash the engine, but rendering will be broken.
	 */

	LOG_CRITICAL, 
	/* Use when the engine is in a critical state and logging is to be 
	 * performed as safely as possible. 
	 * Basically should only be used by engine_crash().
	 */
};

/* 
 * Event interface
 *
 * Logging fires EVENT_LOG and passes by pointer the following struct to all 
 * observers. The struct and its pointers become invalid after the callback. 
 * Events are not fired for LOG_CRITICAL.
 */

struct LogEvent {
	enum LogLevel level;
	const char   *message;
};

/* Don't use directly, use the macros below */
void _log(
	enum LogLevel,
	const char *file,
	const char *func,
	int         line,
	const char *format,
	...
);


#define log_info(...) \
	_log(LOG_INFO,     __FILE__, __func__, __LINE__, __VA_ARGS__)

#define log_warn(...) \
	_log(LOG_WARN,     __FILE__, __func__, __LINE__, __VA_ARGS__)
 
#define log_error(...) \
	_log(LOG_ERROR,    __FILE__, __func__, __LINE__, __VA_ARGS__)

#define log_critical(...) \
	_log(LOG_CRITICAL, __FILE__, __func__, __LINE__, __VA_ARGS__)

#ifndef NDEBUG
#define log_debug(...) \
	_log(LOG_DEBUG,    __FILE__, __func__, __LINE__, __VA_ARGS__)
#else 
#define log_debug(...) ((void)0)
#endif
