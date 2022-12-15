#include "lc3asm.h"
#include "lc3tok.h"

typedef struct IdentifierMeta {
	const char *name;
	TokenType type;
	TokenData data;
} IdentifierMeta;
static IdentifierMeta Identifiers[] = {
	{ "add", TT_Instruction, { TDT_InstructionMeta, .instruction_meta = { IF_Arithmetic, 0x1000 } } },
	{ "and", TT_Instruction, { TDT_InstructionMeta, .instruction_meta = { IF_Arithmetic, 0x5000 } } },
	{ "getc", TT_WordLiteral, { TDT_Word, .word = 0xF020 } },
	{ "halt", TT_WordLiteral, { TDT_Word, .word = 0xF025 } },
	{ "in", TT_WordLiteral, { TDT_Word, .word = 0xF023 } },
	{ "lea", TT_Instruction, { TDT_InstructionMeta, .instruction_meta = { IF_DestOffset, 0xE000 } } },
	{ "not", TT_Instruction, { TDT_InstructionMeta, .instruction_meta = { 0, 0x9000 } } },
	{ "out", TT_WordLiteral, { TDT_Word, .word = 0xF021 } },
	{ "puts", TT_WordLiteral, { TDT_Word, .word = 0xF022 } },
	{ "putsp", TT_WordLiteral, { TDT_Word, .word = 0xF024 } },
	{ "r0", TT_Register, { TDT_Integer, .integer = 0 } },
	{ "r1", TT_Register, { TDT_Integer, .integer = 1 } },
	{ "r2", TT_Register, { TDT_Integer, .integer = 2 } },
	{ "r3", TT_Register, { TDT_Integer, .integer = 3 } },
	{ "r4", TT_Register, { TDT_Integer, .integer = 4 } },
	{ "r5", TT_Register, { TDT_Integer, .integer = 5 } },
	{ "r6", TT_Register, { TDT_Integer, .integer = 6 } },
	{ "r7", TT_Register, { TDT_Integer, .integer = 7 } },
	{ "ret", TT_WordLiteral, { TDT_InstructionMeta, .instruction_meta = { 0, 0xC1C0 } } },
	{ NULL, 0, { TDT_Void, { 0 } } },
};
static IdentifierMeta Directives[] = {
	{ "org", TT_Directive, { TDT_DirectiveType, .directive_type = DT_Origin } },
	{ "stringz", TT_Directive, { TDT_DirectiveType, .directive_type = DT_StringZ } },
	{ NULL, TT_Directive, { TDT_DirectiveType, { 0 } } },
};

