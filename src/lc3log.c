#include "lc3asm.h"

static VerbosityLevel g_verbosity;
static FILE *g_target;

void log_config(VerbosityLevel level, FILE *target) {
	if (level < VL_Quiet || level >= VL_CountPlusOne) {
		fprintf(stderr, "bad verbosity level %u\n", level);
		exit(FAILURE_INTERNAL);
	}
	if (target == NULL) {
		fprintf(stderr, "bad  verbosity level %u\n", level);
		exit(FAILURE_INTERNAL);
	}

	g_verbosity = level;
	g_target = target;
}

void log_printf(VerbosityLevel level, const char *file, int line, const char *format, ...) {
	enum { LINE_MAX_SIZE = 128 };

	if (level <= VL_Quiet || level >= VL_CountPlusOne) {
		fprintf(stderr, "bad verbosity level %u\n", level);
		exit(FAILURE_INTERNAL);
	}
	if (level > g_verbosity) {
		return;
	}

	char buffer[LINE_MAX_SIZE];

	time_t time_raw;
	struct tm *time_info;
	time(&time_raw);
	time_info = localtime(&time_raw);
	size_t written = strftime(buffer, LINE_MAX_SIZE, "%FT%T", time_info);
	if (written == 0) {
		fputs("error while logging", stderr);
		exit(FAILURE_INTERNAL);
	}

	const char *label;
	switch (level) {
		case VL_Error:
			label = "ERROR";
			break;
		case VL_Warn:
			label = "WARN";
			break;
		case VL_Info:
			label = "INFO";
			break;
		case VL_Debug:
			label = "DEBUG";
			break;
		case VL_Trace:
			label = "TRACE";
			break;
		default:
			fprintf(stderr, "unexpected verbosity level %u\n", level);
			exit(FAILURE_INTERNAL);
	}
	int result = snprintf(
		buffer + written,
		LINE_MAX_SIZE - written,
		" %-5s %s:%u ",
		label,
		file,
		line);
	if (result < 0) {
		fputs("error while logging", stderr);
		exit(FAILURE_INTERNAL);
	}
	written += result;
	if (written > LINE_MAX_SIZE) {
		written = LINE_MAX_SIZE;
	}

	va_list args;
	va_start(args, format);
	result = vsnprintf(buffer + written, LINE_MAX_SIZE - written, format, args);
	va_end(args);
	if (result < 0) {
		fputs("error while logging", stderr);
		exit(FAILURE_INTERNAL);
	}
	written += result;

	if (written + 1 < LINE_MAX_SIZE) {
		buffer[written + 0] = '\n';
		buffer[written + 1] = '\0';
	}

	fputs(buffer, stderr);
	fflush(stderr);
}

