.PHONY: all clean hello

CFLAGS =  -std=c18
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wpointer-arith
CFLAGS += -Wcast-align
CFLAGS += -Wwrite-strings
CFLAGS += -Wswitch-default
CFLAGS += -Wunreachable-code
CFLAGS += -Winit-self
CFLAGS += -Wmissing-field-initializers
CFLAGS += -Wno-unknown-pragmas
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wundef
CFLAGS += -Wold-style-definition
CFLAGS += -Wno-misleading-indentation
CFLAGS += -Wfatal-errors
CFLAGS += -pedantic

CC = gcc $(CFLAGS)
SRC = src
OUT = out

all: $(OUT)/lc3asm
clean:
	@rm -rf ./$(OUT)/*
	@find $(SRC) -name '*.gch' -type f -delete
	@echo Cleaned \'./$(OUT)\' and pre-compiled header files
hello: all $(OUT)/hello.obj
	@hexdump -C $(OUT)/hello.obj

$(OUT)/hello.obj: hello.asm $(OUT)/lc3asm
	$(OUT)/lc3asm hello.asm >$(OUT)/hello.obj

$(OUT)/lc3asm: $(OUT)/lc3asm.o $(OUT)/lc3lex.o $(OUT)/lc3tok.o $(OUT)/lc3cu.o
	@mkdir -p $(OUT)
	$(CC) $^ -o $@

$(OUT)/lc3asm.o: $(SRC)/lc3asm.c $(SRC)/lc3asm.h.gch $(SRC)/lc3lex.h $(SRC)/lc3tok.h $(SRC)/lc3cu.h
	@mkdir -p $(OUT)
	$(CC) $< -c -o $@
$(OUT)/lc3lex.o: $(SRC)/lc3lex.c $(SRC)/lc3asm.h.gch $(SRC)/lc3lex.h
	@mkdir -p $(OUT)
	$(CC) $< -c -o $@
$(OUT)/lc3tok.o: $(SRC)/lc3tok.c $(SRC)/lc3asm.h.gch $(SRC)/lc3tok.h
	@mkdir -p $(OUT)
	$(CC) $< -c -o $@
$(OUT)/lc3cu.o: $(SRC)/lc3cu.c $(SRC)/lc3asm.h.gch $(SRC)/lc3cu.h
	@mkdir -p $(OUT)
	$(CC) $< -c -o $@

$(SRC)/lc3asm.h.gch: $(SRC)/lc3asm.h
	$(CC) $<

