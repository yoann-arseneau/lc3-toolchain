#include "lc3asm.h"
#include "lc3cu.h"

typedef struct LabelNode {
	char *name;
	size_t length;
	uint16_t target;
	struct LabelNode *next;
} LabelNode;

typedef struct LateLinkingNode {
	LateLinkingType type;
	uint16_t address;
	char *name;
	size_t length;
	struct LateLinkingNode *next;
} LateLinkingNode;

static char* malloc_string_copy(const char *start, size_t length) {
	char *buffer = malloc(sizeof(length));
	if (!buffer) {
		fputs("ran out of memory!\n", stderr);
		exit(FAILURE_MEMORY);
	}
	for (size_t i = 0; i < length; ++i) {
		buffer[i] = start[i];
	}
	return buffer;
}

static void ensure_capacity(CompilationUnit *CU, size_t size) {
	if (!CU->origin_set) {
		fputs("ensure_capacity: origin not set\n", stderr);
		exit(FAILURE_INTERNAL);
	}
	else if (size > 0xFFFF) {
		fputs("ensure_capacity: size too large", stderr);
		exit(FAILURE_INTERNAL);
	}

	size_t buffer_remaining = CU->buffer_size - CU->buffer_offset;
	if (buffer_remaining >= size) {
		// enough capacity
		return;
	}

	size_t addr_remaining = 0x10000 - CU->origin - CU->buffer_offset;
	if (addr_remaining < size) {
		fputs("address space overflow", stderr);
		exit(FAILURE_INTERNAL);
	}

	size_t new_size = CU->buffer_size * 2;
	if (new_size < 32) {
		new_size = 32;
	}
	if (new_size > addr_remaining) {
		new_size = addr_remaining;
	}

	CU->buffer = realloc(CU->buffer, new_size);
}
static void pad(CompilationUnit *CU, uint16_t word, size_t size) {
	while (size-- > 0) {
		CU->buffer[CU->buffer_offset++] = word;
	}
}

// Emit
void cu_align_to(CompilationUnit *CU, size_t alignment) {
	if (alignment < 1) {
		fputs("alignment must be >= 1\n", stderr);
		exit(FAILURE_INTERNAL);
	}

	size_t padding = CU->buffer_offset % alignment;
	if (padding > 0) {
		ensure_capacity(CU, padding);
		pad(CU, 0, padding);
	}
}

void cu_emit_word(CompilationUnit *CU, uint16_t word) {
	ensure_capacity(CU, 1);
	CU->buffer[CU->buffer_offset++] = word;
}
void cu_emit_words(CompilationUnit *CU, const uint16_t *words, size_t size) {
	if (size < 1) {
		fputs("words size must be >= 1\n", stderr);
		exit(FAILURE_INTERNAL);
	}

	while (size-- > 0) {
		CU->buffer[CU->buffer_offset++] = *words++;
	}
}
void cu_emit_bytes(CompilationUnit *CU, const uint8_t *bytes, size_t size) {
	if (size < 1) {
		fputs("words size must be >= 1\n", stderr);
		exit(FAILURE_INTERNAL);
	}

	while (size-- > 0) {
		CU->buffer[CU->buffer_offset++] = *bytes++;
	}
}
void cu_emit_padding(CompilationUnit *CU, uint16_t word, size_t size) {
	if (size < 1) {
		fputs("padding size must be >= 1\n", stderr);
		exit(FAILURE_INTERNAL);
	}

	pad(CU, word, size);
}

