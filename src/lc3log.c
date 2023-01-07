#include "lc3std.h"
#include "lc3log.h"

typedef struct VerbosityNameEntry {
	const char *name;
	VerbosityLevel level;
} VerbosityNameEntry;

static VerbosityLevel g_verbosity;
static FILE *g_target;

static VerbosityNameEntry g_verbosity_names[] = {
	{ "quiet", VL_Quiet },
	{ "fatal", VL_Fatal },
	{ "error", VL_Error },
	{ "warn", VL_Warn },
	{ "warning", VL_Warn },
	{ "info", VL_Info },
	{ "information", VL_Info },
	{ "debug", VL_Debug },
	{ "trace", VL_Trace },
	{ NULL, 0 },
};

void log_init(void) {
	char *env = getenv("LC3_VERBOSITY");
	VerbosityLevel level;
	FILE *target = stderr;

	if (!log_tryparse_verbosity(env, &level)) {
		level = VL_Warn;
	}
	log_config(level, target);
}

bool log_tryparse_verbosity(const char *string, VerbosityLevel *level) {
	if (string == NULL) {
		return false;
	}
	else if (string[0] == 0) {
		*level = VL_Info;
		return true;
	}
	else if (string[1] == 0) {
		// single-char
		switch (string[0]) {
			case '0':
			case 'Q':
			case 'q':
				*level = VL_Quiet;
				return true;
			case '1':
			case 'F':
			case 'f':
				*level = VL_Fatal;
				return true;
			case '2':
			case 'E':
			case 'e':
				*level = VL_Error;
				return true;
			case '3':
			case 'W':
			case 'w':
				*level = VL_Warn;
				return true;
			case '4':
			case 'I':
			case 'i':
				*level = VL_Info;
				return true;
			case '5':
			case 'D':
			case 'd':
				*level = VL_Debug;
				return true;
			case '6':
			case 'T':
			case 't':
				*level = VL_Trace;
				return true;
			default:
				return false;
		}
	}
	else {
		VerbosityNameEntry *cursor = g_verbosity_names;
		while (cursor->name) {
			if (stricmp(cursor->name, string) == 0) {
				*level = cursor->level;
				return true;
			}
			cursor += 1;
		}
	}
	return false;
}

bool log_enabled(VerbosityLevel level) {
	if (level <= 0 || level >= VL_CountPlusOne) {
		fprintf(stderr, "bad verbosity level %u", level);
	}

	return g_verbosity <= level;
}

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

static void log_vprintf(VerbosityLevel level, const char *file, int line, const char *format, va_list args) {
	enum { LINE_MAX_SIZE = 128 };

	if (level <= VL_Quiet || level >= VL_CountPlusOne) {
		if (g_verbosity >= VL_Fatal) {
			fprintf(stderr, "bad verbosity level %u\n", level);
		}
		exit(FAILURE_INTERNAL);
	}
	if (level > g_verbosity) {
		return;
	}

	char buffer[LINE_MAX_SIZE];

	// add timestamp
	time_t time_raw;
	struct tm *time_info;
	time(&time_raw);
	time_info = localtime(&time_raw);
	size_t written = strftime(buffer, LINE_MAX_SIZE, "%FT%T", time_info);
	if (written == 0) {
		strncpy(buffer, "???\\?-?\\?-??T??:??:??", LINE_MAX_SIZE);
		written = 19;
		if (written > LINE_MAX_SIZE) {
			written = LINE_MAX_SIZE;
		}
	}

	// add label, file, and line
	const char *label;
	switch (level) {
		case VL_Fatal:
			label = "FATAL";
			break;
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
		case VL_Quiet:
			log_vprintf(VL_Fatal, file, line, format, args);
			exit(FAILURE_INTERNAL);
		default:
			fprintf(stderr, "invalid verbosity level %u\n", level);
			exit(FAILURE_INTERNAL);
	}
	int result = snprintf(
		buffer + written,
		LINE_MAX_SIZE - written,
		" %-5s %s:%u ",
		label,
		file,
		line);
	if (result >= 0) {
		written += result;
	}
	else {
		strncpy(buffer + written, " ????? ?:? ", LINE_MAX_SIZE - written);
		written += 11;
	}
	if (written > LINE_MAX_SIZE) {
		written = LINE_MAX_SIZE;
	}

	// add user message
	result = vsnprintf(buffer + written, LINE_MAX_SIZE - written, format, args);
	if (result >= 0) {
		written += result;
	}
	else {
		strncpy(buffer + written, "<failed to format message>", LINE_MAX_SIZE - written);
		written += 26;
	}
	if (written > LINE_MAX_SIZE) {
		written = LINE_MAX_SIZE;
	}

	// add newline
	if (written + 1 < LINE_MAX_SIZE) {
		buffer[written + 0] = '\n';
		buffer[written + 1] = '\0';
	}

	fputs(buffer, stderr);
	fflush(stderr);
}
void log_printf(VerbosityLevel level, const char *file, int line, const char *format, ...) {
	va_list args;
	va_start(args, format);
	log_vprintf(level, file, line, format, args);
	va_end(args);
}

