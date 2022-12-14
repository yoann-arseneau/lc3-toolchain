#include "lc3asm.h"
typedef struct Options {
	FILE *input;
	FILE *output;
	VerbosityLevel verbosity;
} Options;

typedef struct Token {
	TokenType type;
	TokenData data;
} Token;
typedef struct Argument {
	Token *tokens;
	size_t count;
} Argument;
typedef struct Line {
	Token *statement;
	Argument *args;
	size_t nArgs;
	Token *label;
	Token *comment;
} Line;

void parse_options(int argc, char *argv[], Options *options);
void assemble(FILE *input, FILE *output);

int main(int argc, char *argv[]) {
	log_init();

	Options options;
	parse_options(argc, argv, &options);

	if (!options.input) {
		options.input = stdin;
	}
	if (!options.output) {
		options.output = stdout;
	}
	if (options.verbosity) {
		log_config(options.verbosity, stderr);
	}

	LOGF_TRACE("assemble start");
	assemble(options.input, options.output);
	LOGF_TRACE("assemble complete");

	LOGF_TRACE("cleanup");
	if (options.input != stdin) {
		fclose(options.input);
	}
	if (options.output != stdout) {
		fclose(options.output);
	}
	LOGF_TRACE("exit normal");
}

void parse_options(int argc, char *argv[], Options *options) {
	if (argc < 1) {
		FAILF(FAILURE_INTERNAL, "no callee?!");
	}
	memset(options, 0, sizeof(*options));

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
			FAILF(FAILURE_NOTIMPLEMENTED, "long-form argument not implemented (%s)\n", arg);
		}
		else {
			switch (arg[1]) {
				case 'v': {
					VerbosityLevel level;
					if (!log_tryparse_verbosity(&arg[2], &level)) {
						if (arg[2] == 0) {
							level = VL_Info;
						}
						else {
							FAILF(
								FAILURE_ARGS,
								"option -v accepts no value or value in range [0 .. %u]; got (%s)\n",
								VL_CountPlusOne - 2,
								arg);
						}
					}
					options->verbosity = level;
					break;
				}
				default:
					FAILF(FAILURE_ARGS, "unrecognized argument '%s'\n", arg);
			}
		}
	}
	// process filenames
	for (; i < argc; ++i) {
		char *arg = argv[i];
		if (options->input == NULL) {
			FILE *fh = fopen(arg, "r");
			if (!fh) {
				fprintf(stderr, "could not open file \"%s\"\n", arg);
				exit(FAILURE_ARGS);
			}
			options->input = fh;
		}
		else {
			fputs("multiple filenames not supported yet.\n", stderr);
			exit(FAILURE_NOTIMPLEMENTED);
		}
	}
}

