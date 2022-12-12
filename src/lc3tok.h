#pragma once

// Data Types
typedef enum TokenType {
	TT_Invalid = 1,
	TT_Directive,
	TT_Identifier,
	TT_HexIdentifier,
	TT_Instruction,
	TT_WordLiteral,
	TT_Register,
	TT_Number,
	TT_String,
	TT_Character,
	TT_Comma,
	TT_Comment,
} TokenType;

typedef struct StringSlice {
	const char *start;
	size_t length;
} StringSlice;

typedef enum InstructionFormat {
	IF_Arithmetic = 1,
	IF_DestOffset,
} InstructionFormat;
typedef struct InstructionMeta {
	InstructionFormat format;
	uint16_t instruction_mask;
} InstructionMeta;

typedef enum DirectiveType {
	DT_Invalid = 1,
	DT_Origin,
	DT_StringZ,
} DirectiveType;

typedef enum TokenDataType {
	TDT_Void = 1,
	TDT_Character,
	TDT_Word,
	TDT_Integer,
	TDT_Size,
	TDT_Pointer,
	TDT_PointerOwned,
	TDT_String,
	TDT_StringOwned,
	TDT_StringSlice,
	TDT_InstructionMeta,
	TDT_DirectiveType,
} TokenDataType;
typedef struct TokenData {
	TokenDataType dataType;
	union {
		char character;
		uint16_t word;
		long int integer;
		size_t size;
		void *pointer;
		char *string;
		StringSlice string_slice;
		InstructionMeta instruction_meta;
		DirectiveType directive_type;
	};
} TokenData;

// Functions
TokenType parse(const char *lexeme, size_t length, TokenData *tokenData);
StringSlice tokendata_expect_string(TokenData *tokenData);
void free_tokendata(TokenData *tokenData);

