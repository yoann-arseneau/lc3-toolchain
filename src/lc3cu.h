#pragma once

// == Types ==
typedef enum LateLinkingType {
	LLT_AbsoluteWord = 1,
	LLT_OffsetPlusOneImm9,
} LateLinkingType;

typedef struct CompilationUnit {
	bool origin_set;
	uint16_t origin;
	uint16_t *buffer;
	size_t buffer_size;
	size_t buffer_offset;
	struct LabelNode *first_label;
	struct LateLinkingNode *first_late_linking;
} CompilationUnit;

// == Functions ==
// Validation
void cu_ensurecapacity(CompilationUnit *CU, size_t capacity);

// Emit
void cu_align_to(CompilationUnit *CU, size_t alignment);
void cu_emit_word(CompilationUnit *CU, uint16_t word);
void cu_emit_words(CompilationUnit *CU, const uint16_t* words, size_t size);
void cu_emit_bytes(CompilationUnit *CU, const uint8_t *bytes, size_t size);
void cu_emit_padding(CompilationUnit *CU, uint16_t word, size_t count);

// Linking
void cu_register_label(CompilationUnit *CU, const char *name, size_t length, uint16_t target);
bool cu_label_get_target(CompilationUnit *CU, const char *name, size_t length, uint16_t *target);
void cu_late_link(CompilationUnit *CU, uint16_t address, LateLinkingType type, const char *name, size_t length);
bool cu_resolve_linking(CompilationUnit *CU);

// Output
void cu_produce_obj(CompilationUnit *CU, FILE *output);

// Config
bool cu_origin_set(CompilationUnit *CU, uint16_t origin);
uint16_t cu_cursor_get(const CompilationUnit *CU);