int readline(FILE *file, char *buffer, size_t capacity);
void free_token(Token *token);
void process_line(CompilationUnit *CU, size_t line_number, Token *token, size_t nTokens);
void assemble(FILE *input, FILE *output) {
	enum {
		MAX_LINE_CHARS = 4096,
		MAX_LINE_TOKENS = 8,
	};

	char* line_chars = malloc(MAX_LINE_CHARS);
	Token* line_tokens = malloc(MAX_LINE_TOKENS * sizeof(Token));
	size_t line_number = 0;

	if (!line_chars || !line_tokens) {
		fputs("ran out of memory!\n", stderr);
		exit(FAILURE_MEMORY);
	}

	CompilationUnit CU = {0};

	LOGF_INFO("assemble");
	LOGF_TRACE("file read");
	while (!feof(input)) {
		LOGF_TRACE("line read");
		readline(input, line_chars, MAX_LINE_CHARS);
		if (ferror(input)) {
			fputs("error while reading file", stderr);
			exit(FAILURE_IO);
		}
		line_number += 1;
		char *cursor = line_chars;
		size_t nTokens = 0;
		LOGF_TRACE("line parse");
		while (true) {
			char *lexeme = next_lexeme(&cursor);
			if (lexeme == NULL) {
				lexeme = cursor; // lexeme is used to locate error in syntax_error
				goto syntax_error;
			}
			else if (lexeme == cursor) {
				// line is complete
				break;
			}
			else {
				Token *token = &line_tokens[nTokens++];
				token->type = parse(lexeme, cursor - lexeme, &token->data);
				if (token->type == TT_Invalid) {
					goto syntax_error;
				}
			}

			if (nTokens > MAX_LINE_TOKENS) {
				fprintf(stderr, "too many tokens on line %zu\n", line_number);
				exit(FAILURE_LIMITS);
			}
			continue;

		syntax_error:
			fprintf(
				stderr,
				"syntax error at offset %u:\n%s\n%*c\n",
				(unsigned)(lexeme - line_chars),
				line_chars,
				(signed)(lexeme - line_chars + 1),
				'^');
			exit(FAILURE_SYNTAX);
		}
		LOGF_TRACE("line process");
		process_line(&CU, line_number, line_tokens, nTokens);
		LOGF_TRACE("line cleanup");
		for (size_t i = 0; i < nTokens; ++i) {
			free_token(&line_tokens[i]);
		}
	}

	LOGF_TRACE("file cleanup");
	free(line_chars);
	free(line_tokens);

	if (CU.origin_set) {
		LOGF_INFO("produce obj");
		cu_produce_obj(&CU, output);
		exit(EXIT_SUCCESS);
	}
	else {
		fputs("no code found!\n", stderr);
		exit(FAILURE_SYNTAX);
	}
}

