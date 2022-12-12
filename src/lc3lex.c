#include "lc3asm.h"
#include "lc3lex.h"

static int is_whitespace(char c) {
	return c == ' ' || c == '\t';
}
static int is_identifier(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

char* next_lexeme(char **rest) {
	char *cursor;
	char *start = NULL;
	char c;
	cursor = *rest;
	while (true) {
		c = *cursor;
		if (c == 0) {
			*rest = cursor;
			return *rest;
		}
		else if (is_whitespace(c)) {
			cursor += 1;
			continue;
		}
		else {
			start = cursor;
			break;
		}
	}
	
	if (c == '.' || is_identifier(c)) {
		char *cursor = start;
		while (is_identifier(*++cursor)) {
			// scan for first non-identifier
		}
		*rest = cursor;
		return start;
	}
	else if (c == ',') {
		*rest = cursor + 1;
		return start;
	}
	else if (c == '#') {
		char *cursor = start + 1;
		c = *cursor;
		if (c == '-' || c == '+') {
			cursor += 1;
		}
		while (true) {
			c = *cursor;
			if (c < '0' || c > '9') {
				break;
			}
			cursor += 1;
		}
		*rest = cursor;
		return start;
	}
	else if (c == ';') {
		char *end = start + 1;
		while (*end) {
			end += 1;
		}
		*rest = end;
		return start;
	}
	else if (c == '"' || c == '\'') {
		char quote = c;
		char *cursor = start + 1;
		while (true) {
			c = *cursor;
			if (c == 0) {
				// unterminated string constant
				*rest = start;
				return NULL;
			}
			if (c == '\\') {
				// skip escape sequences
				if (!*(cursor + 1)) {
					// unterminated string constant
					*rest = start;
					return NULL;
				}
				cursor += 2;
			}
			else if (c == quote) {
				// closing quote
				break;
			}
			else {
				// literal
				cursor += 1;
			}
		}
		// rest after closing quote
		*rest = cursor + 1;
		return start;
	}
	else if (c == '\'') {
		fputs("\nnot implemented: lexeme char\n", stderr);
		exit(FAILURE_NOTIMPLEMENTED);
	}
	else {
		*rest = start;
		return NULL;
	}
}