// Linking
void cu_register_label(CompilationUnit *CU, const char *name, size_t length, uint16_t target) {
	if (length < 1) {
		fputs("cu_register_label: length < 1", stderr);
		exit(FAILURE_INTERNAL);
	}

	LabelNode *node = malloc(sizeof(LabelNode));
	if (!node) {
		fputs("ran out of memory!", stderr);
		exit(FAILURE_MEMORY);
	}
	node->name = malloc_string_copy(name, length);
	node->length = length;
	node->target = target;
	node->next = NULL;

	LabelNode **cursor = &CU->first_label;
	while (*cursor) {
		// FIXME check for duplicates
		cursor = &(*cursor)->next;
	}
	*cursor = node;
}
bool cu_label_get_target(CompilationUnit *CU, const char *name, size_t length, uint16_t *target) {
	if (length < 1) {
		fputs("cu_label_get_offset: length < 1", stderr);
		exit(FAILURE_INTERNAL);
	}

	LabelNode *cursor = CU->first_label;
	while (cursor) {
		if (cursor->length == length && memcmp(cursor->name, name, length) == 0) {
			if (target) {
				*target = cursor->target;
			}
			return true;
		}
		cursor = cursor->next;
	}
	return false;
}
void cu_late_link(CompilationUnit *CU, uint16_t address, LateLinkingType type, const char *name, size_t length) {
	if (length < 1) {
		fputs("cu_register_label: length < 1", stderr);
		exit(FAILURE_INTERNAL);
	}

	LateLinkingNode *node = malloc(sizeof(LateLinkingNode));
	if (!node) {
		fputs("ran out of memory!", stderr);
		exit(FAILURE_MEMORY);
	}
	node->type = type;
	node->address = address;
	node->name = malloc_string_copy(name, length);
	node->length = length;
	node->next = NULL;

	LateLinkingNode **cursor = &CU->first_late_linking;
	while (*cursor) {
		// FIXME check for duplicates
		*cursor = (*cursor)->next;
	}
	*cursor = node;
}
bool cu_resolve_linking(CompilationUnit *CU) {
	LateLinkingNode **cursor = &CU->first_late_linking;
	while (true) {
		LateLinkingNode *current = *cursor;
		if (current == NULL) {
			break;
		}
		uint16_t address = current->address;
		if (address < CU->origin) {
			fprintf(
				stderr,
				"%s: patch address (x%04X) less than origin (x%04X)",
				__func__,
				address,
				CU->origin);
			exit(FAILURE_INTERNAL);
		}
		size_t index = address - CU->origin;
		if (index >= CU->buffer_offset) {
			fprintf(
				stderr,
				"%s: patch address (x%04X) greater than buffer position (x%04zX)",
				__func__,
				address,
				CU->origin + CU->buffer_offset);
			exit(FAILURE_INTERNAL);
		}
		uint16_t target;
		if (cu_label_get_target(CU, current->name, current->length, &target)) {
			uint16_t word = CU->buffer[index];
			switch (current->type) {
				case LLT_AbsoluteWord:
					word = target;
					break;
				case LLT_OffsetPlusOneImm9: {
					unsigned long offset = -1L - address + target;
					unsigned long mask = ~0ul << 9;
					if (offset & mask && ~offset & mask) {
						fprintf(stderr, "%s: offset (%zi) does not fit in %u bits\n", __func__, offset, 9);
						exit(FAILURE_LINKING);
					}
					word &= mask;
					word |= offset & ~mask;
					break;
				}
				default:
					fprintf(stderr, "cu_resolve_linking: unrecognized linking type (%u)", current->type);
					exit(FAILURE_INTERNAL);
			}
			CU->buffer[index] = word;

			// unlink and delete current
			LateLinkingNode *next = current->next;
			free(current->name);
			free(current);
			*cursor = next;
		}
		else {
			// move to next
			cursor = &current->next;
		}
	}
	return cursor == &CU->first_late_linking;
}

// Output
static void write_byte(FILE *output, uint8_t byte) {
	fputc(byte, output);
}
static void write_word(FILE *output, uint16_t word) {
	char buffer[] = {
		(char)(word >> 8),
		(char)(word & 0xFF),
	};
	fwrite(buffer, 1, 2, output);
}
static void write_dword(FILE *output, uint32_t dword) {
	char buffer[] = {
		(char)(dword >> 24),
		(char)(dword >> 16),
		(char)(dword >> 8),
		(char)(dword >> 0),
	};
	fwrite(buffer, 1, 4, output);
}
static void write_string(FILE *output, const char *string, size_t length) {
	fwrite(string, 1, length, output);
}

void cu_produce_obj(CompilationUnit *CU, FILE *output) {
	enum {
		TABLEMETA_OFFSET = 16,
		HEADER_SIZE = 32,
	};

	cu_resolve_linking(CU);

	uint32_t data_size = CU->buffer_offset * 2;
	uint32_t label_size = 0;
	uint32_t linking_size = 0;

	// write header
	write_string(output, "LC3OBJ", 6);
	write_byte(output, 0);
	write_byte(output, 1);
	write_word(output, CU->origin);
	write_dword(output, HEADER_SIZE);
	write_word(output, data_size);
	write_dword(output, 0);
	write_dword(output, 0);
	write_dword(output, 0);
	write_dword(output, 0);

	// write data
	for (size_t i = 0; i < CU->buffer_offset; ++i) {
		uint16_t word = CU->buffer[i];
		fputc(word >> 8, output);
		fputc(word & 0xFF, output);
	}

	// write label table
	LabelNode *label = CU->first_label;
	while (label) {
		write_word(output, label->target);
		write_byte(output, label->length);
		write_string(output, label->name, label->length);
		label_size += 3 + label->length;
		label = label->next;
	}

	// write linking table
	LateLinkingNode *lateLinking = CU->first_late_linking;
	while (lateLinking) {
		write_word(output, lateLinking->address);
		write_byte(output, lateLinking->type);
		write_byte(output, lateLinking->length);
		write_string(output, lateLinking->name, lateLinking->length);
		linking_size += 4 + lateLinking->length;
		lateLinking = lateLinking->next;
	}

	// patch table metadata
	fseek(output, TABLEMETA_OFFSET, SEEK_SET);
	write_dword(output, HEADER_SIZE + data_size);
	write_dword(output, label_size);
	write_dword(output, HEADER_SIZE + data_size + label_size);
	write_dword(output, linking_size);
}

// Config
bool cu_origin_set(CompilationUnit *CU, uint16_t origin) {
	if (CU->origin_set) {
		return false;
	}

	CU->origin_set = true;
	CU->origin = origin;
	CU->buffer_offset = 0;
	return true;
}
uint16_t cu_cursor_get(const CompilationUnit *CU) {
	if (!CU->origin_set) {
		fputs("cu_cursor_get: origin not set\n", stderr);
		exit(FAILURE_INTERNAL);
	}

	return CU->origin + CU->buffer_offset;
}

