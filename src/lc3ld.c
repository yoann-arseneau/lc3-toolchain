#include "lc3asm.h"

void options(int argc, char *argv[]);

int main(int argc, char *argv[]) {
	puts("lc3ld not implemented\n", stderr);
	exit(FAILURE_NOTIMPLEMENTED);
}

void options(int argc, char *argv[]) {
	if (argc < 1) {
		fputs("no callee?!", stderr);
		exit(FAILURE_INTERNAL);
	}
	VerbosityLevel verbosity = 0;

	int i;
	// process options
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
					verbosity = (VerbosityLevel)(c - '0' + 1);
					break;
				default:
					fprintf(stderr, "unrecognized argument '%s'\n", arg);
					exit(FAILURE_ARGS);
				}
			}
		}
	}
	// process filenames
	for (; i < argc; ++i) {
		char *arg = argv[i];
		if (*input == NULL) {
			FILE *fh = fopen(arg, "r");
			if (!fh) {
				fprintf(stderr, "could not open file \"%s\"\n", arg);
				exit(FAILURE_ARGS);
			}
			*input = fh;
		}
		else {
			fputs("multiple filenames not supported yet.\n", stderr);
			exit(FAILURE_NOTIMPLEMENTED);
		}
	}

	if (!verbosity) {
		verbosity = VL_Warn;
	}
	log_config(verbosity, stderr);
}

static void read(FILE *input, uint8_t* buffer, size_t size, size_t count) {
	size_t read = fread((char*)buffer, size, count, input);
	if (read != count) {
		fprintf(stderr, "expecting %zux%zu; found %zu\n", count, size, read);
		exit(FAILURE_IO);
	}
}
static uint8_t read_byte(FILE *input) {
	uint8_t buffer;
	read(input, &buffer, 1, 1);
	return buffer;
}
static uint16_t read_word(FILE *input) {
	uint8_t buffer[2];
	read(input, buffer, 2, 1);
	return ((uint16_t)buffer[0] << 8)
		| ((uint16_t)buffer[1] << 0);
}
static uint32_t read_dword(FILE *input) {
	uint8_t buffer[4];
	read(input, buffer, 4, 1);
	return ((uint32_t)buffer[0] << 24)
		| ((uint32_t)buffer[1] << 16)
		| ((uint32_t)buffer[2] << 8)
		| ((uint32_t)buffer[3] << 0);
}
static void read_string(FILE *input, char *string, size_t length) {
	read(input, (uint8_t*)string, 1, length);
}

bool cu_read_obj(FILE *input, CompilationUnit *CU) {
	char tag[8];
	size_t size = fread(tag, 1, 8, input);
	if (size != 8 || strncmp(tag, "LC3OBJ", 6) != 0) {
		LOGF_ERROR("not a valid lc3 object file");
		return false;
	}
	uint8_t major = tag[6];
	uint8_t minor = tag[7];
	if (major != 0) {
		LOGF_ERROR("version %hhu.%hhu is not supported", major, minor);
		return false;
	}

	(void)CU;
	fputs("cu_read_obj not implemented\n", stderr);
	exit(FAILURE_NOTIMPLEMENTED);
}

