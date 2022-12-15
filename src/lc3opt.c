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

		if (arg[1] == 0) {
			// not a valid argument; could be a filename?
			break;
		}
		else if (arg[1] == '-') {
			// long-form argument can be `--option` or `--option=value`
			fprintf(stderr, "long-form argument not implemented (%s)\n", arg);
			exit(FAILURE_NOTIMPLEMENTED);
		}
		else {
			switch (arg[1]) {
				case 'v': {
					char c = arg[2];
					if (c == 0) {
						c = '3';
					}
					else if (arg[3] != 0 || c < '0' || c > '0' + VL_CountPlusOne - 2) {
						fprintf(
							stderr,
							"option -v accepts no value or value in range [0 .. %u]; got (%s)\n",
							VL_CountPlusOne - 2,
							arg);
						exit(FAILURE_ARGS);
					}
					opt->verbosity = (VerbosityLevel)(c - '0' + 1);
					break;
				default:
					fprintf(stderr, "unrecognized argument '%s'\n", arg);
					exit(FAILURE_ARGS);
				}
			}
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

