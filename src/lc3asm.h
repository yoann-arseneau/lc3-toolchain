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
	VL_Brief,
	VL_Verbose,
	VL_Debug,
	VL_Trace,
} VerbosityLevel;

extern VerbosityLevel g_verbosity;

#define LOGF(level, ...) do {\
	if (g_verbosity >= (level)) {\
		fprintf(stderr, __VA_ARGS__);\
	}\
} while (false)
#define LOGF_BRIEF(...) LOGF(VL_Brief, __VA_ARGS__)
#define LOGF_VERBOSE(...) LOGF(VL_Verbose, __VA_ARGS__)
#define LOGF_DEBUG(...) LOGF(VL_Debug, __VA_ARGS__)
#define LOGF_TRACE(...) LOGF(VL_Trace, __VA_ARGS__)

#endif//__LC3ASM_H__

