#ifndef __LC3ASM_H__
#define __LC3ASM_H__

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	FAILURE_ARGS = 1,
	FAILURE_IO = 2,
	FAILURE_LIMITS = 3,
	FAILURE_NOTIMPLEMENTED = 4,
	FAILURE_INTERNAL = 5,
	FAILURE_SYNTAX = 6,
	FAILURE_MEMORY = 7,
	FAILURE_LINKING = 8,
};

typedef enum VerbosityLevel {
	VL_Quiet = 1,
	VL_Error,
	VL_Warn,
	VL_Info,
	VL_Debug,
	VL_Trace,
	VL_CountPlusOne,
} VerbosityLevel;

extern VerbosityLevel g_verbosity;

#define LOGF(level, label, ...) do {\
	if (g_verbosity >= (level)) {\
		fprintf(stderr, "%5s %s(%u) ", label, __FILE__, __LINE__);\
		fprintf(stderr, __VA_ARGS__);\
		fputc('\n', stderr);\
	}\
} while (false)
#define LOGF_ERROR(...) LOGF(VL_Error, "ERROR", __VA_ARGS__)
#define LOGF_WARN(...) LOGF(VL_Warn, "WARN", __VA_ARGS__)
#define LOGF_INFO(...) LOGF(VL_Info, "INFO", __VA_ARGS__)
#define LOGF_DEBUG(...) LOGF(VL_Debug, "DEBUG", __VA_ARGS__)
#define LOGF_TRACE(...) LOGF(VL_Trace, "TRACE", __VA_ARGS__)

#endif//__LC3ASM_H__

