#pragma once

typedef enum VerbosityLevel {
	VL_Quiet = 1,
	VL_Fatal,
	VL_Error,
	VL_Warn,
	VL_Info,
	VL_Debug,
	VL_Trace,
	VL_CountPlusOne,
} VerbosityLevel;

void log_init(void);
bool log_tryparse_verbosity(const char *string, VerbosityLevel *level);
void log_config(VerbosityLevel level, FILE *target);
void log_printf(VerbosityLevel level, const char *file, int line, const char *format, ...);
bool log_enabled(VerbosityLevel level);

#define LOGF(level, ...) do {\
	log_printf((level), __FILE__, __LINE__, __VA_ARGS__);\
} while (false)

#define LOGF_FATAL(...) LOGF(VL_Fatal, __VA_ARGS__)
#define LOGF_ERROR(...) LOGF(VL_Error, __VA_ARGS__)
#define LOGF_WARN(...)  LOGF(VL_Warn,  __VA_ARGS__)
#define LOGF_INFO(...)  LOGF(VL_Info,  __VA_ARGS__)
#define LOGF_DEBUG(...) LOGF(VL_Debug, __VA_ARGS__)
#define LOGF_TRACE(...) LOGF(VL_Trace, __VA_ARGS__)

#define FAILF(code, ...) do {\
	LOGF_FATAL(__VA_ARGS__);\
	exit((code));\
} while (false)

