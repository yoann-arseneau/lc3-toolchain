#include "lc3asm.h"
#include "lc3opt.h"

void asm_opt(int argc, char *argv[], AsmOpt *opt) {
	if (argc < 1) {
		fputs("no callee?!", stderr);
		exit(FAILURE_INTERNAL);
	}

	memset(opt, 0, sizeof(*opt));
	opt->callee = argv[0];

	int i;
	for (i = 1; i < argc; ++i) {
		char *arg = argv[i];
		if (strcmp(arg, "--") == 0) {
			// argument '--' transitions to file name processing
			i += 1;
			break;
		}
		else if (arg[0] != '-') {
			// argument not starting in '-' is a file name
			break;
		}

		size_t position;
		union {
			size_t size;
		} data;

		if (sscanf(arg, "-v%zu%zn", &data.size, &position) == 1 && arg[position] == 0) {
			size_t level = data.size;
			if (level >= VL_CountPlusOne) {
				fprintf(stderr, "bad versobity level %zu\n", level);
				exit(FAILURE_ARGS);
			}
			opt->verbosity = (VerbosityLevel)(level + 1);
		}
		else {
			fprintf(stderr, "unrecognized argument '%s'\n", arg);
			exit(FAILURE_ARGS);
		}
	}
	for (; i < argc; ++i) {
		char *arg = argv[i];
		if (opt->input == NULL) {
			FILE *fh = fopen(arg, "r");
			if (!fh) {
				fprintf(stderr, "could not open file \"%s\"\n", arg);
				exit(FAILURE_ARGS);
			}
			opt->input = fh;
		}
		else {
			fputs("multiple filenames not supported yet.\n", stderr);
			exit(FAILURE_NOTIMPLEMENTED);
		}
	}

	if (!opt->input) {
		opt->input = stdin;
	}
	if (!opt->output) {
		opt->output = stdout;
	}
	if (!opt->verbosity) {
		opt->verbosity = VL_Warn;
	}
}

