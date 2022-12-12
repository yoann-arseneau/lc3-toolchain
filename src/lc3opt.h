#pragma once

typedef struct AsmOpt {
	char *callee;
	FILE *input;
	FILE *output;
	VerbosityLevel verbosity;
} AsmOpt;

void asm_opt(int argc, char *argv[], AsmOpt *opt);