void process_instruction(CompilationUnit *CU, Line *line);
void process_word_literal(CompilationUnit *CU, Line *line);
void process_directive(CompilationUnit *CU, Line *line);
void process_line(CompilationUnit *CU, size_t line_number, Token *tokens, size_t nTokens) {
	LOGF_TRACE("process line %zu (tokens: %p; nTokens: %zu)", line_number, (void*)tokens, nTokens);

	enum {
		MAX_ARGUMENTS = 5,
	};

	Line line = {0};
	Argument args[MAX_ARGUMENTS] = {0};

	// ignore empty lines
	if (nTokens == 0) {
		LOGF_TRACE("line empty");
		return;
	}

	// handle comment
	if (tokens[nTokens - 1].type == TT_Comment) {
		line.comment = &tokens[nTokens - 1];
		nTokens -= 1;
	}
	if (nTokens == 0) {
		LOGF_TRACE("line empty");
		return;
	}

	// handle label
	if (tokens[0].type == TT_Identifier) {
		line.label = &tokens[0];
		tokens += 1;
		nTokens -= 1;
		if (nTokens == 0) {
			fprintf(stderr, "dangling label on line %zu\n", line_number);
			exit(FAILURE_SYNTAX);
		}
	}

	// handle statement
	line.statement = &tokens[0];
	tokens += 1;
	nTokens -= 1;

	// handle arguments
	LOGF_TRACE("arguments");
	if (nTokens > 0) {
		if (tokens[nTokens - 1].type == TT_Comma) {
			fputs("dangling comma\n", stderr);
			exit(FAILURE_SYNTAX);
		}
		line.args = args;
		size_t nArgs = 0;
		Token *last = tokens;
		for (size_t i = 0; i < nTokens; ++i) {
			if (tokens[i].type == TT_Comma) {
				if (nArgs >= MAX_ARGUMENTS) {
					break;
				}
				line.args[nArgs++] = (Argument){ last, &tokens[i] - last };
				last = &tokens[i + 1];
			}
		}
		if (nArgs >= MAX_ARGUMENTS) {
			fputs("too many arguments on one line\n", stderr);
			exit(FAILURE_LIMITS);
		}
		line.args[nArgs++] = (Argument){ last, &tokens[nTokens] - last };
		line.nArgs = nArgs;
	}
	else {
		line.args = NULL;
		line.nArgs = 0;
	}

	// process line
	switch (line.statement->type) {
		case TT_Instruction:
			process_instruction(CU, &line);
			break;
		case TT_WordLiteral:
			process_word_literal(CU, &line);
			break;
		case TT_Directive:
			process_directive(CU, &line);
			break;
		default:
			fprintf(stderr, "expecting instruction or directive on line %zu\n", line_number);
			exit(FAILURE_SYNTAX);
			break;
	}
}
static void expect_n_args(size_t expected, size_t actual);
static int expect_reg(Argument *arg, const char *name);
static long expect_off(CompilationUnit *CU, Argument *arg, size_t bits, const char *name);
static int try_reg(Argument *arg);
static int try_imm(Argument *arg, size_t bits, bool errorOnTooLarge);
static bool validate_imm(long number, size_t nBits);
void emit_preamble(CompilationUnit *CU, size_t alignment, Token *label);
void process_instruction(CompilationUnit *CU, Line *line) {
	LOGF_TRACE("instruction");
	Token *instruction = line->statement;
	Argument *args = line->args;
	size_t nArgs = line->nArgs;
	if (instruction->data.dataType != TDT_InstructionMeta) {
		fprintf(stderr, "expecting instruction; got (%u)\n", instruction->data.dataType);
		exit(FAILURE_INTERNAL);
	}
	InstructionMeta *meta = &instruction->data.instruction_meta;
	uint16_t word = meta->instruction_mask;
	emit_preamble(CU, 1, line->label);
	switch (meta->format) {
		case IF_Arithmetic: {
			expect_n_args(3, nArgs);
			int dest = expect_reg(&args[0], "first");
			int lhs = expect_reg(&args[1], "second");
			int rhs = try_reg(&args[2]);
			if (rhs < 0) {
				rhs = try_imm(&args[2], 5, true);
				if (rhs < 0) {
					fprintf(
						stderr,
						"expecting register or immediate as third argument; found (%u)\n",
						args[2].count == 1 ? args[2].tokens[0].type : (TokenType)0);
					exit(FAILURE_SYNTAX);
				}
				rhs |= 0x20;
			}
			word |= dest << 9;
			word |= lhs << 6;
			word |= rhs;
			break;
		}
		case IF_DestOffset: {
			expect_n_args(2, nArgs);
			int dest = expect_reg(&args[0], "first");
			long offset = expect_off(CU, &args[1], 9, "second");
			word |= dest << 9;
			word |= offset;
			break;
		}
		case IF_Offset9: {
			expect_n_args(1, nArgs);
			long offset = expect_off(CU, &args[0], 9, "first");
			word |= offset;
			break;
		}
		case IF_BaseR: {
			expect_n_args(1, nArgs);
			int dest = expect_reg(&args[0], "first");
			word |= dest << 6;
			break;
		}
		default:
			fprintf(stderr, "unrecognized instruction format (%u)\n", meta->format);
			exit(FAILURE_INTERNAL);
			break;
	}
	cu_emit_word(CU, word);
}
static void expect_n_args(size_t expected, size_t actual) {
	if (actual != expected) {
		fprintf(stderr, "expecting %zu arguments; found %zu\n", expected, actual);
		exit(FAILURE_SYNTAX);
	}
}
static int expect_reg(Argument* arg, const char *name) {
	int index = try_reg(arg);
	if (index < 0) {
		fprintf(stderr, "expecting register as %s argument\n", name);
		exit(FAILURE_SYNTAX);
	}
	return index;
}
static long expect_off(CompilationUnit *CU, Argument *arg, size_t nBits, const char *name) {
	long offset = try_imm(arg, nBits, true);
	if (offset < 0) {
		if (arg->count == 1 && arg->tokens[0].type == TT_Identifier) {
			StringSlice slice = tokendata_expect_string(&arg->tokens[0].data);
			uint16_t target;
			if (cu_label_get_target(CU, slice.start, slice.length, &target)) {
				offset = 1L + target - cu_cursor_get(CU);
				if (!validate_imm(offset, nBits)) {
					fprintf(
						stderr,
						"offset for label %.*s (%li) does not fit in %zu bits\n",
						(unsigned)slice.length,
						slice.start,
						offset,
						nBits);
					exit(FAILURE_SYNTAX);
				}
			}
			else {
				cu_late_link(CU, cu_cursor_get(CU), LLT_OffsetPlusOneImm9, slice.start, slice.length);
				offset = 0;
			}
		}
		else {
			fprintf(
				stderr,
				"expecting symbol or number as %s argument; found (%u)\n",
				name,
				arg->count == 1 ? arg->tokens[0].type : (TokenType)0);
			exit(FAILURE_SYNTAX);
		}
	}
	return offset & ~(~0ul << nBits);
}
static int try_reg(Argument *arg) {
	if (arg->count == 0) {
		return -1;
	}
	if (arg->count > 1) {
		fputs("multiple token arg\n", stderr);
		exit(FAILURE_NOTIMPLEMENTED);
	}
	Token *token = &arg->tokens[0];
	if (token->type != TT_Register) {
		return -1;
	}
	if (token->data.dataType != TDT_Integer) {
		fprintf(stderr, "unexpected register data type (%u)", token->data.dataType);
		exit(FAILURE_INTERNAL);
	}
	int index = token->data.integer;
	if (index < 0 || index > 7) {
		fprintf(stderr, "register value out of range (%i)\n", index);
		exit(FAILURE_INTERNAL);
	}
	return index;
}
static int try_imm(Argument *arg, size_t nBits, bool errorOnTooLarge) {
	if (arg->count == 0) {
		return -1;
	}
	if (arg->count > 1) {
		fputs("multiple token arg\n", stderr);
		exit(FAILURE_NOTIMPLEMENTED);
	}
	Token *token = &arg->tokens[0];
	if (nBits <= 1 || nBits > 12) {
		fprintf(stderr, "nBits out of range (%zu)\n", nBits);
		exit(FAILURE_INTERNAL);
	}
	if (token->type != TT_Number) {
		return -1;
	}
	if (token->data.dataType != TDT_Integer) {
		fprintf(stderr, "unexpected number data type (%u)", token->data.dataType);
		exit(FAILURE_INTERNAL);
	}
	int number = token->data.integer;
	unsigned long mask = ~0ul << nBits;
	if (number & mask && ~number & mask) {
		if (errorOnTooLarge) {
			fprintf(stderr, "number (%i) does not fit in %zu bits\n", number, nBits);
			exit(FAILURE_SYNTAX);
		}
		else {
			return -1;
		}
	}
	return number & ~mask;
}
static bool try_int(Argument *arg, long *result) {
	if (arg->count == 0) {
		return false;
	}
	if (arg->count > 1) {
		fputs("multiple token arg\n", stderr);
		exit(FAILURE_NOTIMPLEMENTED);
	}
	Token *token = &arg->tokens[0];
	switch (token->type) {
		case TT_Number:
			if (token->data.dataType != TDT_Integer) {
				fprintf(stderr, "unexpected number data type (%u)", token->data.dataType);
				exit(FAILURE_INTERNAL);
			}
			*result = token->data.integer;
			return true;
		case TT_HexIdentifier: {
			StringSlice string = tokendata_expect_string(&token->data);
			char *end;
			unsigned long value = strtoul(string.start, &end, 16);
			if (end != string.start + string.length || value >= 0x7FFFFFFF) {
				fputs("bad hex integer", stderr);
				exit(FAILURE_SYNTAX);
			}
			*result = (long)value;
			return true;
		}
		default:
			return false;
	}
}
static bool validate_imm(long number, size_t nBits) {
	unsigned long mask = ~0ul << nBits;
	return !(number & mask && ~number & mask);
}
void process_word_literal(CompilationUnit *CU, Line *line) {
	LOGF_TRACE("word literal");
	emit_preamble(CU, 1, line->label);
	cu_emit_word(CU, line->statement->data.word);
}
void process_directive(CompilationUnit *CU, Line *line) {
	LOGF_TRACE("directive");

	Token *directive = line->statement;
	Argument *args = line->args;
	size_t nArgs = line->nArgs;
	Token *label = line->label;
	if (directive->data.dataType != TDT_DirectiveType) {
		fprintf(stderr, "expecting directive; got (%u)\n", directive->data.dataType);
		exit(FAILURE_INTERNAL);
	}
	switch (directive->data.directive_type) {
		case DT_Origin: {
			LOGF_TRACE(".origin");
			if (label) {
				fputs(".origin cannot have a label\n", stderr);
				exit(FAILURE_SYNTAX);
			}
			if (nArgs != 1) {
				fputs(".origin expects exactly one argument\n", stderr);
				exit(FAILURE_SYNTAX);
			}
			Argument *arg = args;
			long number;
			if (!try_int(arg, &number)) {
				fputs(".origin expects a number between [0 .. 0xFFFF]\n", stderr);
				exit(FAILURE_SYNTAX);
			}
			if (number < 0 || number > 0xFFFF) {
				fprintf(stderr, ".origin expects a number between between [0 .. 0xFFFF]; got %li\n", number);
				exit(FAILURE_SYNTAX);
			}
			uint16_t origin = (uint16_t)number;
			if (!cu_origin_set(CU, origin)) {
				fputs(".origin was already set\n", stderr);
				exit(FAILURE_SYNTAX);
			}
			break;
		}
		case DT_StringZ: {
			LOGF_TRACE(".stringz");
			if (nArgs != 1) {
				fputs(".stringz expects exactly one argument\n", stderr);
				exit(FAILURE_SYNTAX);
			}
			Argument *arg = args;
			if (arg->count == 0) {
				fputs("empty argument\n", stderr);
				exit(FAILURE_SYNTAX);
			}
			if (arg->count > 1) {
				fputs("multiple token arg\n", stderr);
				exit(FAILURE_NOTIMPLEMENTED);
			}
			StringSlice slice = tokendata_expect_string(&args->tokens[0].data);
			emit_preamble(CU, 1, line->label);
			cu_emit_bytes(CU, (uint8_t*)slice.start, slice.length);
			cu_emit_word(CU, 0);
			break;
		}
		default:
			fprintf(stderr, "unrecognized directive type (%u)\n", directive->data.directive_type);
			exit(FAILURE_INTERNAL);
	}
}
void emit_preamble(CompilationUnit *CU, size_t alignment, Token *label) {
	cu_align_to(CU, alignment);
	if (label) {
		StringSlice slice = tokendata_expect_string(&label->data);
		cu_register_label(CU, slice.start, slice.length, cu_cursor_get(CU));
	}
}

