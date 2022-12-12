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

#endif//__LC3ASM_H__

