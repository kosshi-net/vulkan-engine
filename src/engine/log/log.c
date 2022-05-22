#include "log/log.h"
#include "engine.h"
#include "common.h"
#include "event/event.h"

#include <stdarg.h>

static const char level_chars[] = {
	[LOG_DEBUG]    = ' ',
	[LOG_INFO]     = 'i',
	[LOG_WARN]     = 'w',
	[LOG_ERROR]    = 'E',
	[LOG_CRITICAL] = 'X',
};

void _log(
	enum LogLevel   level,
	const char     *file,
	const char     *func,
	int             line,
	const char     *format,
	...
){
	va_list args;
	va_start(args, format);

	const char path_prefix[] = "/src/";

	char *sub = strstr(file, path_prefix);
	if (sub) 
		file = sub + sizeof(path_prefix) - 1;

	static char buffer[1024];

	int len = snprintf(buffer, sizeof(buffer), 
		"(%c) %s:%i ", 
		level_chars[level], file, line
	);

	while( len < 36 ) {
		buffer[len] = '.';
		len++;
	}
	buffer[len++] = ' ';
	
	vsnprintf(buffer+len, sizeof(buffer)-len, format, args);
	va_end(args);

	printf("%s\n", buffer);

	if (level != LOG_CRITICAL) {
		static struct LogEvent e;
		e.message = buffer,
		e.level = level,
		event_fire(EVENT_LOG, &e);
	}
}