void free_token(Token *token) {
	if (!token) {
		return;
	}

	free_tokendata(&token->data);
}
void print_tokendata(TokenData *data) {
	switch (data->dataType) {
		case TDT_Void:
			break;
		case TDT_Word:
			fprintf(stderr, "%04X", data->word);
			break;
		case TDT_Integer:
			fprintf(stderr, "%ld", data->integer);
			break;
		case TDT_Size:
			fprintf(stderr, "%zu", data->size);
			break;
		case TDT_Pointer:
		case TDT_PointerOwned:
			fprintf(stderr, "%p", data->pointer);
			break;
		case TDT_String:
		case TDT_StringOwned:
			fprintf(stderr, "%s", data->string);
			break;
		case TDT_StringSlice: {
			StringSlice slice = data->string_slice;
			fprintf(stderr, "%.*s", (int)slice.length, slice.start);
			break;
		}
		case TDT_InstructionMeta: {
			InstructionMeta *meta = &data->instruction_meta;
			fprintf(stderr, "%04X(%i)", meta->instruction_mask, meta->format);
			break;
		}
		default:
			fputs("!bad data!", stderr);
			break;
	}
}

int readline(FILE *file, char *buffer, size_t capacity) {
	size_t i = 0;
	while (true) {
		if (i >= capacity) {
			fputs("line too long\n", stderr);
			exit(FAILURE_LIMITS);
		}
		
		int c = fgetc(file);
		if (c < 0) {
			if (!feof(file)) {
				fputs("io error\n", stderr);
				exit(FAILURE_IO);
			}
			break;
		}
		if (c == '\n' || c == '\r') {
			int nc = fgetc(file);
			if (nc >= 0 && (nc == c || (nc != '\n' && nc != '\r'))) {
				ungetc(nc, file);
			}
			break;
		}
		buffer[i++] = c;
	}
	buffer[i] = 0;
	return i;
}