static int is_identifier_begin(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static size_t unescape_char(const char *str, size_t length, char *result);
static char *allocate_stringliteral(const char *cursor, size_t length);
static int strnicmp(const char *lhs, const char *rhs, size_t maxlen);

TokenType parse(const char *lexeme, size_t length, TokenData *tokenData) {
	char c = lexeme[0];
	if (is_identifier_begin(c)) {
		for (IdentifierMeta *metaCursor = Identifiers; metaCursor->name; ++metaCursor) {
			if (strnicmp(metaCursor->name, lexeme, length) == 0) {
				*tokenData = metaCursor->data;
				return metaCursor->type;
			}
		}
		if (length >= 2 && lexeme[0] == 'x') {
			for (size_t i = 1; i < length; ++i) {
				c = lexeme[i];
				if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
					return TT_Identifier;
				}
			}
			tokenData->dataType = TDT_StringSlice;
			tokenData->string_slice = (StringSlice){ lexeme + 1, length - 1 };
			return TT_HexIdentifier;
		}
		tokenData->dataType = TDT_StringSlice;
		tokenData->string_slice = (StringSlice){ lexeme, length };
		return TT_Identifier;
	}
	else if (c == ',') {
		tokenData->dataType = TDT_Void;
		return TT_Comma;
	}
	else if (c == '#') {
		char *end;
		long int integer = strtol(lexeme + 1, &end, 10);
		if (end != lexeme + length) {
			tokenData->dataType = TDT_StringSlice;
			tokenData->string_slice = (StringSlice){ lexeme, length };
			return TT_Invalid;
		}
		tokenData->dataType = TDT_Integer;
		tokenData->integer = integer;
		return TT_Number;
	}
	else if (c == '.') {
		for (IdentifierMeta *metaCursor = Directives; metaCursor->name; ++metaCursor) {
			if (strnicmp(metaCursor->name, lexeme + 1, length - 1) == 0) {
				*tokenData = metaCursor->data;
				return TT_Directive;
			}
		}
		tokenData->dataType = TDT_StringSlice;
		tokenData->string_slice = (StringSlice){ lexeme, length };
		return TT_Invalid;
	}
	else if (c == '"') {
		size_t i;
		for (i = 1; i < length - 1; ++i) {
			if (lexeme[i] == '\\') {
				char *string = allocate_stringliteral(lexeme + 1, length - 2);
				if (string) {
					tokenData->dataType = TDT_StringOwned;
					tokenData->string = allocate_stringliteral(lexeme + 1, length - 2);
					return TT_String;
				}
				else {
					tokenData->dataType = TDT_StringSlice;
					tokenData->string_slice = (StringSlice){ lexeme, length };
					return TT_Invalid;
				}
			}
		}
		tokenData->dataType = TDT_StringSlice;
		tokenData->string_slice = (StringSlice){ lexeme + 1, length - 2 };
		return TT_String;
	}
	else if (c == '\'') {
		char c;
		size_t count = unescape_char(lexeme + 1, length - 2, &c);
		if (count != length - 2) {
			tokenData->dataType = TDT_StringSlice;
			tokenData->string_slice = (StringSlice){ lexeme, length };
			return TT_Invalid;
		}
		else {
			tokenData->dataType = TDT_Character;
			tokenData->character = c;
			return TT_Character;
		}
	}
	else if (c == ';') {
		tokenData->dataType = TDT_StringSlice;
		tokenData->string_slice = (StringSlice){ lexeme + 1, length - 1 };
		return TT_Comment;
	}
	else {
		tokenData->dataType = TDT_StringSlice;
		tokenData->string_slice = (StringSlice){ lexeme, length };
		return TT_Invalid;
	}
}

static char* allocate_stringliteral(const char *readPtr, size_t length) {
	char *buffer = malloc(length);
	if (!buffer) {
		fputs("ran out of memory!", stderr);
		exit(FAILURE_MEMORY);
	}
	char *writePtr = buffer;
	size_t i = 0;
	while (i < length) {
		char c;
		size_t count = unescape_char(readPtr + i, length - i, &c);
		if (count < 1) {
			free(buffer);
			return NULL;
		}
		i += count;
		*writePtr++ = c;
	}
	*writePtr = 0;
	return buffer;
}
static size_t unescape_char(const char *str, size_t length, char *result) {
	if (length < 1) {
		return -1;
	}

	size_t size = 0;
	char c = str[size++];
	if (c == '\\') {
		if (length < 2) {
			return -1;
		}

		switch (str[size++]) {
			case 'n':
				c = '\n';
				break;
			case 'r':
				c = '\r';
				break;
			case '\'':
				c = '\'';
				break;
			case '"':
				c = '"';
				break;
			case 't':
				c = '\t';
				break;
			case '0':
				c = '\0';
				break;
			default:
				return -1;
		}
	}
	*result = c;
	return size;
}

static int strnicmp(const char *lhs, const char *rhs, size_t maxlen) {
	for (; maxlen > 0; --maxlen) {
		char clhs = *lhs++;
		char crhs = *rhs++;
		int diff = tolower(crhs) - tolower(clhs);
		if (diff != 0 || !clhs) {
			return diff;
		}
	}
	return 0;
}

StringSlice tokendata_expect_string(TokenData *tokenData) {
	switch (tokenData->dataType) {
		case TDT_String:
		case TDT_StringOwned:
			return (StringSlice){ tokenData->string, strlen(tokenData->string) };
		case TDT_StringSlice:
			return tokenData->string_slice;
		default:
			fprintf(stderr, "tokendata_expect_string: expecting string-type; found (%u)\n", tokenData->dataType);
			exit(FAILURE_INTERNAL);
	}
}
void free_tokendata(TokenData *tokenData) {
	if (tokenData == NULL) {
		return;
	}

	switch (tokenData->dataType) {
		case TDT_PointerOwned:
			free(tokenData->pointer);
			break;
		case TDT_StringOwned:
			free(tokenData->string);
			break;
		default:
			break;
	}
	memset(tokenData, 0, sizeof(*tokenData));
}

